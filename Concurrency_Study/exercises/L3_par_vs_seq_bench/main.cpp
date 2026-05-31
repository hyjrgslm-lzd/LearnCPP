// =====================================================================
// 练习 L-3：并行 vs 串行基准（when does parallelism actually pay off?）
//   对应文档：Concurrency_Study/15-模块L-并行算法与执行策略.md 的 练习 L-3
//
//   学习目标：
//     - 系统对比 seq / par / par_unseq 三种执行策略在【不同数据规模】下的耗时，
//       打印一张“规模 × 策略 × 加速比”表；
//     - 用实测证据说清【何时并行才真正加速】：
//         * 数据量太小：线程启动/任务分发/同步的【固定开销】比计算本身还贵，
//           并行【更慢】（本题专门用小规模复现“小数据并行更慢”）；
//         * 每元素工作量太轻（如纯加法）：计算受【内存带宽（memory bandwidth）】
//           而非 CPU 限制，多核抢同一条内存总线，并行收益有限甚至为负；
//         * 数据量大 + 每元素计算重（compute-bound）：并行才接近线性加速；
//     - 体会“并行不是免费午餐”：要权衡【数据规模、每元素工作量、内存带宽瓶颈、
//       并行开销（线程创建/调度/归约合并）】。
//
//   方法学说明（为什么这样测）：
//     - 每个配置多跑几次取【最小值】（min）：最小值最接近“无被抢占干扰”的真实耗时，
//       比平均值更稳定（平均值会被偶发的系统抖动拉高）；
//     - 跑前做一次 warm-up（预热）：让线程池/页缓存就绪，避免首次调用的冷启动污染；
//     - 用 volatile sink 吸收结果，防止编译器把“结果没人用”的计算整段优化掉。
//
//   官方参考：
//     - https://en.cppreference.com/w/cpp/algorithm/execution_policy_tag_t
//     - https://en.cppreference.com/w/cpp/algorithm/transform
//     - 《C++ Concurrency in Action, 2nd ed.》(Anthony Williams) 第 10 章
//       （并行算法的性能与适用条件）
//
//   编译运行（VS2026, C++20；MSVC 并行算法内置，无需链接 TBB）：
//     cmake --build build-vs2026 --target L3_par_vs_seq_bench --config Release
//     ./build-vs2026/L3_par_vs_seq_bench/Release/L3_par_vs_seq_bench.exe
//
//   注意：基准结果【高度依赖机器】（核数、缓存、内存带宽、是否 Release/优化）。
//         Debug 构建下并行往往“更慢”，请务必在 Release/优化下观察趋势。
// =====================================================================
#include "concurrency_study/log.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <execution>
#include <numeric>
#include <string>
#include <vector>

// 跑 f() reps 次，返回最小耗时（毫秒）。最小值最接近无干扰真实耗时。
template <class F>
double bench_min_ms(F&& f, int reps) {
    double best = 1e300;
    for (int r = 0; r < reps; ++r) {
        const auto t0 = std::chrono::steady_clock::now();
        f();
        const auto t1 = std::chrono::steady_clock::now();
        const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        best = std::min(best, ms);
    }
    return best;
}

// 防优化用的全局 sink：把结果累进去，编译器就不能丢掉计算。
volatile double g_sink = 0.0;

// 较重的每元素计算（compute-bound）：足够重，才能让并行体现优势。
inline double heavy(double x) {
    double y = x;
    for (int k = 0; k < 16; ++k) {
        y = std::sin(y) * std::cos(y) + std::sqrt(std::abs(y) + 1.0);
    }
    return y;
}

// 对给定规模 N，分别用 seq / par / par_unseq 跑一遍 transform(heavy)，
// 打印三者耗时与相对 seq 的加速比。
void run_scale(std::size_t N, int reps) {
    std::vector<double> src(N);
    std::iota(src.begin(), src.end(), 1.0);
    std::vector<double> dst(N);

    auto run_with = [&](auto policy) {
        std::transform(policy, src.begin(), src.end(), dst.begin(), heavy);
        // 取一个结果喂给 sink，防止整段被优化掉。
        g_sink += dst.empty() ? 0.0 : dst[N / 2];
    };

    // 预热：让线程池/页缓存就绪，不计入计时。
    run_with(std::execution::seq);

    const double t_seq = bench_min_ms([&] { run_with(std::execution::seq); }, reps);

    // TODO [必做 1]: 补上 par 与 par_unseq 两种策略的测量。
    //   做法：分别用 std::execution::par 和 std::execution::par_unseq 调 run_with，
    //   各自用 bench_min_ms 取最小耗时。它们与 t_seq 比即得加速比。
    //   注意：heavy() 是纯函数（无锁、无分配、不依赖相邻元素），所以 par_unseq
    //   的“交错执行”约束在这里是满足的，可以安全使用。
    //   完成后请遮住下面参考实现重写一遍。
    //
    //   参考实现（已启用以保证可编译运行）：
    const double t_par = bench_min_ms([&] { run_with(std::execution::par); }, reps);
    const double t_pu  = bench_min_ms([&] { run_with(std::execution::par_unseq); }, reps);

    const double sp_par = (t_par > 0 ? t_seq / t_par : 0.0);
    const double sp_pu  = (t_pu  > 0 ? t_seq / t_pu  : 0.0);

    // 对齐成一行表格。加速比 < 1 说明并行【更慢】。
    auto verdict = [](double sp) -> const char* {
        return sp >= 1.0 ? "  (并行更快)" : "  (并行更慢!)";
    };
    cs::logf("N=", N,
             "  seq=", t_seq, "ms",
             "  par=", t_par, "ms (x", sp_par, ")",
             "  par_unseq=", t_pu, "ms (x", sp_pu, ")",
             verdict(sp_par));
}

int main() {
    cs::println("==== L3_par_vs_seq_bench：seq vs par vs par_unseq 的规模扫描基准 ====");
    cs::println("  目标：用实测证据说清“何时并行才真加速”。关注【加速比随规模的变化趋势】。");
    cs::println("  方法：每配置多跑取最小值、先预热、volatile sink 防优化。");
    cs::println("  提醒：结果依赖机器/核数/内存带宽，且必须在 Release/优化下观察。");
    cs::println("");

    // 从“很小”扫到“很大”。小规模一端用来复现“并行更慢”（开销 > 收益）；
    // 大规模一端展示并行接近线性加速（compute-bound）。
    const std::vector<std::size_t> scales = {
        1'000,        // 极小：线程/分发开销远大于计算 -> 预期并行更慢
        10'000,       // 小
        100'000,      // 中
        1'000'000,    // 大
        8'000'000,    // 很大：预期并行明显加速
    };

    cs::println("---- 计算密集型（每元素 heavy()，compute-bound）规模扫描 ----");
    for (std::size_t N : scales) {
        // 规模越小跑越多次，让最小值更稳定。
        const int reps = (N <= 10'000) ? 50 : (N <= 1'000'000 ? 10 : 5);
        run_scale(N, reps);
    }

    cs::logf("[防优化] g_sink = ", static_cast<double>(g_sink), "（仅用于阻止编译器优化掉计算）");

    cs::println("\n如何解读这张表：");
    cs::println("  - 看【小规模行】：par/par_unseq 的加速比通常 < 1（并行更慢）——");
    cs::println("    这就是“小数据并行更慢”的实测证据：线程创建/任务分发/归约合并");
    cs::println("    的固定开销，比这点计算量本身还贵，并行得不偿失。");
    cs::println("  - 看【大规模行】：加速比通常 > 1，随核数趋近线性——计算量大到足以");
    cs::println("    摊薄并行开销，多核才真正帮上忙。");
    cs::println("  - 何时并行才真加速：数据量足够大 且 每元素工作量足够重（compute-bound）；");
    cs::println("    若每元素极轻（如纯加法），会撞上【内存带宽瓶颈】，多核抢总线，收益有限。");
    cs::println("  - 经验法则：先测后用。默认串行；profiling 证明“规模大 + 计算重”再上 par。");
    cs::println("\n==== 演示结束。请对照文档“验收点/复盘问题”自检。 ====");
    return 0;
}
