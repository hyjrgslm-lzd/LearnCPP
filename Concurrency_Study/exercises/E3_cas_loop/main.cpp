// =====================================================================
// 练习 E-3：CAS 循环（compare_exchange_weak / strong）
//   对应文档：Concurrency_Study/07-模块E-原子操作基础.md 的 练习 E-3
//
//   学习目标：
//     - 掌握“比较并交换”（compare-and-swap，CAS）这一切无锁编程的基石：
//       compare_exchange_weak/strong(expected, desired) 原子地“若当前值
//       等于 expected 则改成 desired”，并把当前值回写进 expected；
//     - 学会 CAS 循环（CAS loop）范式：用它实现标准库没有内置的任意
//       “读-改-写”（read-modify-write）操作——本题用 CAS 循环实现
//       atomic fetch_max（原子取最大值）；
//     - 说清 weak 与 strong 的取舍：weak 允许“伪失败”（spurious failure，
//       即便值确实等于 expected 也可能返回 false），但在某些平台上更快、
//       且天然适合放进循环里；strong 不会伪失败，适合不便重试的单次调用。
//
//   关于内存序：本题仍用默认 seq_cst；CAS 的“成功/失败可用不同内存序”
//   这一更细的控制留到模块 F。
//
//   官方参考：
//     - https://en.cppreference.com/w/cpp/atomic/atomic/compare_exchange
//     - https://en.cppreference.com/w/cpp/atomic/atomic/fetch_add
//     - 《C++ Concurrency in Action, 2nd ed.》(Anthony Williams) 第 5 章 5.2.4
//
//   编译运行（VS2026, C++20）：
//     cmake --build build-vs2026 --target E3_cas_loop --config Release
//     ./build-vs2026/E3_cas_loop/Release/E3_cas_loop.exe
// =====================================================================
#include "concurrency_study/log.hpp"

#include <atomic>
#include <thread>
#include <vector>

// =====================================================================
// 必做 1：用 CAS 循环实现 atomic fetch_max（原子取最大值）。
//   标准库给了 fetch_add/sub/and/or/xor，但没有 fetch_max。当我们要做的
//   “读-改-写”不在内置清单里时，通用解法就是 CAS 循环：
//     1) load 出当前值 cur（作为 expected）；
//     2) 算出想要写入的新值 desired（这里是 max(cur, candidate)）；
//     3) compare_exchange_weak(cur, desired)：
//        - 成功（当前确实还是 cur）→ 写入完成，退出循环；
//        - 失败（别的线程已改了值）→ cur 被自动刷新为“最新当前值”，
//          回到第 2 步用新 cur 重算重试。
//   这个“读—算—试，不行就拿着最新值重来”的骨架，是后续无锁数据结构
//   （模块 G）里反复出现的核心套路。
// =====================================================================
void atomic_fetch_max(std::atomic<int>& target, int candidate) {
    // 先读一次当前值作为初始 expected。
    int cur = target.load();
    // TODO [必做 1]: 用 compare_exchange_weak 写一个 CAS 循环，把 target
    //   原子地更新为 max(target, candidate)。
    //   要点：
    //     - 若 candidate <= cur，无需更新，可直接返回（这是一个常见优化，
    //       避免无谓的 CAS）；
    //     - compare_exchange_weak(cur, desired)：成功返回 true 且 target
    //       已被设为 desired；失败返回 false，并把 cur 刷新为最新当前值，
    //       于是循环条件用新 cur 重新判断/计算；
    //     - 用 weak 而非 strong：它可能“伪失败”，但放在循环里无所谓
    //       （伪失败只是多转一圈），且在部分平台上更高效。
    //   下面已是正确写法，留作必做 1 的参考实现：
    while (candidate > cur) {
        // 尝试把 target 从 cur 改成 candidate。
        if (target.compare_exchange_weak(cur, candidate)) {
            return; // 成功：我把它抬高到了 candidate
        }
        // 失败：cur 已被刷新为别人写入的最新值；循环条件用新 cur 重判。
        // 若此刻别人已把值抬得 >= candidate，candidate > cur 不再成立，退出。
    }
}

void demo_fetch_max() {
    cs::println("========= 必做 1：CAS 循环实现 atomic fetch_max =========");

    std::atomic<int> hi{0}; // 全局“目前见过的最大值”
    constexpr int kThreads = 8;
    constexpr int kPerThread = 50000;

    // 每个线程抛出一串候选值竞争去抬高 hi；最终 hi 必须等于所有候选里的最大者。
    // 我们让线程 i 的候选范围是 [i*kPerThread, (i+1)*kPerThread)，
    // 因此全局最大候选 = kThreads*kPerThread - 1。
    std::vector<std::thread> ts;
    for (int i = 0; i < kThreads; ++i) {
        ts.emplace_back([&hi, i] {
            const int base = i * kPerThread;
            for (int k = 0; k < kPerThread; ++k) {
                atomic_fetch_max(hi, base + k);
            }
        });
    }
    for (auto& t : ts) t.join();

    const int expected = kThreads * kPerThread - 1;
    cs::logf("[max] hi = ", hi.load(), "，期望 = ", expected,
             "（", (hi.load() == expected ? "精确相等" : "异常"), "）");
    cs::println("");
}

// =====================================================================
// 必做 2：再用 CAS 循环实现一个“原子乘法”，巩固范式。
//   乘法同样不是内置 RMW，套用同一骨架即可：load 当前值 → 算 cur*factor
//   → compare_exchange_weak，失败就拿最新 cur 重试。
//   这里单线程演示正确性即可（多线程乘法本身意义不大，重点是范式）。
// =====================================================================
void atomic_fetch_multiply(std::atomic<long long>& target, long long factor) {
    long long cur = target.load();
    // TODO [必做 2]: 用 CAS 循环把 target 原子地乘以 factor。
    //   要点：与 fetch_max 同构——desired = cur * factor；
    //   compare_exchange_weak 失败时 cur 被自动刷新，循环重算。
    //   下面已是正确写法，留作必做 2 的参考实现：
    while (!target.compare_exchange_weak(cur, cur * factor)) {
        // 失败：cur 已是最新当前值，循环体下一轮用它重算 cur*factor。
    }
}

void demo_fetch_multiply() {
    cs::println("======== 必做 2：CAS 循环实现 atomic fetch_multiply ========");

    std::atomic<long long> v{1};
    for (int i = 0; i < 10; ++i) {
        atomic_fetch_multiply(v, 2); // 连乘 2，共 10 次 → 1024
    }
    cs::logf("[mul] v = ", v.load(), "，期望 = 1024（",
             (v.load() == 1024 ? "相等" : "异常"), "）");
    cs::println("");
}

// =====================================================================
// 进阶 1：观察 weak 与 strong 的语义差异。
//   - compare_exchange_strong：除非当前值确实不等于 expected，否则不会
//     失败——没有伪失败。适合“不放在循环里、不便重试”的一次性判断。
//   - compare_exchange_weak：即使当前值等于 expected 也可能（伪）失败，
//     因此几乎总要写在循环里。优点是在 LL/SC 架构上可省一次额外校验、
//     更高效。
//   下面用 strong 演示一次“期望命中→成功”和一次“期望落空→失败且
//   expected 被刷新为真实当前值”。
// =====================================================================
void demo_weak_vs_strong() {
    cs::println("=========== 进阶 1：weak vs strong 语义 ===========");

    std::atomic<int> a{5};

    // TODO [进阶 1]: 用 compare_exchange_strong 演示成功与失败两条路径，
    //   并观察“失败时 expected 被刷新为当前真实值”这一关键行为。
    //   下面已是正确写法，留作进阶 1 的参考实现：

    int expected = 5;                       // 与当前值相符
    bool ok = a.compare_exchange_strong(expected, 99);
    cs::logf("[cas] strong(expected=5 → 99)：返回 ", ok,
             "，a=", a.load(), "，expected 现为 ", expected);

    expected = 5;                           // 此时 a 已是 99，期望落空
    ok = a.compare_exchange_strong(expected, 0);
    cs::logf("[cas] strong(expected=5 → 0)：返回 ", ok,
             "（应为 false），a=", a.load(),
             "，expected 被刷新为真实当前值 ", expected, "（应为 99）");

    cs::println("[note] 经验法则：放进循环重试用 weak；单次、不便重试用 strong。");
    cs::println("");
}

int main() {
    cs::println("==== E3_cas_loop：CAS 循环范式 / weak vs strong ====\n");

    demo_fetch_max();        // 必做 1：CAS 循环实现 fetch_max
    demo_fetch_multiply();   // 必做 2：CAS 循环实现 fetch_multiply
    demo_weak_vs_strong();   // 进阶 1：weak vs strong 语义

    cs::println("==== 全部演示结束。请对照文档“验收点/复盘问题”自检。 ====");
    return 0;
}
