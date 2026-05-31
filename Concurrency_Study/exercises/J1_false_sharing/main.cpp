// =====================================================================
// 练习 J-1：伪共享实测（false sharing benchmark）
//   对应文档：Concurrency_Study/13-模块J-缓存与伪共享.md 的 练习 J-1
//
//   学习目标：
//     - 理解缓存行（cache line，典型 64 字节）是缓存一致性协议
//       （cache coherence protocol）的【最小搬运单位】：CPU 永远以
//       整条缓存行为粒度在核心之间传输数据，而非单个字节/单个变量；
//     - 亲手测出【伪共享（false sharing）】带来的性能下降：两个线程
//       各写各自的计数器，逻辑上互不相干，但因两个计数器恰好落在
//       【同一条缓存行】，每次写都会让对方核心持有的该行失效
//       （invalidate），缓存行在两个核心间反复“弹跳”
//       （cache line bouncing），吞吐暴跌；
//     - 学会用 alignas(std::hardware_destructive_interference_size)
//       （或保守的 64 字节）把两个计数器【强制分到不同缓存行】，
//       消除伪共享，再测一次，对比加速比（speedup）；
//     - 建立一条硬直觉：多线程频繁写的变量，若被不同线程触碰，
//       务必让它们各占一条缓存行——这是无锁/并发代码的常见性能陷阱。
//
//   官方参考：
//     - https://en.cppreference.com/w/cpp/thread/hardware_destructive_interference_size
//     - 《C++ Concurrency in Action, 2nd ed.》(Anthony Williams) 第 8 章
//       （8.2.3 节：伪共享 false sharing；数据布局对性能的影响）
//     - Ulrich Drepper, "What Every Programmer Should Know About Memory"
//       （第 6 节：缓存优化 / 伪共享）
//     - Intel 64 and IA-32 Architectures Optimization Reference Manual
//       （缓存行与伪共享章节）；Agner Fog, "Optimizing software in C++"
//
//   编译运行（VS2026, C++20, 建议 Release，否则差距可能被优化掩盖/放大）：
//     cmake --build build-vs2026 --target J1_false_sharing --config Release
// =====================================================================
#include "concurrency_study/log.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <new>      // std::hardware_destructive_interference_size（C++17）
#include <thread>

// ---------------------------------------------------------------------
// 关于 std::hardware_destructive_interference_size（C++17，<new>）：
//   它是“为避免伪共享，两个对象至少应相距多少字节”的实现定义（implementation
//   -defined）常量，标准只保证它 >= alignof(std::max_align_t)。x86-64 上
//   MSVC/常见实现给 64（即一条缓存行）。
//
//   注意（GCC 特有，本套在 MSVC 上不会遇到）：GCC 对“在 ABI 边界上使用该常量”
//   会给 -Winterference-size 告警，因为该值可能随编译目标变化、影响布局兼容性。
//   MSVC 无此告警。为让本练习在任意实现下都稳妥，下面用一个保守回退：
//   若实现未提供该常量则退回 64。
// ---------------------------------------------------------------------
#ifdef __cpp_lib_hardware_interference_size
constexpr std::size_t kCacheLine = std::hardware_destructive_interference_size;
#else
constexpr std::size_t kCacheLine = 64; // 保守回退：x86-64 典型缓存行大小。
#endif

// 每个线程对自己的计数器执行的写次数。调大可放大伪共享的影响，
// 也可让计时更稳定（减少噪声占比）。
constexpr std::int64_t kIters = 200'000'000;

// =====================================================================
// 版本一：伪共享版（BAD）——两个计数器【紧挨着】，极可能落在同一缓存行
//
//   两个 std::atomic<int64_t> 各 8 字节，相邻放置时几乎必定共享同一条
//   64 字节缓存行。线程 0 只写 a、线程 1 只写 b，逻辑上零冲突；但硬件层面
//   每次写都会抢占整条缓存行的所有权（MESI 协议的 M 态），让对方核心的
//   该行失效——这就是伪共享。
// =====================================================================
struct CountersShared {
    std::atomic<std::int64_t> a{0};
    std::atomic<std::int64_t> b{0};
    // 二者相邻，sizeof 通常仅 16 字节 —— 同一缓存行，伪共享在此发生。
};

// =====================================================================
// 版本二：对齐消除版（GOOD）——用 alignas 把两个计数器各推到独立缓存行
//
//   对每个计数器套一层 alignas(kCacheLine) 的包装，使其【起始地址按缓存行
//   对齐】，且整体大小被填充（padding）到至少一条缓存行。于是 a、b 必然分属
//   不同缓存行，一个核心写 a 不再波及另一核心持有的 b 所在行 —— 伪共享消除。
// =====================================================================
struct alignas(kCacheLine) PaddedCounter {
    std::atomic<std::int64_t> v{0};
    // alignas(kCacheLine) 同时保证：(1) 本对象起始地址按缓存行对齐；
    // (2) sizeof(PaddedCounter) 被向上取整到 kCacheLine 的整数倍，
    //     从而数组/相邻两个 PaddedCounter 不会落进同一行。
};

struct CountersPadded {
    PaddedCounter a;
    PaddedCounter b; // a、b 各占独立缓存行。
};

// 热循环：对给定的 atomic 计数器做 n 次自增。用 relaxed 内存序——本题只关心
// “写同一缓存行的竞争代价”，不需要任何跨线程的顺序保证（避免内存序开销污染对比）。
inline void hammer(std::atomic<std::int64_t>& counter, std::int64_t n) {
    for (std::int64_t i = 0; i < n; ++i) {
        counter.fetch_add(1, std::memory_order_relaxed);
    }
}

// 跑“两个线程各锤各的计数器”，返回墙钟耗时（毫秒）。
template <class Fn>
double run_two_threads(Fn&& body) {
    using clock = std::chrono::steady_clock;
    const auto t0 = clock::now();
    body();
    const auto t1 = clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

int main() {
    cs::println("==== J1_false_sharing：伪共享实测 ====");
    cs::logf("每线程写次数 kIters = ", kIters,
             "，缓存行 kCacheLine = ", kCacheLine, " 字节。");
    cs::println("提示：请用 Release 构建；Debug 下原子操作开销会掩盖伪共享差距。\n");

    // -----------------------------------------------------------------
    // TODO [必做 1]: 测“伪共享版”耗时。
    //   构造一个 CountersShared（两个计数器相邻、同一缓存行），开两个线程：
    //   线程 0 调 hammer(c.a, kIters)，线程 1 调 hammer(c.b, kIters)，join。
    //   用 run_two_threads 计时，记到 ms_shared。
    //
    //   要点：两个 atomic 相邻 → 同一缓存行 → 两核心反复争夺该行所有权
    //         （cache line bouncing）→ 慢。
    //
    //   参考实现（已启用以保证可编译运行）：
    // -----------------------------------------------------------------
    double ms_shared = 0.0;
    {
        CountersShared c;
        ms_shared = run_two_threads([&c] {
            std::thread t0(hammer, std::ref(c.a), kIters);
            std::thread t1(hammer, std::ref(c.b), kIters);
            t0.join();
            t1.join();
        });
        // 防止整段循环被优化掉：读一下结果。
        cs::logf("[伪共享版] a=", c.a.load(), " b=", c.b.load(),
                 " 耗时=", ms_shared, " ms");
    }

    // -----------------------------------------------------------------
    // TODO [必做 2]: 测“对齐消除版”耗时。
    //   构造一个 CountersPadded（两个计数器各占独立缓存行），同样开两个线程
    //   分别锤 c.a.v 与 c.b.v，join，计时记到 ms_padded。
    //
    //   要点：alignas(kCacheLine) 把 a、b 分到不同缓存行 → 两核心各写各的行、
    //         互不失效 → 快。预期 ms_padded 明显小于 ms_shared。
    //
    //   参考实现（已启用以保证可编译运行）：
    // -----------------------------------------------------------------
    double ms_padded = 0.0;
    {
        CountersPadded c;
        ms_padded = run_two_threads([&c] {
            std::thread t0(hammer, std::ref(c.a.v), kIters);
            std::thread t1(hammer, std::ref(c.b.v), kIters);
            t0.join();
            t1.join();
        });
        cs::logf("[对齐版]   a=", c.a.v.load(), " b=", c.b.v.load(),
                 " 耗时=", ms_padded, " ms");
    }

    // -----------------------------------------------------------------
    // 对比汇总：打印两者耗时与加速比。
    // -----------------------------------------------------------------
    cs::println("\n---- 对比 ----");
    cs::logf("伪共享版（相邻、同一缓存行）: ", ms_shared, " ms");
    cs::logf("对齐版  （各占一条缓存行）  : ", ms_padded, " ms");
    if (ms_padded > 0.0) {
        const double speedup = ms_shared / ms_padded;
        cs::logf("加速比 speedup = 伪共享 / 对齐 = ", speedup, " 倍");
        cs::println("  speedup 明显 > 1（常见 2~8 倍，随机器而异）即证明伪共享真实存在。");
        cs::println("  若约等于 1：可能两计数器恰巧已不同行 / 机器为单核 / 被优化器改写——");
        cs::println("  请确认 Release 构建并适当加大 kIters。");
    }

    // -----------------------------------------------------------------
    // 打印结构体布局，直观看到“相邻 vs 对齐”的尺寸差异。
    // -----------------------------------------------------------------
    cs::println("\n---- 内存布局（sizeof / alignof）----");
    cs::logf("sizeof(CountersShared) = ", sizeof(CountersShared),
             " 字节（两个计数器挤在一起，多半同一缓存行）");
    cs::logf("sizeof(PaddedCounter)  = ", sizeof(PaddedCounter),
             " 字节，alignof = ", alignof(PaddedCounter),
             "（被填充/对齐到一条缓存行）");
    cs::logf("sizeof(CountersPadded) = ", sizeof(CountersPadded),
             " 字节（两个计数器各占独立缓存行）");

    // TODO [进阶 1]: 关掉原子、改用普通 int64_t + 仅一个线程写一个变量，
    //   你会发现伪共享对“普通写”同样成立（不限于原子）。把 atomic 换成裸
    //   int64_t 重测，体会“伪共享是缓存一致性层面的现象，与是否原子无关”。
    //   提示：裸写要防优化器把循环优化没，可用 volatile 或把结果累加打印。
    //
    // TODO [进阶 2]: 把线程数加到机器核数，用一个 PaddedCounter 数组（每核一个），
    //   对比“数组元素未对齐（相邻打包）”与“每元素 alignas 缓存行”两种布局的
    //   总吞吐。这正是并发计数器/分片统计（sharded counter）的标准做法。

    cs::println("\n==== 演示结束。请对照文档“验收点/复盘问题”自检。 ====");
    return 0;
}
