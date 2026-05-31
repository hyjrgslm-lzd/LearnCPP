// =====================================================================
// 练习 L-1：并行算法与执行策略（parallel algorithms & execution policies）
//   对应文档：Concurrency_Study/15-模块L-并行算法与执行策略.md 的 练习 L-1
//
//   学习目标：
//     - 掌握 C++17 起 STL 算法的【执行策略（execution policy）】重载：把串行
//       std::for_each / std::transform / std::sort 升级为并行，只需把
//       std::execution::par 作为【第一个实参】传进去；
//     - 说清四种策略的语义（头文件 <execution>）：
//         std::execution::seq        (C++17) 串行、不向量化；
//         std::execution::par        (C++17) 多线程并行；
//         std::execution::par_unseq  (C++17) 多线程并行 + 允许向量化（SIMD）；
//         std::execution::unseq      (C++20) 单线程但允许向量化；
//     - 牢记 par_unseq / unseq 的【交错执行（interleaving）】约束：元素函数体内
//       【禁止加锁、禁止分配内存、禁止任何会与同一线程其他元素调用相互依赖的操作】，
//       否则是未定义行为；par 则允许（不同元素在不同线程上跑，但每个调用是完整的）；
//     - 工具链差异：MSVC 的并行算法【内置完整支持，无需链接任何额外库】；
//       GCC/libstdc++ 历史上把并行后端委托给 Intel TBB，需要 -ltbb 才能真正并行
//       （否则退化为串行）。本仓库 CMake 用 TbbSetup 守卫了非 MSVC 平台；
//     - 验证并行结果与串行【完全一致】（transform/sort 是确定性的），并做计时对比。
//
//   官方参考：
//     - https://en.cppreference.com/w/cpp/algorithm/execution_policy_tag_t
//     - https://en.cppreference.com/w/cpp/algorithm/for_each
//     - https://en.cppreference.com/w/cpp/algorithm/transform
//     - https://en.cppreference.com/w/cpp/algorithm/sort
//     - 《C++ Concurrency in Action, 2nd ed.》(Anthony Williams) 第 10 章
//       （并行算法 / parallel algorithms）
//
//   编译运行（VS2026, C++20；MSVC 并行算法内置，无需链接 TBB）：
//     cmake --build build-vs2026 --target L1_par_algorithms --config Release
//     ./build-vs2026/L1_par_algorithms/Release/L1_par_algorithms.exe
// =====================================================================
#include "concurrency_study/log.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <execution>
#include <numeric>
#include <random>
#include <string>
#include <vector>

// 一个小计时器：返回 lambda 执行耗时（毫秒，double）。
template <class F>
double time_ms(F&& f) {
    const auto t0 = std::chrono::steady_clock::now();
    f();
    const auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

// 每元素“有点分量”的计算：故意做几次三角/开方，放大并行收益。
// 注意：这是【纯函数】——只依赖入参、不加锁、不分配，因此 par_unseq 也安全。
inline double heavy_map(double x) {
    double y = x;
    for (int k = 0; k < 8; ++k) {
        y = std::sin(y) * std::cos(y) + std::sqrt(std::abs(y) + 1.0);
    }
    return y;
}

// =====================================================================
// 第一部分：std::transform —— 把串行映射升级为并行（结果应与串行逐字节一致）
// =====================================================================
void demo_transform() {
    cs::println("\n---- 第一部分：std::transform（seq vs par，验证结果一致 + 计时）----");

    constexpr std::size_t N = 4'000'000;
    std::vector<double> src(N);
    std::iota(src.begin(), src.end(), 1.0); // 1,2,3,...

    std::vector<double> out_seq(N);
    std::vector<double> out_par(N);

    // 串行基线：std::execution::seq（也可以省略策略实参，效果相同）。
    const double t_seq = time_ms([&] {
        std::transform(std::execution::seq, src.begin(), src.end(),
                       out_seq.begin(), heavy_map);
    });

    // TODO [必做 1]: 把上面的串行 transform 改写成【并行】版本。
    //   做法：把执行策略实参换成 std::execution::par（其余完全不变），
    //   写入 out_par。par 会把区间切块、分发到线程池并行执行 heavy_map。
    //   完成后请遮住下面参考实现重写一遍。
    //
    //   参考实现（已启用以保证可编译运行）：
    const double t_par = time_ms([&] {
        std::transform(std::execution::par, src.begin(), src.end(),
                       out_par.begin(), heavy_map);
    });

    // transform 是确定性映射：每个元素独立计算，并行不改变任何单个结果，
    // 因此并行结果应与串行【逐元素完全相等】（== 而非容差）。
    const bool same = (out_seq == out_par);
    cs::logf("[transform] N=", N, "  seq=", t_seq, "ms  par=", t_par,
             "ms  加速比=", (t_par > 0 ? t_seq / t_par : 0.0), "x");
    cs::logf("[transform] 并行结果与串行一致？ ", (same ? "是（逐元素相等）" : "否（异常！）"));
}

// =====================================================================
// 第二部分：std::for_each —— 就地并行修改（每元素独立、无共享写）
//
//   关键约束：传给 par 的可调用对象作用于【不同元素】，可能在不同线程上并发执行。
//   只要每个元素只改自己那一格、彼此不依赖，就天然无数据竞争——无需加锁。
//   这正是“尴尬并行（embarrassingly parallel）”的典型形态。
// =====================================================================
void demo_for_each() {
    cs::println("\n---- 第二部分：std::for_each（par 就地修改，每元素独立）----");

    constexpr std::size_t N = 4'000'000;
    std::vector<double> a(N), b(N);
    std::iota(a.begin(), a.end(), 1.0);
    b = a; // 复制一份给串行基线

    const double t_seq = time_ms([&] {
        std::for_each(std::execution::seq, b.begin(), b.end(),
                      [](double& v) { v = heavy_map(v); });
    });

    // 并行就地修改：每个元素只写自己那一格 -> 无共享写 -> 不必加锁。
    const double t_par = time_ms([&] {
        std::for_each(std::execution::par, a.begin(), a.end(),
                      [](double& v) { v = heavy_map(v); });
    });

    const bool same = (a == b);
    cs::logf("[for_each] N=", N, "  seq=", t_seq, "ms  par=", t_par,
             "ms  加速比=", (t_par > 0 ? t_seq / t_par : 0.0), "x");
    cs::logf("[for_each] 并行结果与串行一致？ ", (same ? "是" : "否（异常！）"));

    cs::println("  反例提醒：若在 lambda 里对【同一个】共享变量累加，就是数据竞争——");
    cs::println("            那种“归约”应改用 std::reduce（见练习 L-2），不要手动加锁。");
}

// =====================================================================
// 第三部分：std::sort —— 并行排序（结果应与串行排序完全一致）
// =====================================================================
void demo_sort() {
    cs::println("\n---- 第三部分：std::sort（par 并行排序，验证与串行一致 + 计时）----");

    constexpr std::size_t N = 4'000'000;
    std::vector<int> base(N);
    {
        std::mt19937 rng(12345);
        std::uniform_int_distribution<int> dist(0, 1'000'000'000);
        for (auto& x : base) x = dist(rng);
    }

    std::vector<int> v_seq = base;
    std::vector<int> v_par = base;

    const double t_seq = time_ms([&] {
        std::sort(std::execution::seq, v_seq.begin(), v_seq.end());
    });

    // TODO [必做 2]: 把排序改写为【并行】版本。
    //   做法：std::sort(std::execution::par, v_par.begin(), v_par.end());
    //   并行排序内部用并行归并/分块策略，最终结果与串行排序【完全相同】
    //   （排序结果唯一），因此可用 v_seq == v_par 严格校验。
    //
    //   参考实现（已启用以保证可编译运行）：
    const double t_par = time_ms([&] {
        std::sort(std::execution::par, v_par.begin(), v_par.end());
    });

    const bool same = (v_seq == v_par);
    cs::logf("[sort] N=", N, "  seq=", t_seq, "ms  par=", t_par,
             "ms  加速比=", (t_par > 0 ? t_seq / t_par : 0.0), "x");
    cs::logf("[sort] 并行结果与串行一致？ ", (same ? "是（完全相同的有序序列）" : "否（异常！）"));
}

int main() {
    cs::println("==== L1_par_algorithms：给 STL 算法加执行策略（seq/par/par_unseq/unseq）====");
    cs::println("  策略语义速记：");
    cs::println("    seq       串行、不向量化（C++17）");
    cs::println("    par       多线程并行（C++17）");
    cs::println("    par_unseq 多线程并行 + 向量化（C++17）—— 元素函数内禁止加锁/分配");
    cs::println("    unseq     单线程但向量化（C++20）—— 同样禁止加锁/分配");
    cs::println("  工具链：MSVC 并行算法内置、无需链接；GCC/libstdc++ 历史上需 -ltbb 才真并行。");

    demo_transform();
    demo_for_each();
    demo_sort();

    cs::println("\n小结：");
    cs::println("  - 给算法加执行策略只是“多传一个第一实参”，接口几乎不变；");
    cs::println("  - transform/for_each/sort 的并行结果与串行【一致】（确定性）；");
    cs::println("  - par 把工作切块分发到线程并行；par_unseq/unseq 额外允许向量化，");
    cs::println("    代价是元素函数体必须无锁、无分配、彼此不依赖（交错执行约束）。");
    cs::println("\n==== 演示结束。请对照文档“验收点/复盘问题”自检。 ====");
    return 0;
}
