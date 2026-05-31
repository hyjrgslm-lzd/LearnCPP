// =====================================================================
// 练习 F-2：relaxed 计数器（memory_order_relaxed 的正确边界）
//   对应文档：Concurrency_Study/08-模块F-内存模型与memory_order.md 的 练习 F-2
//
//   学习目标：
//     - 论证为什么纯计数器用 memory_order_relaxed 的 fetch_add 是【正确】的：
//       计数只依赖“原子性（atomicity）”与“单变量的修改顺序
//       （modification order）”，不依赖任何跨变量的可见性顺序；
//     - 反向理解为什么用 relaxed 做“标志位发布数据”是【错误】的：
//       relaxed 不建立 synchronizes-with / happens-before，
//       两个独立变量的写在别的线程看来可以被【重排】观测到。
//
//   一句话边界：
//     relaxed 保证“这一个原子变量自己”的原子性与修改顺序，
//     但【不保证】不同变量之间的先后可见性。
//     => 适合：互不依赖的计数 / 统计；
//        不适合：用一个变量去“发布”另一片数据（那需要 release/acquire）。
//
//   官方参考：
//     - https://en.cppreference.com/w/cpp/atomic/memory_order
//       （relaxed ordering 一节；及 "modification order" 定义）
//     - 《C++ Concurrency in Action, 2nd ed.》(Anthony Williams) 第 5 章 5.3.3
//     - Mara Bos, 《Rust Atomics and Locks》第 3 章（Relaxed 一节）
//
//   编译运行（VS2026, C++20）：
//     cmake --build build-vs2026 --target F2_relaxed_counter --config Release
//     ./build-vs2026/F2_relaxed_counter/Release/F2_relaxed_counter.exe
// =====================================================================
#include "concurrency_study/log.hpp"

#include <atomic>
#include <thread>
#include <vector>

// =====================================================================
// 必做 1：多线程 relaxed fetch_add 计数，验证求和正确。
//
//   为什么 relaxed 在这里足够：
//     1) 原子性：fetch_add 是读-改-写（RMW），每次 +1 不会与别的线程的
//        +1 互相撕裂或丢失——这是 atomic 本身（任何内存序）都保证的。
//     2) 修改顺序：每个原子变量都有一条全线程一致的“修改顺序”，
//        所有对 counter 的 RMW 都串在这条顺序上，没有计数被吞掉。
//     计数器不关心“别的变量是否已可见”，所以不需要 release/acquire 的
//     跨变量同步——relaxed 既正确又最省（无多余内存屏障）。
// =====================================================================
void demo_relaxed_counter_is_correct() {
    cs::println("======== 必做 1：relaxed fetch_add 计数，求和正确 ========");

    constexpr int kThreads      = 8;
    constexpr int kIncPerThread = 100000;
    std::atomic<long long> counter{0};

    auto worker = [&] {
        for (int i = 0; i < kIncPerThread; ++i) {
            // TODO [必做 1]: 用 relaxed 的 fetch_add 自增计数器。
            //   要点：纯计数只需原子性 + 修改顺序，不需要跨变量同步，
            //   故 memory_order_relaxed 既正确又最省（无多余屏障）。
            //   下面已是正确写法，留作必做 1 的参考实现：
            counter.fetch_add(1, std::memory_order_relaxed);
        }
    };

    std::vector<std::thread> ts;
    for (int i = 0; i < kThreads; ++i) ts.emplace_back(worker);
    for (auto& t : ts) t.join();

    const long long expected = static_cast<long long>(kThreads) * kIncPerThread;
    const long long got      = counter.load(std::memory_order_relaxed);
    cs::logf("[counter] 线程数=", kThreads, " 每线程+", kIncPerThread,
             " 期望=", expected, " 实得=", got,
             "（相等? ", (got == expected), "）");
    cs::println("说明：即便用最弱的 relaxed，RMW 的原子性 + 单变量修改顺序"
                " 也保证一次自增都不丢，求和精确。");
    cs::println("");
}

// =====================================================================
// 进阶 1：relaxed 下两个标志的“重排”可被另一个线程观测到。
//
//   场景（message passing 的反例）：
//     生产线程：先 relaxed 写 data，再 relaxed 写 flag=1。
//     消费线程：relaxed 读到 flag==1 后，relaxed 读 data。
//   在 release/acquire 下，“读到 flag==1”能保证“看到 data 的写”
//   （synchronizes-with → happens-before）。
//   但在 relaxed 下，这两个写【是独立变量】，没有任何顺序保证：
//   消费线程完全可能“看到 flag==1，却仍读到 data 的旧值（0）”。
//   这正是“用 relaxed 做标志位发布数据”的错误本质。
//
//   x86 现实：x86 是强内存模型（TSO），store-store 不会被硬件重排，
//   所以在 x86 上几乎观测不到 data==0 的反例（relaxed 计数可能恒为 0）。
//   这【不代表代码正确】——换 ARM/POWER 或经编译器重排就会暴露。
//   本演示统计 0 次也属预期：请把结论建立在 happens-before 推理上，
//   而不是“我机器上没复现”。我们顺带用 acquire/release 跑一遍作为对照，
//   它在【任何】平台都能保证读到 data==1。
// =====================================================================
// 高效 litmus 跑法：常驻 producer/consumer 两个线程，用轮次票据反复跑同一个
// message-passing 体，统计“消费者看到 flag==1 却读到 data 旧值(0)”的次数。
// 不每轮新建线程（那会淹没真正想观测的重排）。
//   producer_release: data 写用 relaxed，flag 写用此参数指定的序；
//   consumer_acquire: flag 读用此参数指定的序，data 读用 relaxed。
static long long run_message_passing(int rounds,
                                     std::memory_order flag_store_order,
                                     std::memory_order flag_load_order) {
    std::atomic<int> data{0}, flag{0};
    std::atomic<int> turn{0}, done_p{0}, done_c{0}, saw{0};
    std::atomic<bool> stop{false};

    std::thread producer([&] {
        int my = 0;
        for (;;) {
            while (turn.load(std::memory_order_acquire) <= my) {
                if (stop.load(std::memory_order_acquire)) return;
            }
            ++my;
            data.store(1, std::memory_order_relaxed); // 普通数据写
            flag.store(1, flag_store_order);          // 发布点（relaxed 或 release）
            done_p.fetch_add(1, std::memory_order_release);
        }
    });
    std::thread consumer([&] {
        int my = 0;
        for (;;) {
            while (turn.load(std::memory_order_acquire) <= my) {
                if (stop.load(std::memory_order_acquire)) return;
            }
            ++my;
            while (flag.load(flag_load_order) == 0) { /* 自旋等发布 */ }
            if (data.load(std::memory_order_relaxed) == 0) {
                saw.fetch_add(1, std::memory_order_relaxed); // 看到 flag=1 却 data 旧值
            }
            done_c.fetch_add(1, std::memory_order_release);
        }
    });

    for (int i = 1; i <= rounds; ++i) {
        data.store(0, std::memory_order_relaxed);
        flag.store(0, std::memory_order_relaxed);
        turn.store(i, std::memory_order_release); // 同时放行两线程
        while (done_p.load(std::memory_order_acquire) < i ||
               done_c.load(std::memory_order_acquire) < i) { /* spin */ }
    }
    stop.store(true, std::memory_order_release);
    turn.fetch_add(1, std::memory_order_release);
    producer.join();
    consumer.join();
    return saw.load(std::memory_order_relaxed);
}

void demo_relaxed_flag_reorder() {
    cs::println("======== 进阶 1：relaxed 做标志位 —— 重排可观测（反例）========");

    constexpr int kRounds = 500000;

    // TODO [进阶 1]: flag 的写/读都用 relaxed —— 这是反面教材。
    //   要点：relaxed 不建立 synchronizes-with，data 的写与 flag 的写之间
    //   没有可见性顺序保证；消费者可能“看到 flag=1 却读到 data 旧值 0”。
    //   下面这次调用就是“错误写法”的核心（relaxed/relaxed），保留以演示：
    long long relaxed_saw_stale = run_message_passing(
        kRounds, std::memory_order_relaxed, std::memory_order_relaxed);

    // 正确对照：flag 写 release、读 acquire。读到 flag=1 即 happens-before
    // data 的写，理论上恒不会读到 data 旧值（应为 0）。
    long long acqrel_saw_stale = run_message_passing(
        kRounds, std::memory_order_release, std::memory_order_acquire);

    cs::logf("[reorder] 轮数=", kRounds,
             "  relaxed 观测到 (flag=1 且 data=0) 次数=", relaxed_saw_stale,
             "  release/acquire 同样统计=", acqrel_saw_stale);
    cs::println("解读：");
    cs::println("  - relaxed 计数可能为 0（x86 TSO 难复现重排），也可能 >0；"
                "无论如何，relaxed 用作标志位都是【无保证】的错误用法。");
    cs::println("  - release/acquire 版本在任何平台都恒为 0：读到 flag=1 即"
                " happens-before data 的写，必然看到 data=1。");
    cs::println("结论：纯计数用 relaxed；‘发布数据’必须 release/acquire。");
    cs::println("");
}

int main() {
    cs::println("==== F2_relaxed_counter：relaxed 的正确边界 ====\n");

    demo_relaxed_counter_is_correct(); // 必做 1：relaxed 计数求和正确
    demo_relaxed_flag_reorder();       // 进阶 1：relaxed 做标志位的可观测重排

    cs::println("==== 全部演示结束。请对照文档“验收点/复盘问题”自检。 ====");
    return 0;
}
