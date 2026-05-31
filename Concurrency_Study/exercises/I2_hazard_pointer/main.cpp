// =====================================================================
// 练习 I-2：hazard pointer 安全回收
//   对应文档：Concurrency_Study/11-模块I-安全内存回收.md 的 练习 I-2
//
//   学习目标：
//     - 给模块 G 的 Treiber 无锁栈（lock-free stack）的 pop 加上【安全的节点
//       回收】，彻底消除“pop 后 delete 节点”导致的 use-after-free。
//     - 掌握风险指针（hazard pointer）的保护-回收协议：
//         · 读者（这里是 pop）在解引用 head 之前，先用 hazard_pointer::protect
//           把 head【登记保护】，并重读校验确认登记期间它没被换走；
//         · 安全访问（读 next、CAS 摘下）之后，旧 head 不直接 delete，而是
//           retire() 交给回收系统延迟删除；
//         · 回收系统只 delete 那些“没有任何 hazard 槽正在保护”的退休节点。
//     - 对比模块 G-1：那里 pop 故意泄漏（永不 delete）以回避 use-after-free；
//       这里用 hazard pointer 让“既安全回收、又无锁”同时成立。
//
//     ★ 本题使用本仓库自带的【教学版】风险指针（cs::hazard_pointer，
//       concurrency_study/hazard_pointer.hpp）。C++26 标准在 std 命名空间
//       <hazard_pointer> 提供等价设施（提案 P2530R3），但 MSVC VS2026 尚未
//       实现，故用 cs:: 教学版。两者 API 的差异映射见模块文档的对照表。
//
//   官方参考：
//     - 提案 P2530R3 "Hazard Pointers for C++26"：
//       https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2530r3.pdf
//     - 早期提案 P0566
//     - cppreference（C++26）<hazard_pointer>：
//       https://en.cppreference.com/w/cpp/header/hazard_pointer
//     - folly Hazptr（工业级对照）：
//       https://github.com/facebook/folly/blob/main/folly/synchronization/Hazptr.h
//     - 《C++ Concurrency in Action, 2nd ed.》(Williams) 第 7 章 7.2.2
//
//   编译运行（VS2026, C++20）：
//     cmake --build build-vs2026 --target I2_hazard_pointer --config Release
// =====================================================================
#include "concurrency_study/hazard_pointer.hpp"
#include "concurrency_study/log.hpp"

#include <atomic>
#include <thread>
#include <vector>

// =====================================================================
// 带安全回收的 Treiber 无锁栈。
//   与模块 G-1 的唯一区别：pop 不再泄漏节点，而是借助 hazard pointer 安全回收。
//
//   节点继承 cs::hazard_pointer_obj_base，于是它带上了 retire()——退休时不
//   立即 delete，而是登记到回收系统，等“没有读者正在保护它”时才真正释放。
// =====================================================================
template <class T>
class TreiberStackHP {
public:
    TreiberStackHP() = default;
    TreiberStackHP(const TreiberStackHP&) = delete;
    TreiberStackHP& operator=(const TreiberStackHP&) = delete;

    ~TreiberStackHP() {
        // 析构：把剩余节点直接 delete（此时无并发，安全）。
        Node* n = head_.load(std::memory_order_relaxed);
        while (n) {
            Node* next = n->next;
            delete n;
            n = next;
        }
    }

    // push 与 G-1 完全一致：new 节点，CAS 接到 head，release 发布。
    void push(T value) {
        Node* new_node = new Node(std::move(value));
        new_node->next = head_.load(std::memory_order_relaxed);
        while (!head_.compare_exchange_weak(new_node->next, new_node,
                                            std::memory_order_release,
                                            std::memory_order_relaxed)) {
            // 失败：new_node->next 已被刷新为最新 head，重试。
        }
    }

    // -----------------------------------------------------------------
    // pop —— 用 hazard pointer 保护 head 后安全摘下，旧节点 retire 延迟回收。
    //
    //   协议（这是本题的灵魂）：
    //     1) 申请一个 hazard_pointer（持有一个全局保护槽）；
    //     2) protect(head_)：登记“我正在保护 head 的当前值”并重读校验；
    //        —— 校验通过后，被保护的节点在本 hazard_pointer 释放前不会被回收，
    //           于是接下来读 old->next 绝不会 use-after-free；
    //     3) 若 head 为空 -> 空栈返回 false；
    //     4) CAS 把 head 从 old 换成 old->next（与 push 的 release 配对，acquire）；
    //        CAS 失败说明 head 变了，重读 + 重新 protect；
    //     5) 成功摘下 old 后，取走值，调用 old->retire()——不立即 delete，
    //        交给回收系统：等没有任何 hazard 槽保护 old 时才真正释放。
    // -----------------------------------------------------------------
    bool pop(T& out) {
        // TODO [必做 1]: 申请 hazard pointer 并用它保护读出的 head。
        //   参考实现已给出以保证可编译运行；理解后请遮住重写一遍。
        //   关键：protect 返回的指针在本 hp 析构前都不会被回收，
        //   所以下面读 old->next / old->data 都是安全的（无 use-after-free）。
        //
        //   cs::hazard_pointer hp = cs::make_hazard_pointer();
        cs::hazard_pointer hp = cs::make_hazard_pointer();

        Node* old = nullptr;
        while (true) {
            // protect：登记并校验。返回值就是“被安全保护住”的当前 head。
            old = hp.protect(head_);
            if (old == nullptr) {
                return false; // 空栈
            }
            // old 已被保护，读 old->next 安全。尝试 CAS 摘下。
            Node* next = old->next;
            if (head_.compare_exchange_weak(old, next,
                                            std::memory_order_acquire,
                                            std::memory_order_relaxed)) {
                break; // 成功摘下 old
            }
            // 失败：head 已变（old 被刷新）。循环顶部会重新 protect 最新 head。
        }

        out = old->data;

        // TODO [必做 1]: 安全访问完毕后，retire 旧节点（延迟回收，而非泄漏/直接 delete）。
        //   关键：此刻 old 已从栈上摘下，逻辑上不再可达；但可能仍有别的线程的
        //   hazard_pointer 正保护着它（它们在我们 CAS 前读到了同一个 old）。
        //   所以【不能直接 delete】，而是 retire：回收系统会在确认无人保护后再删。
        //   这正是相对 G-1“故意泄漏”的根本改进——既安全又最终真正释放。
        //
        //   old->retire();
        old->retire();

        // 清除本次保护（hp 析构时也会清，这里显式清更直观）。
        hp.reset_protection(nullptr);
        return true;
    }

private:
    // 节点继承 hazard_pointer_obj_base 以获得 retire() 能力。
    struct Node : cs::hazard_pointer_obj_base<Node> {
        explicit Node(T v) : data(std::move(v)), next(nullptr) {}
        T data;
        Node* next;
    };

    std::atomic<Node*> head_{nullptr};
};

int main() {
    cs::println("==== I2_hazard_pointer：用风险指针给 Treiber 栈安全回收 ====\n");

    constexpr int kThreads = 4;
    constexpr int kOpsPerThread = 50000;

    TreiberStackHP<int> stack;

    // 必做 1（验收）：多线程同时 push/pop 混合压测，全程不崩（无 use-after-free）、
    //   收支平衡（无丢失）。每个线程交替 push 与 pop，pop 出来的节点被安全回收。
    std::atomic<long long> push_count{0};
    std::atomic<long long> pop_count{0};
    std::atomic<bool> start{false};

    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t] {
            while (!start.load(std::memory_order_acquire)) std::this_thread::yield();
            long long local_push = 0, local_pop = 0;
            int value = 0;
            for (int i = 0; i < kOpsPerThread; ++i) {
                if ((i & 1) == 0) {
                    stack.push(t * kOpsPerThread + i);
                    ++local_push;
                } else {
                    if (stack.pop(value)) {
                        ++local_pop; // 安全解引用 + 安全回收，不应崩溃
                    }
                }
            }
            push_count.fetch_add(local_push, std::memory_order_relaxed);
            pop_count.fetch_add(local_pop, std::memory_order_relaxed);
            cs::logf("[worker ", t, "] push=", local_push, " pop=", local_pop);
        });
    }

    start.store(true, std::memory_order_release);
    for (auto& th : threads) th.join();

    // 把残留元素全部 pop 干净，验证总收支平衡。
    int v = 0;
    long long drained = 0;
    while (stack.pop(v)) ++drained;

    const long long total_push = push_count.load();
    const long long total_pop = pop_count.load() + drained;
    cs::logf("[main] 压测期间 push=", total_push,
             " pop=", pop_count.load(), " 收尾 drain=", drained);
    cs::logf("[main] 总 push=", total_push, " 总 pop=", total_pop, " -- ",
             (total_push == total_pop ? "收支平衡 OK（不丢不重，无 use-after-free 崩溃）"
                                      : "不平衡 MISMATCH"));

    cs::println("\n要点：");
    cs::println("  - protect 读出的 head -> 安全读 next/data -> CAS 摘下 -> retire 旧节点。");
    cs::println("  - retire 不立即 delete：回收系统确认无 hazard 槽保护该节点后才释放。");
    cs::println("  - 相比 G-1 的“故意泄漏”，这里既消除 use-after-free，又最终真正回收内存。");
    cs::println("\n==== 演示结束。请对照文档“验收点/复盘问题”自检。 ====");
    return 0;
}
