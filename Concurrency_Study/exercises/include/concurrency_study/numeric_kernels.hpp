#pragma once

#include "concurrency_study/exercise_check.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <limits>
#include <numeric>
#include <span>
#include <stdexcept>
#include <thread>
#include <vector>
#if CS_HAS_PARALLEL_ALGORITHMS
#include <execution>
#endif

namespace cs::numeric {
using input = std::span<const float>;
using output = std::span<float>;

// std::less provides a total order even for pointers into different allocations.
template<class A, class B>
bool overlaps(std::span<A> a, std::span<B> b) {
    if (a.empty() || b.empty()) return false;
    const auto less = std::less<const void*>{};
    return less(a.data(), b.data() + b.size()) && less(b.data(), a.data() + a.size());
}

inline void unary_shape(input a, output out) {
    cs::check(a.size() == out.size(), "unary lengths differ");
    cs::check(!overlaps(a, out), "output must not overlap input");
}
inline void binary_shape(input a, input b, output out) {
    unary_shape(a, out);
    cs::check(a.size() == b.size(), "binary lengths differ");
    cs::check(!overlaps(b, out), "output must not overlap second input");
}
inline void add_scalar(input a, input b, output out) {
    binary_shape(a, b, out);
    for (std::size_t i = 0; i < a.size(); ++i) out[i] = a[i] + b[i];
}
inline double dot_scalar(input a, input b) {
    cs::check(a.size() == b.size(), "dot lengths differ");
    double sum = 0;
    for (std::size_t i = 0; i < a.size(); ++i)
        sum += static_cast<double>(a[i]) * static_cast<double>(b[i]);
    return sum;
}

enum class condition { absolute, clamp, relu };
inline float conditional_value(float x, condition op) {
    if (op == condition::absolute) return std::fabs(x); // abs(-0) is +0
    if (op == condition::relu) return x > 0 ? x : 0.0f; // NaN maps to +0
    return x < -1 ? -1.0f : (x > 1 ? 1.0f : x); // NaN and -0 preserved
}
inline void conditional_scalar(input a, output out, condition op) {
    unary_shape(a, out);
    for (std::size_t i = 0; i < a.size(); ++i) out[i] = conditional_value(a[i], op);
}

// Checked permutation baseline. Duplicate scatter is rejected before any write.
inline void gather(input source, std::span<const std::size_t> index, output out) {
    cs::check(index.size() == out.size(), "gather lengths differ");
    cs::check(!overlaps(source, out), "gather overlap");
    cs::check(!overlaps(index, out), "gather index overlap");
    for (auto i : index) cs::check(i < source.size(), "gather index out of range");
    for (std::size_t i = 0; i < index.size(); ++i) out[i] = source[index[i]];
}
inline void scatter_unique(input source, std::span<const std::size_t> index, output out) {
    cs::check(source.size() == index.size(), "scatter lengths differ");
    cs::check(!overlaps(source, out) && !overlaps(index, out), "scatter overlap");
    auto sorted = std::vector<std::size_t>(index.begin(), index.end());
    for (auto i : sorted) cs::check(i < out.size(), "scatter index out of range");
    std::sort(sorted.begin(), sorted.end());
    cs::check(std::adjacent_find(sorted.begin(), sorted.end()) == sorted.end(),
              "duplicate scatter index");
    for (std::size_t i = 0; i < index.size(); ++i) out[index[i]] = source[i];
}

// Worker id owns one exception slot. No concurrent writes to the same slot;
// jthread cleanup also joins previously created workers if creation throws.
template<class F>
void parallel_chunks(std::size_t n, std::size_t requested, F&& body) {
    cs::check(requested > 0 && requested <= 256, "threads must be in [1,256]");
    if (n == 0) return;
    const auto count = std::min(n, requested);
    std::vector<std::exception_ptr> errors(count);
    std::vector<std::jthread> workers;
    workers.reserve(count);
    for (std::size_t t = 0; t < count; ++t) {
        const auto lo = n / count * t + std::min(t, n % count);
        const auto hi = lo + n / count + (t < n % count ? 1 : 0);
        workers.emplace_back([&, t, lo, hi] {
            try { body(t, lo, hi); }
            catch (...) { errors[t] = std::current_exception(); }
        });
    }
    workers.clear();
    for (const auto& error : errors) if (error) std::rethrow_exception(error);
}
inline double sum_threaded(input a, std::size_t threads) {
    cs::check(threads > 0 && threads <= 256, "threads must be in [1,256]");
    std::vector<double> sums(std::min(a.size(), threads));
    parallel_chunks(a.size(), threads, [&](auto t, auto lo, auto hi) {
        double local = 0;
        for (auto i = lo; i < hi; ++i) local += static_cast<double>(a[i]);
        sums[t] = local; // One final store: padding is not the first optimization.
    });
    return std::accumulate(sums.begin(), sums.end(), 0.0);
}

enum class policy { plain, seq, par, unseq, par_unseq };
inline bool policy_available(policy p) {
#if CS_HAS_PARALLEL_ALGORITHMS
    (void)p;
    return true;
#else
    return p == policy::plain;
#endif
}
inline policy parse_policy(std::string_view name) {
    if (name == "plain") return policy::plain;
    if (name == "seq") return policy::seq;
    if (name == "par") return policy::par;
    if (name == "unseq") return policy::unseq;
    if (name == "par_unseq") return policy::par_unseq;
    throw std::invalid_argument("unknown execution policy");
}
// Integer-valued float inputs in [-16,16] give an exactly representable oracle.
inline float map_value(float x) noexcept { return x * x + 2.0f; }
// Finite-domain heavy workload: sum_{k=0}^{96} x^k, |x|<=1/2.
// Independent validation uses 1/(1-x) and a tail bound, not this recurrence.
inline float geometric_value(float x) noexcept {
    double term=1,sum=1;
    for (int k=1;k<=96;++k) { term*=static_cast<double>(x); sum+=term; }
    return static_cast<float>(sum);
}
template<class F> inline void map_with(input a, output out, policy p,F operation) {
    unary_shape(a, out);
    cs::check(policy_available(p), "execution policy unavailable");
    if (p == policy::plain) { std::transform(a.begin(), a.end(), out.begin(), operation); return; }
#if CS_HAS_PARALLEL_ALGORITHMS
    switch (p) {
    case policy::seq: std::transform(std::execution::seq, a.begin(), a.end(), out.begin(), operation); break;
    case policy::par: std::transform(std::execution::par, a.begin(), a.end(), out.begin(), operation); break;
    case policy::unseq: std::transform(std::execution::unseq, a.begin(), a.end(), out.begin(), operation); break;
    case policy::par_unseq: std::transform(std::execution::par_unseq, a.begin(), a.end(), out.begin(), operation); break;
    default: break;
    }
#endif
}
inline void map(input a,output out,policy p,bool heavy=false) {
    if (heavy) {
        // Validate before entering a policy callback (callbacks must not throw).
        for (float x:a) cs::check(std::isfinite(x) && std::abs(x)<=0.5f,"heavy input must be finite and in [-0.5,0.5]");
        map_with(a,out,p,[](float x) noexcept { return geometric_value(x); });
    } else map_with(a,out,p,[](float x) noexcept { return map_value(x); });
}
inline double sum_policy(input a, bool parallel) {
    const auto widen = [](float x) noexcept { return static_cast<double>(x); };
    if (!parallel) return std::transform_reduce(a.begin(), a.end(), 0.0, std::plus<>{}, widen);
#if CS_HAS_PARALLEL_ALGORITHMS
    return std::transform_reduce(std::execution::par, a.begin(), a.end(), 0.0, std::plus<>{}, widen);
#else
    throw std::runtime_error("parallel reduce unavailable");
#endif
}
inline void sort_values(std::span<int> a, bool parallel) {
    if (!parallel) { std::sort(a.begin(), a.end()); return; }
#if CS_HAS_PARALLEL_ALGORITHMS
    std::sort(std::execution::par, a.begin(), a.end());
#else
    throw std::runtime_error("parallel sort unavailable");
#endif
}

inline std::size_t square_size(std::size_t n) {
    cs::check(n == 0 || n <= std::numeric_limits<std::size_t>::max() / n, "matrix extent overflow");
    return n * n;
}
inline void matrix_shape(input a, input b, output c, std::size_t n) {
    cs::check(a.size() == square_size(n), "matrix extent mismatch");
    binary_shape(a, b, c);
}
inline void gemm_naive(input a, input b, output c, std::size_t n) {
    matrix_shape(a, b, c, n);
    for (std::size_t i = 0; i < n; ++i)
        for (std::size_t j = 0; j < n; ++j) {
            float s = 0;
            for (std::size_t k = 0; k < n; ++k) s += a[i*n+k] * b[k*n+j];
            c[i*n+j] = s;
        }
}
// Only [lo,hi) rows are written. Every k contributes once, in increasing order.
inline void gemm_rows(input a, input b, output c, std::size_t n,
                      std::size_t block, std::size_t lo, std::size_t hi) {
    for (auto ii = lo; ii < hi; ii += std::min(block, hi - ii))
        for (std::size_t kk = 0; kk < n; kk += std::min(block, n - kk))
            for (std::size_t jj = 0; jj < n; jj += std::min(block, n - jj))
                for (auto i = ii; i < ii + std::min(block, hi - ii); ++i)
                    for (auto k = kk; k < kk + std::min(block, n - kk); ++k)
                        for (auto j = jj; j < jj + std::min(block, n - jj); ++j)
                            c[i*n+j] += a[i*n+k] * b[k*n+j];
}
inline void gemm_tiled(input a, input b, output c, std::size_t n, std::size_t block) {
    matrix_shape(a, b, c, n);
    cs::check(block > 0, "zero tile");
    std::fill(c.begin(), c.end(), 0.0f);
    gemm_rows(a, b, c, n, block, 0, n);
}
inline void gemm_threaded(input a, input b, output c, std::size_t n,
                          std::size_t block, std::size_t threads) {
    matrix_shape(a, b, c, n);
    cs::check(block > 0 && threads > 0 && threads <= 256, "invalid tile or threads");
    std::fill(c.begin(), c.end(), 0.0f);
    parallel_chunks(n, threads, [&](auto, auto lo, auto hi) { gemm_rows(a,b,c,n,block,lo,hi); });
}
inline void gemm_par(input a, input b, output c, std::size_t n, std::size_t block) {
    matrix_shape(a, b, c, n);
    cs::check(block > 0, "zero tile");
#if CS_HAS_PARALLEL_ALGORITHMS
    std::vector<std::size_t> rows(n / block + (n % block != 0));
    std::iota(rows.begin(), rows.end(), std::size_t{0});
    std::fill(c.begin(), c.end(), 0.0f);
    std::for_each(std::execution::par, rows.begin(), rows.end(), [&](auto tile) noexcept {
        const auto lo = tile * block;
        gemm_rows(a,b,c,n,block,lo,lo + std::min(block,n-lo));
    });
#else
    throw std::runtime_error("parallel GEMM unavailable");
#endif
}
struct particle { float x, y, z, mass; };
inline void update_aos(std::span<particle> a, float dt) {
    for (auto& p : a) p.x += dt * p.mass;
}
inline void update_soa(output x, input mass, float dt) {
    cs::check(x.size() == mass.size() && !overlaps(x,mass), "SoA shape/overlap");
    for (std::size_t i = 0; i < x.size(); ++i) x[i] += dt * mass[i];
}
} // namespace cs::numeric
