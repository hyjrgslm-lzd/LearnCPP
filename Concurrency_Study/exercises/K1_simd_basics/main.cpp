// =====================================================================
// 练习 K-1：std::simd 基础（xsimd 回退）
//   对应文档：Concurrency_Study/14-模块K-数据并行与std-simd.md 的 练习 K-1
//
//   学习目标：
//     - 建立【数据并行（data parallelism）/ SIMD（Single Instruction,
//       Multiple Data，单指令多数据）】的心智模型：一条向量指令同时对一“批”
//       （batch / lane，通道）相邻数据做同样的算术。这与本课程前面的【任务并行
//       （task parallelism，多线程各干各的）】是正交的两个维度——SIMD 是在
//       【一个线程内部】把吞吐放大若干倍。
//     - 用 xsimd 的 batch<float> 把“两数组逐元素相加”向量化：以
//       batch<float>::size 个元素为一步，load_unaligned 取一批、相加、
//       store_unaligned 写回。
//     - 学会处理【尾部余数（remainder / tail）】：数组长度通常不是 batch 宽度
//       的整数倍，最后不足一整批的元素必须用标量循环补齐——这是所有 SIMD 代码
//       的必修课，漏掉就会越界或漏算。
//     - 用标量版结果做【正确性对照】，并粗略计时感受 SIMD 的加速。
//
//   标准目标 vs 本题回退：
//     - 标准目标是 C++26 的 std::simd（<simd>, 提案 P1928R15，命名空间
//       std::simd，主模板 basic_vec<T,Abi> / basic_mask<T,Abi>，别名
//       vec<T,N> / mask<T,N>）。截至 2026-05，MSVC（含 VS2026）尚未实现。
//     - 因此本题用 header-only 且支持 MSVC 的 xsimd 作回退：batch<T> 对应
//       std::simd 的 basic_vec<T>，load/store/算术运算概念一一对应。
//       xsimd <-> std::simd 的完整差异映射见模块文档的对照表。
//
//   官方参考：
//     - 提案 P1928R15（std::simd）：
//       https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p1928r15.pdf
//     - cppreference <simd>（std::simd / basic_vec / vec / basic_mask / mask）：
//       https://en.cppreference.com/w/cpp/header/simd
//     - xsimd 文档（basic usage / batch）：
//       https://xsimd.readthedocs.io/en/latest/
//
//   编译运行（VS2026, C++20 + xsimd）：
//     cmake --build build-vs2026 --target K1_simd_basics --config Release
// =====================================================================
#include "concurrency_study/log.hpp"

#include <xsimd/xsimd.hpp>

#include <chrono>
#include <cmath>
#include <cstddef>
#include <vector>

namespace {

// 标量基准版：逐元素 c[i] = a[i] + b[i]。用来对照 SIMD 结果是否一致。
void add_scalar(const float* a, const float* b, float* c, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) {
        c[i] = a[i] + b[i];
    }
}

// SIMD 向量加法：用 xsimd::batch<float> 一次处理 batch::size 个元素。
//   ★ 本题核心。注意两件事：
//     (1) 主循环以一整批（batch::size 个）为步长前进；
//     (2) 末尾不足一整批的【余数（remainder）】用标量循环补齐。
void add_simd(const float* a, const float* b, float* c, std::size_t n) {
    using batch = xsimd::batch<float>;
    constexpr std::size_t W = batch::size; // 该平台一个 batch 容纳的 float 数（如 SSE=4, AVX=8）

    std::size_t i = 0;

    // TODO [必做 1]: 向量化主循环。
    //   以 W 为步长，每步 load 一批 a、一批 b，相加，store 回 c。
    //   参考实现已给出以保证可编译运行；理解后请遮住自己写一遍。
    //
    //   关键 API：
    //     batch::load_unaligned(ptr)  从 ptr 起读 W 个 float 进一个 batch
    //     va + vb                     逐通道相加（一条向量指令）
    //     vc.store_unaligned(ptr)     把 batch 的 W 个结果写回 ptr
    //
    //   循环上界用 n - n % W（即 n 向下取整到 W 的倍数），保证 load/store 不越界。
    //
    //   for (; i + W <= n; i += W) {
    //       batch va = batch::load_unaligned(a + i);
    //       batch vb = batch::load_unaligned(b + i);
    //       batch vc = va + vb;
    //       vc.store_unaligned(c + i);
    //   }
    for (; i + W <= n; i += W) {
        batch va = batch::load_unaligned(a + i);
        batch vb = batch::load_unaligned(b + i);
        batch vc = va + vb;
        vc.store_unaligned(c + i);
    }

    // TODO [必做 2]: 处理尾部余数（remainder / tail）。
    //   主循环只覆盖了前 (n - n%W) 个元素，剩下 n%W 个不足一整批，
    //   必须用标量循环补齐——这是 SIMD 代码最容易漏的一步，漏了就漏算/越界。
    //   参考实现已给出。
    //
    //   for (; i < n; ++i) c[i] = a[i] + b[i];
    for (; i < n; ++i) {
        c[i] = a[i] + b[i];
    }
}

} // namespace

int main() {
    cs::println("==== K1_simd_basics：SIMD 数据并行模型 —— 向量化数组加法 ====\n");

    using batch = xsimd::batch<float>;
    cs::logf("[info] 本平台 xsimd::batch<float>::size = ", batch::size,
             "（一条向量指令同时处理这么多个 float）");

    // 故意取一个【不是 batch 宽度整数倍】的长度，强制触发尾部余数处理。
    constexpr std::size_t N = 100003;
    std::vector<float> a(N), b(N), c_scalar(N), c_simd(N);
    for (std::size_t i = 0; i < N; ++i) {
        a[i] = static_cast<float>(i) * 0.5f;
        b[i] = static_cast<float>(i) * 0.25f + 1.0f;
    }

    // ---- 标量版（基准 + 计时）----
    auto t0 = std::chrono::steady_clock::now();
    constexpr int kReps = 200; // 多跑几遍让计时更稳定
    for (int r = 0; r < kReps; ++r) {
        add_scalar(a.data(), b.data(), c_scalar.data(), N);
    }
    auto t1 = std::chrono::steady_clock::now();

    // ---- SIMD 版（计时）----
    for (int r = 0; r < kReps; ++r) {
        add_simd(a.data(), b.data(), c_simd.data(), N);
    }
    auto t2 = std::chrono::steady_clock::now();

    // ---- 正确性对照：逐元素比较标量结果与 SIMD 结果 ----
    // 加法没有浮点累加顺序问题，可以要求严格相等（同样的输入、同样的 + 运算）。
    std::size_t mismatch = 0;
    for (std::size_t i = 0; i < N; ++i) {
        if (c_scalar[i] != c_simd[i]) ++mismatch;
    }

    auto ms = [](auto d) {
        return std::chrono::duration<double, std::milli>(d).count();
    };
    cs::logf("[scalar] ", kReps, " 遍耗时 ", ms(t1 - t0), " ms");
    cs::logf("[simd]   ", kReps, " 遍耗时 ", ms(t2 - t1), " ms");
    cs::logf("[check]  逐元素对照：", (mismatch == 0 ? "全部一致 OK" : "出现不一致 MISMATCH"),
             "（不一致元素数 = ", mismatch, "）");
    cs::logf("[note]   N = ", N, "，N % size = ", N % batch::size,
             " -> 尾部余数已由标量循环补齐。");

    cs::println("\n要点：");
    cs::println("  - SIMD 是【线程内】的数据并行：一条指令同时算一批 lane。");
    cs::println("  - 主循环步长 = batch::size；不足一整批的尾部必须标量补齐。");
    cs::println("  - std::simd（C++26, P1928）是标准目标；xsimd 是当前 MSVC 可编译回退。");
    cs::println("\n==== 演示结束。请对照文档“验收点/复盘问题”自检。 ====");
    return 0;
}
