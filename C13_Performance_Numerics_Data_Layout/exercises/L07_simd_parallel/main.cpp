#include "concurrency_study/benchmark.hpp"
#include "concurrency_study/numeric_kernels.hpp"
#include "concurrency_study/simd_kernels.hpp"
#include <array>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {
double dot_oracle(std::size_t n) {
    const auto groups = n / 17;
    const auto remainder = n % 17;
    return static_cast<double>(groups) * 68.0
        + static_cast<double>(remainder) * static_cast<double>(remainder == 0 ? 0 : remainder - 1) / 4.0;
}

void verify_add(std::span<const float> out) {
    for (std::size_t i = 0; i < out.size(); ++i)
        cs::check(out[i] == static_cast<float>(i % 17) + 0.5f, "SIMD add analytic oracle");
}

void verify_map(std::span<const float> out) {
    for (std::size_t i = 0; i < out.size(); ++i) {
        const float x = static_cast<float>(static_cast<int>(i % 33) - 16);
        cs::check(out[i] == x * x + 2.0f, "execution map analytic oracle");
    }
}
} // namespace

int main(int argc, char** argv) try {
    cs::bench::arguments args(argc, argv);
    const auto n = args.number("--size", 257);
    args.finish();
    cs::check(n <= 4'000'000, "size cap");

    std::vector<float> a(n), b(n, 0.5f), out(n);
    for (std::size_t i = 0; i < n; ++i) a[i] = static_cast<float>(i % 17);

    const std::array<std::pair<std::string_view, cs::numeric::backend>, 4> backends{{
        {"scalar", cs::numeric::backend::scalar},
        {"sse2", cs::numeric::backend::sse2},
        {"xsimd", cs::numeric::backend::xsimd},
        {"std_simd", cs::numeric::backend::std_simd},
    }};

    for (const auto [name, backend] : backends) {
        if (!cs::numeric::available(backend)) {
            std::cerr << "SKIP " << name << ": backend unavailable\n";
            continue;
        }
        std::fill(out.begin(),out.end(),std::numeric_limits<float>::quiet_NaN());
        const double ms = cs::bench::measure_ms([&] { cs::numeric::add(a, b, out, backend); });
        verify_add(out);
        cs::bench::emit_row("simd_add", name, n, 1, ms, n,
            "calls C08 add; allocation/init/check excluded; no performance ranking");
    }

    if (cs::numeric::available(cs::numeric::backend::sse2)) {
        std::fill(out.begin(),out.end(),std::numeric_limits<float>::quiet_NaN());
        const double ms = cs::bench::measure_ms([&] { cs::numeric::add_sse2_tail_mask(a, b, out); });
        verify_add(out);
        cs::bench::emit_row("simd_add", "sse2_tail_mask", n, 1, ms, n,
            "temporary-array tail mask; proves safe tail, not speed");
    }

    for (const auto [name, backend] : backends) {
        if (!cs::numeric::available(backend)) continue;
        double result = 0.0;
        const double ms = cs::bench::measure_ms([&] { result = cs::numeric::dot(a, b, backend); });
        cs::check(result == dot_oracle(n), "dot analytic oracle");
        cs::bench::emit_row("simd_dot", name, n, 1, ms, n,
            "float inputs, double accumulation; analytic oracle checks tail");
    }

    std::vector<float> map_in(n), map_out(n);
    for (std::size_t i = 0; i < n; ++i) map_in[i] = static_cast<float>(static_cast<int>(i % 33) - 16);
    const std::array<std::pair<std::string_view, cs::numeric::policy>, 5> policies{{
        {"plain", cs::numeric::policy::plain},
        {"seq", cs::numeric::policy::seq},
        {"par", cs::numeric::policy::par},
        {"unseq", cs::numeric::policy::unseq},
        {"par_unseq", cs::numeric::policy::par_unseq},
    }};
    for (const auto [name, policy] : policies) {
        if (!cs::numeric::policy_available(policy)) {
            std::cerr << "SKIP " << name << ": execution policy unavailable\n";
            continue;
        }
        std::fill(map_out.begin(),map_out.end(),std::numeric_limits<float>::quiet_NaN());
        const double ms = cs::bench::measure_ms([&] { cs::numeric::map(map_in, map_out, policy); });
        verify_map(map_out);
        const bool managed=policy==cs::numeric::policy::par || policy==cs::numeric::policy::par_unseq;
        cs::bench::emit_row("execution_map", name, n, managed ? 0 : 1, ms, n,
            managed ? "std::execution implementation-managed; actual worker count not observed"
                    : "plain/seq/unseq: operations on invoking thread; vectorization not inferred");
    }

    return 0;
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}
