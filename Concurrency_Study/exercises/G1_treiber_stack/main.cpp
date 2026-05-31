// =====================================================================
// 练习 G-1：Treiber 无锁栈（lock-free stack / Treiber stack）
//   对应文档：Concurrency_Study/09-模块G-无锁数据结构.md 的 练习 G-1
//
//   学习目标：
//     - 掌握 CAS 循环（compare-and-swap loop）这一无锁编程的核心范式：
//       读旧值 → 基于旧值算新值 → compare_exchange_weak 提交；
//       失败说明别人抢先改了 head，旧值被刷新，重试即可；
//     - 用 compare_exchange_weak 操作 head 实现后进先出（LIFO）栈的
//       push（new node，CAS 接到 head）与 pop（CAS 摘 head）；
//     - 说清每一步用什么内存序（memory order）以及为什么；
//     - 理解 lock-free（无锁）进展保证：没有任何线程持锁，总有线程能前进；
//     - 认识 pop 的节点回收难题（reclamation / use-after-free），
//       本题先简化为“泄漏”，真正解法留模块 I（hazard pointer / RCU）。
//
//   官方参考：
//     - https://en.cppreference.com/w/cpp/atomic/atomic
//     - https://en.cppreference.com/w/cpp/atomic/atomic/compare_exchange
//     - 《C++ Concurrency in Action, 2nd ed.》(Anthony Williams) 第 7 章
//       （7.2.1 “实现一个无锁的线程安全栈”）
//     - Fedor Pikus, "Lock-free Programming" (CppCon 演讲)
//
//   编译运行（VS2026, C++20）：
//     cmake --build build-vs2026 --target G1_treiber_stack --config Release
// =====================================================================
#include "concurrency_study/log.hpp"

#include <atomic>
#include <thread>
#include <vector>

// =====================================================================
// Treiber 无锁栈：所有操作都围绕单个原子指针 head 做 CAS。
//
//   栈是一条单向链表，head 指向栈顶节点（LIFO，后进先出）。
//   push：新节点 next 指向当前 head，再用 CAS 把 head 换成新节点。
//   pop ：读 head，用 CAS 把 head 换成 head->next，取走原 head 的值。
//
//   关于节点回收（reclamation）——本题的【已知缺陷】：
//     pop 成功摘下一个节点后，本应 delete 它。但在无锁世界里这是危险的：
//     可能另一个线程此刻正握着同一个节点指针准备读它的 next（见 G-2 的
//     ABA 场景）。直接 delete 会造成 use-after-free。安全回收需要
//     hazard pointer / RCU（读-复制-更新），那是模块 I 的内容。
//     >>> 因此本题刻意【不释放】pop 下来的节点（故意泄漏），把注意力
//         集中在 CAS 循环与内存序上。这是教学简化，生产代码绝不可这样。
// =====================================================================
template <class T>
class TreiberStack {
public:
    TreiberStack() = default;

    // 不可拷贝/移动：内部是裸指针链表 + 原子 head，语义上是共享对象。
    TreiberStack(const TreiberStack&) = delete;
    TreiberStack& operator=(const TreiberStack&) = delete;

    // -----------------------------------------------------------------
    // 必做 1：push —— new 一个节点，CAS 把它接到 head。
    //
    //   范式：
    //     1) new node，让它的 next 指向“我看到的当前 head”；
    //     2) 用 compare_exchange_weak 试图把 head 从 old_head 换成 new_node；
    //     3) 若失败：说明别人抢先改了 head，CAS 已把最新值写回 new_node->next，
    //        于是（next 已被刷新）直接重试（循环体可为空）。
    //
    //   内存序：
    //     - 成功分支用 release：把“写好 new_node->next、data 等内容”这件事
    //       发布出去，让随后 acquire 读到该 head 的线程能看到完整节点
    //       （建立 happens-before）；
    //     - 失败分支用 relaxed：只是拿回最新 head 准备重试，没有要同步的数据。
    // -----------------------------------------------------------------
    void push(T value) {
        Node* new_node = new Node(std::move(value));

        // new_node->next 必须在 CAS 成功“发布”之前写好。
        new_node->next = head_.load(std::memory_order_relaxed);

        // TODO [必做 1]: 写出 push 的 CAS 循环。
        //   下面已直接给出参考实现以保证可编译运行；理解后请遮住重写一遍。
        //   关键：compare_exchange_weak 的第一个参数是【引用】——失败时它会把
        //   head 的最新值写进 new_node->next，所以循环体可以是空的。
        //
        //   while (!head_.compare_exchange_weak(
        //              new_node->next, new_node,
        //              std::memory_order_release,   // 成功：发布新节点
        //              std::memory_order_relaxed))  // 失败：仅取回最新 head
        //   { /* 失败时 next 已被刷新为最新 head，重试 */ }
        while (!head_.compare_exchange_weak(new_node->next, new_node,
                                            std::memory_order_release,
                                            std::memory_order_relaxed)) {
            // 失败：new_node->next 已被刷新为最新 head，重试即可。
        }
    }

    // -----------------------------------------------------------------
    // 必做 1：pop —— 读 head，CAS 把 head 摘成 head->next。
    //
    //   范式：
    //     1) acquire 读当前 head（要 acquire：以便看到 push 端 release 发布的
    //        节点内容，构成 synchronizes-with）；
    //     2) 若 head 为 nullptr → 空栈，返回 false；
    //     3) 否则用 CAS 试图把 head 从 old_head 换成 old_head->next；
    //     4) 成功：取走 old_head->data；失败：old_head 被刷新为最新 head，重试。
    //
    //   返回 true 表示弹出成功，值写入 out。
    // -----------------------------------------------------------------
    bool pop(T& out) {
        // acquire：确保读到的 head 所指节点的内容（含 next）是完整可见的。
        Node* old_head = head_.load(std::memory_order_acquire);

        // TODO [必做 1]: 写出 pop 的 CAS 循环。
        //   下面已直接给出参考实现以保证可编译运行；理解后请遮住重写一遍。
        //   思考：在 CAS 成功之前读 old_head->next 安全吗？
        //     —— 在本题“无回收”的简化前提下安全（节点永不释放，old_head 恒有效）。
        //        一旦引入 delete，这里就会 use-after-free，这正是 ABA（G-2）
        //        与安全回收（模块 I）要解决的问题。
        //
        //   while (old_head &&
        //          !head_.compare_exchange_weak(
        //              old_head, old_head->next,
        //              std::memory_order_acquire,   // 成功：与 push 的 release 配对
        //              std::memory_order_acquire))  // 失败：重读时也需 acquire
        //   { /* 失败时 old_head 被刷新为最新 head，重试 */ }
        while (old_head &&
               !head_.compare_exchange_weak(old_head, old_head->next,
                                            std::memory_order_acquire,
                                            std::memory_order_acquire)) {
            // 失败：old_head 已被刷新为最新 head，重试。
        }

        if (old_head == nullptr) {
            return false; // 空栈
        }

        out = old_head->data;

        // !!! 节点回收：本应 delete old_head，但无锁下不安全（见文件头说明）。
        //     本题刻意【泄漏】，把安全回收留给模块 I。
        // delete old_head;  // <-- 故意注释：直接释放会引入 use-after-free 风险。

        return true;
    }

private:
    struct Node {
        explicit Node(T v) : data(std::move(v)), next(nullptr) {}
        T data;
        Node* next;
    };

    std::atomic<Node*> head_{nullptr};
};

int main() {
    cs::println("==== G1_treiber_stack：Treiber 无锁栈 ====\n");

    constexpr int kThreads = 4;
    constexpr int kPushPerThread = 10000;

    TreiberStack<int> stack;

    // 必做 2：多线程压测 —— N 个线程各自 push，再多线程 pop，
    //         统计弹出的元素总数应等于压入总数（不丢、不重）。
    std::vector<std::thread> pushers;
    for (int t = 0; t < kThreads; ++t) {
        pushers.emplace_back([&stack, t] {
            for (int i = 0; i < kPushPerThread; ++i) {
                stack.push(t * kPushPerThread + i);
            }
            cs::logf("[pusher ", t, "] 完成 push。");
        });
    }
    for (auto& th : pushers) th.join();

    const int total_pushed = kThreads * kPushPerThread;
    cs::logf("[main] 全部 push 完毕，共 ", total_pushed, " 个元素。开始多线程 pop。");

    std::atomic<int> popped_total{0};
    std::vector<std::thread> poppers;
    for (int t = 0; t < kThreads; ++t) {
        poppers.emplace_back([&stack, &popped_total] {
            int value = 0;
            int local = 0;
            while (stack.pop(value)) {
                ++local;
            }
            popped_total.fetch_add(local, std::memory_order_relaxed);
        });
    }
    for (auto& th : poppers) th.join();

    const int actual = popped_total.load();
    cs::logf("[main] 期望弹出 ", total_pushed, " 个，实际弹出 ", actual, " 个 -- ",
             (actual == total_pushed ? "一致 OK（不丢不重）" : "不一致 MISMATCH"));

    cs::println("\n注意：pop 下来的节点本题【刻意泄漏】未释放——");
    cs::println("      无锁安全回收（hazard pointer / RCU）留到模块 I 详解。");
    cs::println("\n==== 演示结束。请对照文档“验收点/复盘问题”自检。 ====");
    return 0;
}
