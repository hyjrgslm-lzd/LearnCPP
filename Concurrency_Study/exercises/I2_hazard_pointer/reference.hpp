#pragma once
#include "concurrency_study/hazard_pointer.hpp"
#include "concurrency_study/exercise_check.hpp"
#include <algorithm>
#include <atomic>
#include <future>
#include <iostream>
#include <vector>

namespace hp_exercise {
// 有限实验只存 int，避免 CAS 摘除成功后复制元素抛异常的额外所有权分支。
// 节点只发布一次；next 发布后不改；多生产者/多消费者，无固定容量。
class stack {
    struct node : cs::hazard_pointer_obj_base<node> {
        int value;
        node* next = nullptr;
        inline static std::atomic<int> destroyed{0};
        explicit node(int v) : value(v) {}
        ~node() { ++destroyed; }
    };
    std::atomic<node*> head_{nullptr};
public:
    stack() = default;
    stack(const stack&) = delete;
    stack& operator=(const stack&) = delete;
    ~stack() {
        // 先 join 全部使用者才准析构。剩余可达节点也走同一退休路径。
        node* p = head_.exchange(nullptr);
        while (p) { node* next = p->next; p->retire(); p = next; }
        cs::hazard_pointer_cleanup();
    }
    static int destroyed() { return node::destroyed.load(); }
    void push(int value) {
        auto* fresh = new node(value);
        fresh->next = head_.load();
        while (!head_.compare_exchange_weak(fresh->next, fresh)) {}
    }
    bool pop(int& out) {
        auto hp = cs::make_hazard_pointer();
        for (;;) {
            node* old = hp.protect(head_);
            if (!old) return false; // 空读取为本次失败的线性化点。
            node* next = old->next; // 必须在保护成立以后读取。
            if (!head_.compare_exchange_weak(old, next)) continue;
            out = old->value; // 只有摘除成功者读出 int；这里不会抛。
            old->retire();
            hp.reset_protection();
            return true;
        }
    }
};
inline void run() {
    constexpr int workers = 4, per_worker = 1000;
    const int before = stack::destroyed();
    std::vector<int> all;
    all.reserve(workers * per_worker);
    {
        stack s;
        std::vector<std::future<std::vector<int>>> tasks;
        for (int t = 0; t < workers; ++t) {
            tasks.push_back(std::async(std::launch::async, [&, t] {
                std::vector<int> values;
                values.reserve(per_worker);
                for (int i = 0; i < per_worker; ++i) {
                    s.push(t * per_worker + i);
                    int out;
                    if (s.pop(out)) values.push_back(out);
                }
                return values; // 异常由 future.get 回传主线程。
            }));
        }
        for (auto& task : tasks) {
            auto values = task.get();
            all.insert(all.end(), values.begin(), values.end());
        }
        int out;
        while (s.pop(out)) all.push_back(out);
    }
    std::sort(all.begin(), all.end());
    cs::check(all.size() == workers * per_worker, "HP stack lost items");
    for (int i = 0; i < workers * per_worker; ++i)
        cs::check(all[static_cast<std::size_t>(i)] == i, "HP stack duplicate/missing ID");
    cs::check(stack::destroyed() - before == workers * per_worker, "HP final cleanup leaked");
    // 单线程历史补充 LIFO；并发唯一 ID 检查不冒充线性化证明。
    {
        stack s;
        s.push(1); s.push(2);
        int out;
        cs::check(s.pop(out) && out == 2, "LIFO first");
        cs::check(s.pop(out) && out == 1, "LIFO second");
        cs::check(!s.pop(out), "empty stack");
    }
    std::cout << "HP: 4000 unique IDs, exact destruction, LIFO PASS\n";
}
} // namespace hp_exercise
