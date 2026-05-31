// =====================================================================
// 练习 G-2：ABA 问题（the ABA problem）
//   对应文档：Concurrency_Study/09-模块G-无锁数据结构.md 的 练习 G-2
//
//   学习目标：
//     - 理解 ABA：一个线程读到 head == A，挂起；其间别的线程把 head 改成
//       B、又改回 A（甚至 A 是被释放后重新分配/复用的同址节点），原线程
//       醒来做 compare_exchange，发现“还是 A”便误判“没人动过”而成功提交，
//       却把基于陈旧假设算出的新值写了进去 → 链表被破坏 / 丢节点；
//     - CAS 只比较“值（这里是指针）”，不比较“值的历史”，这是 ABA 的根因；
//     - 用带版本号的标签指针（tagged pointer / version counter）破解 ABA：
//       把 (指针, 计数) 打包成一个原子整体做 CAS，每次成功改动 +1，
//       于是“A 变 B 又变回 A”的计数器一定变了，CAS 必然失败、强制重试；
//     - 知道 hazard pointer / RCU 是另一类（基于安全回收的）解法，详见模块 I。
//
//   官方参考：
//     - https://en.cppreference.com/w/cpp/atomic/atomic/compare_exchange
//     - 《C++ Concurrency in Action, 2nd ed.》(Anthony Williams) 第 7 章
//       （7.2.2 “停止内存泄漏：用风险指针管理内存” 引出 ABA 与回收）
//     - Fedor Pikus, "Lock-free Programming" — ABA 一节
//
//   编译运行（VS2026, C++20）：
//     cmake --build build-vs2026 --target G2_aba_problem --config Release
//
//   说明：为了让 ABA【确定性复现】（而非靠运气撞时序），本题用两个
//   std::atomic<bool> 信号把两个线程的步骤严格编排成想要的交错顺序。
// =====================================================================
#include "concurrency_study/log.hpp"

#include <atomic>
#include <cstdint>
#include <thread>

// =====================================================================
// 第一部分：演示“朴素指针 CAS 的栈”如何被 ABA 击穿。
//
//   场景（经典 ABA）：栈自底向上是  C -> B -> A(top)，head == A。
//     线程 1（受害者）：读 old_head = A，记下 A->next == B，准备
//                       CAS(head: A -> B) 完成一次 pop。此时被挂起。
//     线程 2（捣乱者）：pop A（head 变 B）；pop B（head 变 C）；
//                       随后“复用”同一个 A 节点重新 push（head 又变回 A），
//                       但此时 A->next 已经不再是 B，而是 C。
//     线程 1 恢复：它的 CAS 期望 head==A —— 确实还是 A（值相同！）—— 成功，
//                  于是把 head 设成它【缓存的旧 next == B】。但 B 早被 pop
//                  走了，已不在栈里！栈被改成 head=B（一个已弹出的悬空节点），
//                  C 丢失。这就是 ABA 造成的逻辑破坏。
// =====================================================================
struct Node {
    int value;
    Node* next;
    explicit Node(int v) : value(v), next(nullptr) {}
};

class NaiveStack {
public:
    // 仅把节点接到 head（用于搭建初始链表），非线程安全细节本题不关心。
    void push_node(Node* n) {
        n->next = head_.load(std::memory_order_relaxed);
        head_.store(n, std::memory_order_relaxed);
    }

    std::atomic<Node*>& head() { return head_; }

private:
    std::atomic<Node*> head_{nullptr};
};

static void demo_aba_broken() {
    cs::println("---- 第一部分：朴素指针 CAS 被 ABA 击穿 ----");

    // 搭建栈：C -> B -> A(top)。三个节点地址各不相同。
    Node* C = new Node(3);
    Node* B = new Node(2);
    Node* A = new Node(1);
    NaiveStack st;
    st.push_node(C);
    st.push_node(B);
    st.push_node(A); // head == A, A->next == B, B->next == C

    // 两个编排信号：保证线程交错严格按 ABA 剧本走。
    std::atomic<bool> victim_loaded{false};   // 受害者已读到 old_head==A 并缓存 next
    std::atomic<bool> attacker_done{false};   // 捣乱者完成 A->B->A 改回

    // 线程 1：受害者。读 A，缓存 next，等捣乱者把 head 折腾回 A 后再 CAS。
    std::thread victim([&] {
        Node* old_head = st.head().load(std::memory_order_acquire); // == A
        Node* cached_next = old_head->next;                          // 缓存 == B
        cs::logf("[victim] 读到 head=A(value=", old_head->value,
                 ")，缓存 next=B(value=", cached_next->value, ")，即将挂起。");

        victim_loaded.store(true, std::memory_order_release);
        // 等捣乱者把 head 变成 B、C，又把 A 复用 push 回去（head 再次 == A）。
        while (!attacker_done.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }

        // 朴素 CAS：只比较指针值。head 现在确实 == A（值相同！），于是成功，
        // 把 head 设成【陈旧的 cached_next == B】—— 而 B 早已被弹出！
        bool ok = st.head().compare_exchange_strong(
            old_head, cached_next,
            std::memory_order_acq_rel, std::memory_order_acquire);
        cs::logf("[victim] CAS(head: A -> 旧next B) 结果=", (ok ? "成功(!!)" : "失败"),
                 "  —— 成功恰恰是 ABA 的灾难：把 head 指向了已弹出的 B。");
    });

    std::thread attacker([&] {
        while (!victim_loaded.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        // pop A：head A -> B
        Node* h = st.head().load(std::memory_order_acquire);
        st.head().store(h->next, std::memory_order_release); // head == B
        cs::logf("[attacker] pop A，head 现在=B。");
        // pop B：head B -> C
        h = st.head().load(std::memory_order_acquire);
        st.head().store(h->next, std::memory_order_release); // head == C
        cs::logf("[attacker] pop B，head 现在=C。");
        // “复用”A 重新 push：A->next 指向当前 head(C)，head 变回 A。
        A->next = st.head().load(std::memory_order_acquire); // A->next = C
        st.head().store(A, std::memory_order_release);       // head == A 又回来了
        cs::logf("[attacker] 复用 A 重新 push，head 又变回 A（但 A->next 现在是 C 不是 B！）。");

        attacker_done.store(true, std::memory_order_release);
    });

    victim.join();
    attacker.join();

    // 检查损坏：若受害者 CAS 成功，head 现在指向 B（已弹出节点），栈被破坏。
    Node* final_head = st.head().load(std::memory_order_acquire);
    cs::logf("[check] 最终 head value=", final_head->value,
             "（== 2 表示 head 指向了被弹出的 B，链表已损坏 → ABA 发生）。");

    cs::println("");
    // 教学演示，节点不回收（无锁回收见模块 I）。delete 略。
}

// =====================================================================
// 第二部分：用带版本号的标签指针（tagged pointer）破解 ABA。
//
//   思路：把 head 表示为一个 {下标/指针, version} 组合，整体做原子 CAS。
//   每次成功改动都让 version += 1。于是“A 变 B 又变回 A”这条路径上，
//   version 一定被改过 → 受害者拿着旧 version 的 CAS 必然失败、被迫重读、
//   看到真实的最新状态。CAS 比较的是 (指针, version) 的【整体】，
//   而不再只是裸指针值，ABA 被根除。
//
//   这里用一个 64 位整数把 (32 位节点下标) 和 (32 位版本号) 打包，
//   存进 std::atomic<std::uint64_t>，避免依赖平台是否支持双字 CAS。
//   下标 0xFFFFFFFF 约定为“空”。
// =====================================================================
struct TaggedStack {
    static constexpr std::uint32_t kNull = 0xFFFFFFFFu;

    // 用一个简单的节点池（数组）让“下标”可寻址；教学用，固定容量。
    Node* pool[8] = {};
    std::atomic<std::uint64_t> head{pack(kNull, 0)};

    static std::uint64_t pack(std::uint32_t idx, std::uint32_t ver) {
        return (static_cast<std::uint64_t>(ver) << 32) | idx;
    }
    static std::uint32_t idx_of(std::uint64_t h) {
        return static_cast<std::uint32_t>(h & 0xFFFFFFFFu);
    }
    static std::uint32_t ver_of(std::uint64_t h) {
        return static_cast<std::uint32_t>(h >> 32);
    }

    // 把池中下标 i 的节点压栈（版本号 +1）。
    void push_idx(std::uint32_t i) {
        std::uint64_t old_h = head.load(std::memory_order_relaxed);
        std::uint64_t new_h;
        do {
            pool[i]->next = (idx_of(old_h) == kNull) ? nullptr : pool[idx_of(old_h)];
            new_h = pack(i, ver_of(old_h) + 1); // 版本号 +1：让任何改动都可被察觉
        } while (!head.compare_exchange_weak(old_h, new_h,
                                             std::memory_order_release,
                                             std::memory_order_relaxed));
    }
};

static void demo_aba_fixed() {
    cs::println("---- 第二部分：带版本号的标签指针破解 ABA ----");

    TaggedStack st;
    st.pool[0] = new Node(3); // C
    st.pool[1] = new Node(2); // B
    st.pool[2] = new Node(1); // A
    st.push_idx(0);           // C
    st.push_idx(1);           // B
    st.push_idx(2);           // A，此时 head 的下标==2(A)，version==3

    std::atomic<bool> victim_loaded{false};
    std::atomic<bool> attacker_done{false};

    // 受害者：读到 head==(idx=A, ver=v0)，缓存其 next；待捣乱者折腾后再 CAS。
    std::thread victim([&] {
        std::uint64_t old_h = st.head.load(std::memory_order_acquire);
        std::uint32_t old_idx = TaggedStack::idx_of(old_h); // A
        std::uint32_t old_ver = TaggedStack::ver_of(old_h);
        Node* cached_next = st.pool[old_idx]->next;          // == B
        std::uint32_t cached_next_idx =
            (cached_next == nullptr) ? TaggedStack::kNull : 1u; // B 的下标=1
        cs::logf("[victim] 读到 head=(idx=A, ver=", old_ver, ")，缓存 next=B，挂起。");

        victim_loaded.store(true, std::memory_order_release);
        while (!attacker_done.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }

        // 即便此刻栈顶“看起来又是 A”，但 version 已被捣乱者改动过；
        // 我们用【旧的 old_h（含旧 version）】做 CAS —— 必然失败。
        std::uint64_t desired =
            TaggedStack::pack(cached_next_idx, old_ver + 1);
        bool ok = st.head.compare_exchange_strong(
            old_h, desired,
            std::memory_order_acq_rel, std::memory_order_acquire);
        cs::logf("[victim] 用旧 (idx,ver) 做 CAS 结果=", (ok ? "成功" : "失败(正确!)"),
                 "  —— 失败正是我们想要的：version 变了，受害者被迫重试、看到真实状态。");
        cs::logf("[victim] CAS 失败后 old_h 被刷新为最新 head=(idx=",
                 TaggedStack::idx_of(old_h), ", ver=", TaggedStack::ver_of(old_h),
                 ")，据此重新决策即可。");
    });

    std::thread attacker([&] {
        while (!victim_loaded.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        // 模拟 pop A、pop B，再把 A 复用 push 回来——每一步 version 都会 +1。
        // 这里用一次 push_idx(2) 直接把“A 又回到栈顶”做出来，version 已被推进。
        // （pop 的细节本题不展开；关键是 version 单调递增，旧 CAS 必失败。）
        std::uint64_t h = st.head.load(std::memory_order_acquire);
        // 人为推进若干次版本（等价于 pop A、pop B、push A 这串改动）。
        for (int k = 0; k < 3; ++k) {
            std::uint64_t cur = st.head.load(std::memory_order_acquire);
            std::uint64_t bumped = TaggedStack::pack(
                TaggedStack::idx_of(cur), TaggedStack::ver_of(cur) + 1);
            st.head.store(bumped, std::memory_order_release);
        }
        cs::logf("[attacker] 完成 A->B->A 等价改动，version 已被推进（栈顶下标看似仍是 A）。");
        attacker_done.store(true, std::memory_order_release);
        (void)h;
    });

    victim.join();
    attacker.join();

    cs::println("");
    cs::println("结论：tagged pointer 让 CAS 比较的是 (指针, 版本) 的整体，");
    cs::println("      “变回 A”也骗不过它，ABA 被根除。");
    cs::println("      另一类解法是 hazard pointer / RCU（管住节点何时能安全回收），");
    cs::println("      它们从“别让被复用的同址节点出现”这个角度解决，详见模块 I。");
    cs::println("");
}

int main() {
    cs::println("==== G2_aba_problem：ABA 问题与缓解 ====\n");

    demo_aba_broken();
    demo_aba_fixed();

    cs::println("==== 演示结束。请对照文档“验收点/复盘问题”自检。 ====");
    return 0;
}
