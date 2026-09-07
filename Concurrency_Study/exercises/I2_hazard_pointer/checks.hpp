#pragma once
#include "student.hpp"
#include "concurrency_study/exercise_check.hpp"
#include <algorithm>
#include <future>
#include <iostream>
#include <vector>

namespace student_checks {
// 输入构造与异常收尾由题目提供；所有 pop 都走 student::pop。
class stack {
    std::atomic<student::node*> head_{nullptr};
public:
    stack() = default;
    stack(const stack&) = delete;
    ~stack() {
        // 使用者已收束。仅为异常清理，不作为学生 cleanup 的通过证据。
        auto* p = head_.exchange(nullptr);
        while (p) { auto* next = p->next; delete p; p = next; }
        cs::hazard_pointer_cleanup();
    }
    void push(int value) {
        auto fresh = std::make_unique<student::node>(value);
        fresh->next = head_.load();
        while (!head_.compare_exchange_weak(fresh->next, fresh.get())) {}
        fresh.release();
    }
    bool pop(int& out) { return student::pop(head_, out); }
};

// 全部 TODO 都在启动线程前实际调用，结果不依赖完成标志。
inline void preflight() {
    cs::hazard_pointer_cleanup();
    const int before = student::node::destroyed.load();
    auto hp = cs::make_hazard_pointer();
    std::atomic<student::node*> source{new student::node(42)};
    const auto finish = [&] {
        hp.reset_protection();
        delete source.exchange(nullptr);
        cs::hazard_pointer_cleanup();
    };
    try {
        cs::check(student::protect(hp, source) == source.load(), "protect must return source");
        student::retire(std::unique_ptr<student::node>{source.exchange(nullptr)});
        cs::check(student::cleanup() == 0 && student::node::destroyed == before,
                  "protected node reclaimed early");
        hp.reset_protection();
        cs::check(student::cleanup() == 1 && student::node::destroyed == before + 1,
                  "cleanup must actually finish deletion");
    } catch (...) { finish(); throw; }
    finish();
    stack s;
    s.push(10); s.push(20);
    int out = -1;
    cs::check(s.pop(out) && out == 20, "student pop LIFO first");
    cs::check(s.pop(out) && out == 10, "student pop LIFO second");
    cs::check(!s.pop(out), "student pop empty");
    student::cleanup();
    cs::check(student::node::created == student::node::destroyed, "preflight ownership");
}

inline void run() {
    preflight();
    std::cout << "preflight PASS; starting student workers\n";
    constexpr int workers = 4, count = 1000;
    stack s;
    std::vector<std::future<std::vector<int>>> tasks;
    tasks.reserve(workers);
    for (int t = 0; t < workers; ++t) {
        tasks.push_back(std::async(std::launch::async, [&, t] {
            std::vector<int> result;
            result.reserve(count);
            for (int i = 0; i < count; ++i) {
                s.push(t * count + i);
                int out;
                if (s.pop(out)) result.push_back(out);
            }
            return result;
        }));
    }
    // tasks 先于 s 析构；worker 不等待主线程门闩，失败也能自行结束。
    std::vector<int> all;
    for (auto& task : tasks) {
        auto part = task.get();
        all.insert(all.end(), part.begin(), part.end());
    }
    int out;
    while (s.pop(out)) all.push_back(out);
    student::cleanup();
    cs::check(student::node::created == student::node::destroyed, "student cleanup leaked");
    std::sort(all.begin(), all.end());
    cs::check(all.size() == workers * count, "student pop lost IDs");
    for (int i = 0; i < workers * count; ++i)
        cs::check(all[static_cast<std::size_t>(i)] == i, "student pop duplicate/missing ID");
    std::cout << "I2 student PASS: 4000 unique IDs, LIFO, protection and cleanup\n";
}
} // namespace student_checks
