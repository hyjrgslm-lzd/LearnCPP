// =====================================================================
// 练习 K-3：SIMD 掩码与条件运算（where / select）
//   对应文档：Concurrency_Study/14-模块K-数据并行与std-simd.md 的 练习 K-3
//
//   学习目标：
//     - 解决 SIMD 里的“if 怎么办”：向量寄存器一次处理 W 条 lane，但每条 lane 的
//       条件可能不同（有的该走 then 分支、有的该走 else）。CPU 无法对一个向量
//       “分叉跳转”，于是改用【分支无关（branch-free）】的做法：
//         1. 比较得到一个【掩码（mask）】——每条 lane 一个 bool；
//         2. 用【select（条件选择）】按掩码逐 lane 在两个候选值里挑一个。
//       本质：then 和 else 两边【都算】，再按掩码挑结果。没有真正的跳转，
//       因此没有分支预测失败，对随机条件反而比标量 if 更快。
//     - xsimd 具体 API：
//         比较 va < vb 得到 xsimd::batch_bool<float>（掩码，每 lane 一 bool）；
//         xsimd::select(mask, x, y) 逐 lane：mask 为真取 x、为假取 y。
//     - 用三种典型条件运算练手：
//         (1) abs：select(x < 0, -x, x)
//         (2) clamp 到 [lo, hi]：先 max(x,lo) 再 min(., hi)（min/max 本身就是
//             分支无关的逐 lane 运算）
//         (3) 条件赋值 relu：select(x > 0, x, 0)
//     - 用标量版逐元素对照，确认分支无关写法与朴素 if 结果完全一致。
//
//   标准目标 vs 本题回退：
//     - C++26 std::simd（<simd>, P1928R15）用 where 表达式 / std::simd::select：
//         比较得到 std::simd::basic_mask<T>（别名 mask<T,N>）；
//         where(mask, v) = expr 或 select(mask, a, b) 做条件写/选。
//       本题用 xsimd 的 batch_bool + xsimd::select 等价实现。差异映射见模块文档。
//
//   官方参考：
//     - 提案 P1928R15（std::simd，where / select / mask）：
//       https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p1928r15.pdf
//     - cppreference <simd>（std::simd::basic_mask / mask / where / select）：
//       https://en.cppreference.com/w/cpp/header/simd
//     - xsimd 文档（select / batch_bool / 比较）：
//       https://xsimd.readthedocs.io/en/latest/api/cond_index.html
//
//   编译运行（VS2026, C++20 + xsimd）：
//     cmake --build build-vs2026 --target K3_simd_where_select --config Release
// =====================================================================
#include "concurrency_study/log.hpp"

#include <xsimd/xsimd.hpp>

#include <chrono>
#include <cmath>
#include <cstddef>
#include <vector>

namespace {

constexpr float kLo = -1.0f;
constexpr float kHi = 1.0f;

// ---- 标量基准版（朴素 if / 三目）：定义“参考答案” ----
void abs_scalar(const float* x, float* out, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) out[i] = (x[i] < 0.0f) ? -x[i] : x[i];
}
void clamp_scalar(const float* x, float* out, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) {
        float v = x[i];
        if (v < kLo) v = kLo;
        if (v > kHi) v = kHi;
        out[i] = v;
    }
}
void relu_scalar(const float* x, float* out, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) out[i] = (x[i] > 0.0f) ? x[i] : 0.0f;
}

// ---- SIMD 版：用掩码 + select 实现分支无关的逐元素条件计算 ----

// abs：select(x < 0, -x, x)
void abs_simd(const float* x, float* out, std::size_t n) {
    using batch = xsimd::batch<float>;
    constexpr std::size_t W = batch::size;
    std::size_t i = 0;

    // TODO [必做 1]: 用比较得掩码、select 选值，实现分支无关 abs。
    //   参考实现已给出；理解后遮住重写。
    //
    //   关键 API：
    //     vx < zero            -> xsimd::batch_bool<float> 掩码（每 lane 一 bool）
    //     xsimd::select(m,a,b) -> 逐 lane：m 真取 a、假取 b
    //
    //   for (; i + W <= n; i += W) {
    //       batch vx = batch::load_unaligned(x + i);
    //       auto  neg = vx < batch::broadcast(0.0f);   // batch_bool 掩码
    //       batch r  = xsimd::select(neg, -vx, vx);    // 分支无关挑选
    //       r.store_unaligned(out + i);
    //   }
    for (; i + W <= n; i += W) {
        batch vx = batch::load_unaligned(x + i);
        auto neg = vx < batch::broadcast(0.0f);
        batch r = xsimd::select(neg, -vx, vx);
        r.store_unaligned(out + i);
    }
    // 尾部余数标量补齐。
    for (; i < n; ++i) out[i] = (x[i] < 0.0f) ? -x[i] : x[i];
}

// clamp 到 [lo, hi]：min/max 本身就是分支无关的逐 lane 运算。
void clamp_simd(const float* x, float* out, std::size_t n) {
    using batch = xsimd::batch<float>;
    constexpr std::size_t W = batch::size;
    const batch lo = batch::broadcast(kLo);
    const batch hi = batch::broadcast(kHi);
    std::size_t i = 0;

    // TODO [必做 2]: 用 min/max（或等价的 select）实现分支无关 clamp。
    //   参考实现用 max/min；你也可以改成两次 select 体会等价性。
    //
    //   for (; i + W <= n; i += W) {
    //       batch vx = batch::load_unaligned(x + i);
    //       batch r  = xsimd::min(xsimd::max(vx, lo), hi);
    //       r.store_unaligned(out + i);
    //   }
    for (; i + W <= n; i += W) {
        batch vx = batch::load_unaligned(x + i);
        batch r = xsimd::min(xsimd::max(vx, lo), hi);
        r.store_unaligned(out + i);
    }
    for (; i < n; ++i) {
        float v = x[i];
        if (v < kLo) v = kLo;
        if (v > kHi) v = kHi;
        out[i] = v;
    }
}

// relu（条件赋值）：select(x > 0, x, 0)
void relu_simd(const float* x, float* out, std::size_t n) {
    using batch = xsimd::batch<float>;
    constexpr std::size_t W = batch::size;
    const batch zero = batch::broadcast(0.0f);
    std::size_t i = 0;

    // TODO [必做 3]: 用掩码 + select 实现 relu = max(x,0) 的条件赋值写法。
    //   这里特意用 select 而非 max，强调“条件赋值”这一通用模式。
    //
    //   for (; i + W <= n; i += W) {
    //       batch vx  = batch::load_unaligned(x + i);
    //       auto  pos = vx > zero;                 // batch_bool 掩码
    //       batch r   = xsimd::select(pos, vx, zero);
    //       r.store_unaligned(out + i);
    //   }
    for (; i + W <= n; i += W) {
        batch vx = batch::load_unaligned(x + i);
        auto pos = vx > zero;
        batch r = xsimd::select(pos, vx, zero);
        r.store_unaligned(out + i);
    }
    for (; i < n; ++i) out[i] = (x[i] > 0.0f) ? x[i] : 0.0f;
}

// 逐元素比对两个数组是否完全一致（select/min/max 不改变数值，可严格相等）。
std::size_t count_mismatch(const float* p, const float* q, std::size_t n) {
    std::size_t m = 0;
    for (std::size_t i = 0; i < n; ++i)
        if (p[i] != q[i]) ++m;
    return m;
}

} // namespace

int main() {
    cs::println("==== K3_simd_where_select：掩码 + select —— 分支无关的条件运算 ====\n");

    using batch = xsimd::batch<float>;
    cs::logf("[info] 本平台 xsimd::batch<float>::size = ", batch::size);

    constexpr std::size_t N = 100003; // 非整数倍，触发尾部余数
    std::vector<float> x(N);
    std::vector<float> ref(N), got(N);
    for (std::size_t i = 0; i < N; ++i) {
        // 在 [-2, 2) 之间来回，正负与超界都有，充分覆盖三种条件运算的分支。
        x[i] = static_cast<float>((static_cast<long long>(i) % 41) - 20) * 0.1f;
    }

    auto ms = [](auto d) {
        return std::chrono::duration<double, std::milli>(d).count();
    };
    constexpr int kReps = 300;

    struct Case {
        const char* name;
        void (*scalar)(const float*, float*, std::size_t);
        void (*simd)(const float*, float*, std::size_t);
    };
    const Case cases[] = {
        {"abs   (select(x<0,-x,x))", &abs_scalar, &abs_simd},
        {"clamp ([-1,1] via min/max)", &clamp_scalar, &clamp_simd},
        {"relu  (select(x>0,x,0))", &relu_scalar, &relu_simd},
    };

    bool all_ok = true;
    for (const auto& cse : cases) {
        auto t0 = std::chrono::steady_clock::now();
        for (int r = 0; r < kReps; ++r) cse.scalar(x.data(), ref.data(), N);
        auto t1 = std::chrono::steady_clock::now();
        for (int r = 0; r < kReps; ++r) cse.simd(x.data(), got.data(), N);
        auto t2 = std::chrono::steady_clock::now();

        const std::size_t mm = count_mismatch(ref.data(), got.data(), N);
        all_ok = all_ok && (mm == 0);
        cs::logf("[", cse.name, "] scalar ", ms(t1 - t0), " ms | simd ", ms(t2 - t1),
                 " ms | 对照 ", (mm == 0 ? "一致 OK" : "MISMATCH"),
                 "（不一致 = ", mm, "）");
    }

    cs::logf("[result] 三种条件运算总体：", (all_ok ? "全部一致 OK" : "存在不一致 FAIL"));

    cs::println("\n要点：");
    cs::println("  - SIMD 没有逐 lane 跳转：用【比较得掩码 + select 选值】替代 if。");
    cs::println("  - then/else 两边都算再按掩码挑 -> 分支无关，无分支预测失败。");
    cs::println("  - batch_bool 对应 std::simd 的 basic_mask；select 对应 where/select。");
    cs::println("\n==== 演示结束。请对照文档“验收点/复盘问题”自检。 ====");
    return 0;
}
