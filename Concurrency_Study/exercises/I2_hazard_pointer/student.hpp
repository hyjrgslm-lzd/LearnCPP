#pragma once
#include "concurrency_study/hazard_pointer.hpp"
#include <atomic>
#include <memory>
#include <stdexcept>

namespace student {
struct node : cs::hazard_pointer_obj_base<node> {
    const int value;
    node* next = nullptr; // 首次发布后不修改，不重新插入摘除节点。
    inline static std::atomic<int> created{0}, destroyed{0};
    explicit node(int v) : value(v) { ++created; }
    ~node() { ++destroyed; }
};

// Part A：调用已有 HP 设施，完成公布+验证；不重写 hazard_pointer.hpp。
inline node* protect(cs::hazard_pointer& hp, const std::atomic<node*>& source) {
    (void)hp; (void)source;
    // TODO A: 返回 hp 成功保护的源指针；只 load 不构成保护。
    throw std::logic_error("TODO A: protect");
}

// removed 是成功摘除者的独占所有权，须转交回收域。
inline void retire(std::unique_ptr<node> removed) {
    (void)removed;
    // TODO B1: release 独占所有权并 retire；不能直接 delete。
    throw std::logic_error("TODO B1: retire");
}

// 已提供循环与空路径；CAS 失败回到 protect，不能解引用 CAS 回填值。
inline bool pop(std::atomic<node*>& head, int& out) {
    auto hp = cs::make_hazard_pointer();
    for (;;) {
        node* old = protect(hp, head);
        if (!old) return false;
        (void)out;
        // TODO B2: 安全读取 next，SC CAS 摘除；失败 continue。
        // 成功后复制 int 到 out，调用本文件 retire，清除 hp，返回 true。
        throw std::logic_error("TODO B2: pop");
    }
}

// Part C：调用者已经 join 并清空栈、结束全部保护。
inline std::size_t cleanup() {
    // TODO C: 实际执行最终 HP 清理，返回完成回调数。
    throw std::logic_error("TODO C: cleanup");
}
} // namespace student
