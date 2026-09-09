#include <stdexec/execution.hpp>
#include <execution>
#include <numeric>
#include <vector>
#include <iostream>
#include <span>
#include <cstdint>

namespace ex = stdexec;

int main() {
    // ── 数据准备 ──────────────────────────────────────────
    constexpr int N = 1'000'000;
    std::vector<int64_t> data(N);
    std::iota(data.begin(), data.end(), 1);  // 1, 2, ..., 1'000'000

    // 辅助 lambda：对子区间求"偶数平方和"
    auto is_even_square_sum = [](std::span<const int64_t> rng) -> int64_t {
        int64_t sum = 0;
        for (auto v : rng) {
            if (v % 2 == 0) {
                sum += v * v;
            }
        }
        return sum;
    };

    // ══════════════════════════════════════════════════════
    // 版本 A：std::execution 并行算法
    // ══════════════════════════════════════════════════════
    int64_t result_a = 0;

    // TODO [必做]: 使用 std::transform_reduce(std::execution::par, ...)
    //   计算 data 中所有偶数的平方和。
    //   提示：transform_reduce 的 unary_op 对每个元素做变换，
    //         binary_op 做归约。偶数平方返回 v*v，奇数返回 0。
    // result_a = std::transform_reduce(
    //     std::execution::par,
    //     data.begin(), data.end(),
    //     int64_t{0},
    //     /* binary_op  */ ???,
    //     /* unary_op   */ ???
    // );

    std::cout << "[版本 A] 偶数平方和 = " << result_a << "\n";

    // ══════════════════════════════════════════════════════
    // 版本 B：sender 图 —— 手动 4 分支 + when_all
    // ══════════════════════════════════════════════════════
    const size_t chunk = data.size() / 4;

    std::span<const int64_t> s0(data.data(),             chunk);
    std::span<const int64_t> s1(data.data() + chunk,     chunk);
    std::span<const int64_t> s2(data.data() + chunk * 2, chunk);
    std::span<const int64_t> s3(data.data() + chunk * 2, data.size() - chunk * 3);

    // TODO [必做]: 为每段构造一个 sender 分支
    //   每个分支形如：ex::just(span) | ex::then(is_even_square_sum)
    //   然后用 ex::when_all(...) 汇合 4 个分支，
    //   再用 ex::then([](int64_t a, int64_t b, int64_t c, int64_t d){ return a+b+c+d; })
    //   最后用 ex::sync_wait(...) 取出结果。

    // auto branch0 = ex::just(s0) | ex::then(is_even_square_sum);
    // auto branch1 = ???;
    // auto branch2 = ???;
    // auto branch3 = ???;
    //
    // auto merged = ex::when_all(
    //     std::move(branch0),
    //     std::move(branch1),
    //     std::move(branch2),
    //     std::move(branch3)
    // ) | ex::then([](int64_t a, int64_t b, int64_t c, int64_t d) {
    //     return a + b + c + d;
    // });
    //
    // auto [result_b] = ex::sync_wait(std::move(merged)).value();

    int64_t result_b = 0;  // 替换为 sync_wait 得到的值

    std::cout << "[版本 B] 偶数平方和 = " << result_b << "\n";

    // ── 验证 ──────────────────────────────────────────────
    if (result_a == result_b && result_a != 0) {
        std::cout << "[OK] 两个版本结果一致: " << result_a << "\n";
    } else {
        std::cout << "[MISMATCH] A=" << result_a << "  B=" << result_b << "\n";
    }

    // ══════════════════════════════════════════════════════
    // TODO [进阶]: 把每段返回值从 int64_t 升级为结构体
    //   struct PartialResult { int64_t partial_sum; int64_t even_count; };
    //   在最终汇合阶段同时产出总和与总计数。
    //   尝试把 sender 版本拆成"构图函数"和"消费函数"两层。
    // ══════════════════════════════════════════════════════

    return 0;
}
