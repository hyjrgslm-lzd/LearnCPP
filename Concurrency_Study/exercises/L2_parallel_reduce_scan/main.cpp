// =====================================================================
// 练习 L-2：并行归约与扫描（parallel reduce & scan）
//   对应文档：Concurrency_Study/15-模块L-并行算法与执行策略.md 的 练习 L-2
//
//   学习目标：
//     - 掌握并行【归约（reduction）】：std::reduce（求和/聚合）、
//       std::transform_reduce（先逐元素映射再归约，可做点积/范数）；
//     - 掌握并行【扫描（scan）/前缀和（prefix sum）】：
//       std::inclusive_scan（含当前元素）、std::exclusive_scan（不含当前元素）；
//     - 说清【为什么并行归约要求二元运算满足结合律（associativity）】：
//       并行实现把区间切块、各块各算、再两两合并，括号化顺序与串行不同；
//       只有满足结合律，任意括号化结果才相同。还常要求交换律（commutativity），
//       因为块的合并顺序也不保证；
//     - 说清 std::reduce 与 std::accumulate 的区别：
//         std::accumulate 严格【从左到右】顺序累加（保证确定顺序，但天然串行）；
//         std::reduce 不保证顺序/分组，因而【可并行】，但要求运算结合（+交换）；
//     - 警惕【浮点求和的非确定性】：浮点加法【不满足结合律】（舍入误差），
//       并行 reduce 的分组顺序变化会让浮点结果【产生微小差异】——因此浮点对照
//       串行结果时必须用【容差（tolerance）】比较，而非 == 精确相等。
//
//   官方参考：
//     - https://en.cppreference.com/w/cpp/algorithm/reduce
//     - https://en.cppreference.com/w/cpp/algorithm/transform_reduce
//     - https://en.cppreference.com/w/cpp/algorithm/inclusive_scan
//     - https://en.cppreference.com/w/cpp/algorithm/exclusive_scan
//     - 《C++ Concurrency in Action, 2nd ed.》(Anthony Williams) 第 10 章
//
//   编译运行（VS2026, C++20；MSVC 并行算法内置，无需链接 TBB）：
//     cmake --build build-vs2026 --target L2_parallel_reduce_scan --config Release
//     ./build-vs2026/L2_parallel_reduce_scan/Release/L2_parallel_reduce_scan.exe
// =====================================================================
#include "concurrency_study/log.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <execution>
#include <numeric>
#include <vector>

template <class F>
double time_ms(F&& f) {
    const auto t0 = std::chrono::steady_clock::now();
    f();
    const auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

// =====================================================================
// 第一部分：std::reduce —— 并行求和（对照 std::accumulate 的串行求和）
//
//   accumulate：严格从左到右 ((((init+a0)+a1)+a2)...)，顺序确定但串行。
//   reduce：把区间切块、各块各自归约、再合并；分组/顺序不保证，故可并行，
//           代价是要求二元运算满足【结合律】（这里是 +）。
//   整型加法满足结合律 -> 整型 reduce 结果与 accumulate【完全相等】。
// =====================================================================
void demo_reduce_sum() {
    cs::println("\n---- 第一部分：std::reduce 并行求和 vs std::accumulate 串行求和 ----");

    constexpr std::size_t N = 8'000'000;
    std::vector<long long> v(N);
    std::iota(v.begin(), v.end(), 1LL); // 1..N，和 = N*(N+1)/2

    long long sum_acc = 0;
    const double t_acc = time_ms([&] {
        // 串行基线：accumulate 从左到右逐个加。
        sum_acc = std::accumulate(v.begin(), v.end(), 0LL);
    });

    // TODO [必做 1]: 用 std::reduce 做【并行】求和。
    //   做法：std::reduce(std::execution::par, v.begin(), v.end(), 0LL);
    //   它把区间切块并行归约后合并。整型 + 满足结合律 -> 结果与 accumulate 相等。
    //   （注意 init 用 0LL 保证以 long long 累加，避免溢出/类型收窄。）
    //   完成后请遮住下面参考实现重写一遍。
    //
    //   参考实现（已启用以保证可编译运行）：
    long long sum_red = 0;
    const double t_red = time_ms([&] {
        sum_red = std::reduce(std::execution::par, v.begin(), v.end(), 0LL);
    });

    const long long expect = static_cast<long long>(N) * (N + 1) / 2;
    cs::logf("[reduce] N=", N, "  accumulate=", sum_acc, "  reduce(par)=", sum_red,
             "  理论值=", expect);
    cs::logf("[reduce] 整型：reduce == accumulate ？ ",
             (sum_red == sum_acc && sum_acc == expect ? "是（整型加法满足结合律，可放心并行）"
                                                      : "否（异常！）"));
    cs::logf("[reduce] 计时：accumulate=", t_acc, "ms  reduce(par)=", t_red,
             "ms  加速比=", (t_red > 0 ? t_acc / t_red : 0.0), "x");
}

// =====================================================================
// 第二部分：std::transform_reduce —— 并行点积（dot product）
//
//   点积 = Σ a[i]*b[i]：这是“先逐元素 transform（乘），再 reduce（加）”的合体。
//   transform_reduce 一趟完成、可并行。这里用【浮点】，正好演示“浮点不满足
//   结合律 -> 并行与串行结果可能有微小差异 -> 必须用容差比较”。
// =====================================================================
void demo_transform_reduce_dot() {
    cs::println("\n---- 第二部分：std::transform_reduce 并行点积（浮点，用容差对照）----");

    constexpr std::size_t N = 8'000'000;
    std::vector<double> a(N), b(N);
    for (std::size_t i = 0; i < N; ++i) {
        a[i] = std::sin(0.001 * static_cast<double>(i));
        b[i] = std::cos(0.001 * static_cast<double>(i));
    }

    // 串行基线：std::inner_product 从左到右累加 a[i]*b[i]。
    double dot_seq = 0.0;
    const double t_seq = time_ms([&] {
        dot_seq = std::inner_product(a.begin(), a.end(), b.begin(), 0.0);
    });

    // TODO [必做 2]: 用 std::transform_reduce 做【并行】点积。
    //   做法：std::transform_reduce(std::execution::par,
    //                               a.begin(), a.end(), b.begin(), 0.0);
    //   含义：对每对 (a[i], b[i]) 先乘（默认 multiplies），再用加法（默认 plus）
    //   并行归约。init 必须给 0.0（double）。
    //   完成后请遮住下面参考实现重写一遍。
    //
    //   参考实现（已启用以保证可编译运行）：
    double dot_par = 0.0;
    const double t_par = time_ms([&] {
        dot_par = std::transform_reduce(std::execution::par,
                                        a.begin(), a.end(), b.begin(), 0.0);
    });

    // 浮点加法不满足结合律：并行分组顺序不同 -> 结果可能有微小差异。
    // 因此用相对容差比较，而不是 ==。
    const double diff = std::abs(dot_par - dot_seq);
    const double tol = 1e-6 * (std::abs(dot_seq) + 1.0);
    cs::logf("[transform_reduce] N=", N, "  inner_product(seq)=", dot_seq,
             "  transform_reduce(par)=", dot_par);
    cs::logf("[transform_reduce] |差| = ", diff, "  容差 = ", tol,
             "  在容差内？ ", (diff <= tol ? "是" : "否（差异偏大，检查实现）"));
    cs::logf("[transform_reduce] 计时：seq=", t_seq, "ms  par=", t_par,
             "ms  加速比=", (t_par > 0 ? t_seq / t_par : 0.0), "x");
    cs::println("  要点：浮点加法【不满足结合律】，并行与串行的微小差异是正常现象 ->");
    cs::println("        浮点归约对照结果【必须用容差】，绝不能要求逐位相等。");
}

// =====================================================================
// 第三部分：std::inclusive_scan / exclusive_scan —— 并行前缀和（prefix sum）
//
//   inclusive_scan：out[i] = a[0] + a[1] + ... + a[i]      （含 a[i]）
//   exclusive_scan：out[i] = init + a[0] + ... + a[i-1]    （不含 a[i]，out[0]=init）
//   并行扫描内部用 Blelloch / 两遍扫描等算法，同样要求结合律（这里是 +）。
//   整型场景下可与串行 partial_sum 逐元素严格对照。
// =====================================================================
void demo_scan() {
    cs::println("\n---- 第三部分：inclusive_scan / exclusive_scan 并行前缀和 ----");

    constexpr std::size_t N = 8'000'000;
    std::vector<long long> a(N);
    std::iota(a.begin(), a.end(), 1LL); // 1,2,3,...

    // 串行基线：std::partial_sum 即串行 inclusive scan。
    std::vector<long long> inc_seq(N);
    const double t_seq = time_ms([&] {
        std::partial_sum(a.begin(), a.end(), inc_seq.begin());
    });

    // 并行 inclusive_scan：out[i] = 前 i+1 个元素之和。
    std::vector<long long> inc_par(N);
    const double t_inc = time_ms([&] {
        std::inclusive_scan(std::execution::par, a.begin(), a.end(), inc_par.begin());
    });

    // 并行 exclusive_scan：out[i] = init + 前 i 个元素之和（out[0]=init=0）。
    std::vector<long long> exc_par(N);
    const double t_exc = time_ms([&] {
        std::exclusive_scan(std::execution::par, a.begin(), a.end(), exc_par.begin(), 0LL);
    });

    // 校验：整型扫描确定，inclusive_scan 应与 partial_sum 逐元素相等；
    // 且 inclusive[i] - exclusive[i] == a[i]（两者相差恰好当前元素）。
    const bool inc_ok = (inc_par == inc_seq);
    bool rel_ok = true;
    for (std::size_t i = 0; i < N; ++i) {
        if (inc_par[i] - exc_par[i] != a[i]) { rel_ok = false; break; }
    }
    cs::logf("[scan] N=", N, "  inclusive(par) 末元素=", inc_par.back(),
             "  理论值=", static_cast<long long>(N) * (N + 1) / 2);
    cs::logf("[scan] inclusive(par) == partial_sum(seq) ？ ",
             (inc_ok ? "是" : "否（异常！）"));
    cs::logf("[scan] inclusive[i] - exclusive[i] == a[i] 处处成立？ ",
             (rel_ok ? "是" : "否（异常！）"));
    cs::logf("[scan] 计时：partial_sum(seq)=", t_seq, "ms  inclusive(par)=", t_inc,
             "ms  exclusive(par)=", t_exc, "ms");
}

int main() {
    cs::println("==== L2_parallel_reduce_scan：并行归约 reduce / transform_reduce 与扫描 scan ====");
    cs::println("  核心规则：并行归约/扫描要求二元运算满足【结合律】（常还需交换律），");
    cs::println("            因为并行实现会切块、各算、再合并，括号化与顺序均不保证。");
    cs::println("  reduce vs accumulate：accumulate 从左到右串行确定；reduce 不保证顺序、可并行。");
    cs::println("  浮点警告：浮点加法不满足结合律 -> 并行结果可能微变 -> 对照必须用容差。");

    demo_reduce_sum();
    demo_transform_reduce_dot();
    demo_scan();

    cs::println("\n小结：");
    cs::println("  - std::reduce：并行求和/聚合，要求结合(+交换)；整型与 accumulate 相等。");
    cs::println("  - std::transform_reduce：map+reduce 合体，做点积/范数最顺手。");
    cs::println("  - inclusive_scan/exclusive_scan：并行前缀和，差一个“是否含当前元素”。");
    cs::println("  - 浮点归约对照串行务必用容差；要严格确定顺序请退回 accumulate(串行)。");
    cs::println("\n==== 演示结束。请对照文档“验收点/复盘问题”自检。 ====");
    return 0;
}
