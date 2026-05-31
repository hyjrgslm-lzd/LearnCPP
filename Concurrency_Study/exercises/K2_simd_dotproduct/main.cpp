// =====================================================================
// 练习 K-2：SIMD 向量化点积（乘累加 + 水平归约）
//   对应文档：Concurrency_Study/14-模块K-数据并行与std-simd.md 的 练习 K-2
//
//   学习目标：
//     - 把【点积（dot product）= Σ a[i]*b[i]】向量化。它比 K-1 的逐元素加法多
//       一个难点：结果是【一个标量】，而 SIMD 天然产出的是【一批通道值（lane）】。
//     - 学会【累加器向量化（vectorized accumulator）】：用一个 batch<float> 当
//       累加器 acc，主循环里 acc += va*vb（乘累加，FMA 的思想），相当于把
//       “第 0、W、2W… 个乘积”累进 lane0，把“第 1、W+1… 个”累进 lane1……
//       于是 W 条部分和并行累加，互不干扰。
//     - 学会【水平归约（horizontal reduction）】：主循环结束后，acc 里是 W 个
//       部分和，用 xsimd::reduce_add(acc) 把这 W 个 lane 横向加成一个标量，
//       再补上尾部余数的标量乘累加，得到最终点积。
//     - 理解【浮点累加顺序】问题：SIMD 把加法分散到 W 条 lane 再归约，求和顺序
//       与标量从左到右逐个累加【不同】；浮点加法不满足结合律，故两者结果会有
//       【微小误差】。所以正确性对照【不能用严格相等】，要用【相对容差】比较。
//
//   标准目标 vs 本题回退：
//     - C++26 std::simd（<simd>, P1928R15）提供 std::simd::reduce(v, std::plus<>{})
//       做水平归约；本题用 xsimd::reduce_add(batch) 等价实现。差异映射见模块文档。
//
//   官方参考：
//     - 提案 P1928R15（std::simd，reduce）：
//       https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p1928r15.pdf
//     - cppreference <simd>（std::simd::reduce）：
//       https://en.cppreference.com/w/cpp/header/simd
//     - xsimd 文档（reduce_add / 水平运算）：
//       https://xsimd.readthedocs.io/en/latest/api/reducer_index.html
//
//   编译运行（VS2026, C++20 + xsimd）：
//     cmake --build build-vs2026 --target K2_simd_dotproduct --config Release
// =====================================================================
#include "concurrency_study/log.hpp"

#include <xsimd/xsimd.hpp>

#include <chrono>
#include <cmath>
#include <cstddef>
#include <vector>

namespace {

// 标量基准版：从左到右逐个乘累加。这定义了“参考答案”的求和顺序。
double dot_scalar(const float* a, const float* b, std::size_t n) {
    // 用 double 累加以减小标量基准自身的舍入误差，让它更接近“真值”。
    double acc = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
        acc += static_cast<double>(a[i]) * static_cast<double>(b[i]);
    }
    return acc;
}

// SIMD 点积：累加器向量化 + 末尾水平归约 + 尾部余数。
float dot_simd(const float* a, const float* b, std::size_t n) {
    using batch = xsimd::batch<float>;
    constexpr std::size_t W = batch::size;

    // 向量累加器：W 条 lane 各自维护一路部分和，初始全 0。
    batch acc = batch::broadcast(0.0f);
    std::size_t i = 0;

    // TODO [必做 1]: 向量化乘累加主循环。
    //   每步 load 一批 a、一批 b，做 acc = acc + va*vb（W 路并行乘累加）。
    //   参考实现已给出；理解后遮住重写。
    //
    //   关键：acc 是【一个 batch】，循环结束时它含 W 个部分和，尚未合并成标量。
    //
    //   for (; i + W <= n; i += W) {
    //       batch va = batch::load_unaligned(a + i);
    //       batch vb = batch::load_unaligned(b + i);
    //       acc = acc + va * vb;        // 亦可写 acc = xsimd::fma(va, vb, acc);
    //   }
    for (; i + W <= n; i += W) {
        batch va = batch::load_unaligned(a + i);
        batch vb = batch::load_unaligned(b + i);
        acc = acc + va * vb;
    }

    // TODO [必做 2]: 水平归约（horizontal reduction）。
    //   把累加器 acc 的 W 个 lane 横向加成一个标量部分和。
    //   参考实现已给出。
    //
    //   ★ 这一步对应 std::simd 的 reduce(acc, std::plus<>{})。
    //
    //   float result = xsimd::reduce_add(acc);
    float result = xsimd::reduce_add(acc);

    // TODO [必做 3]: 补上尾部余数的标量乘累加。
    //   主循环只覆盖了前 (n - n%W) 个元素，剩下的不足一整批，标量补齐。
    //   参考实现已给出。
    //
    //   for (; i < n; ++i) result += a[i] * b[i];
    for (; i < n; ++i) {
        result += a[i] * b[i];
    }

    return result;
}

} // namespace

int main() {
    cs::println("==== K2_simd_dotproduct：向量化点积 —— 乘累加 + 水平归约 ====\n");

    using batch = xsimd::batch<float>;
    cs::logf("[info] 本平台 xsimd::batch<float>::size = ", batch::size);

    constexpr std::size_t N = 100003; // 非 batch 宽度整数倍，触发尾部余数
    std::vector<float> a(N), b(N);
    for (std::size_t i = 0; i < N; ++i) {
        a[i] = static_cast<float>((i % 17) + 1) * 0.5f;
        b[i] = static_cast<float>((i % 13) + 1) * 0.25f;
    }

    constexpr int kReps = 300;

    // ---- 标量版（基准 + 计时）----
    double dot_ref = 0.0;
    auto t0 = std::chrono::steady_clock::now();
    for (int r = 0; r < kReps; ++r) {
        dot_ref = dot_scalar(a.data(), b.data(), N);
    }
    auto t1 = std::chrono::steady_clock::now();

    // ---- SIMD 版（计时）----
    float dot_vec = 0.0f;
    for (int r = 0; r < kReps; ++r) {
        dot_vec = dot_simd(a.data(), b.data(), N);
    }
    auto t2 = std::chrono::steady_clock::now();

    // ---- 正确性对照：用【相对容差】而非严格相等 ----
    //   原因：SIMD 把求和分散到 W 条 lane 再归约，求和顺序与标量不同；
    //   浮点加法不满足结合律，结果必然有微小差异。
    const double diff = std::abs(static_cast<double>(dot_vec) - dot_ref);
    const double rel = diff / (std::abs(dot_ref) + 1e-12);
    constexpr double kTol = 1e-4; // float 点积、十万量级累加的合理相对容差
    const bool ok = rel < kTol;

    auto ms = [](auto d) {
        return std::chrono::duration<double, std::milli>(d).count();
    };
    cs::logf("[scalar] dot = ", dot_ref, "，", kReps, " 遍耗时 ", ms(t1 - t0), " ms");
    cs::logf("[simd]   dot = ", dot_vec, "，", kReps, " 遍耗时 ", ms(t2 - t1), " ms");
    cs::logf("[check]  |diff| = ", diff, "，相对误差 = ", rel,
             "（容差 ", kTol, "） -> ", (ok ? "在容差内 OK" : "超出容差 FAIL"));

    cs::println("\n要点：");
    cs::println("  - 累加器向量化：用一个 batch 当 acc，W 路部分和并行累加。");
    cs::println("  - 水平归约：reduce_add 把 W 个 lane 横向加成标量（= std::simd 的 reduce(+)）。");
    cs::println("  - 浮点累加顺序变了 -> 结果有微小误差 -> 对照要用相对容差，不能严格相等。");
    cs::println("\n==== 演示结束。请对照文档“验收点/复盘问题”自检。 ====");
    return 0;
}
