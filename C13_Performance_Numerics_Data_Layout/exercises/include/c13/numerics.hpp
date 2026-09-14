#ifndef C13_NUMERICS_HPP
#define C13_NUMERICS_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <random>
#include <ranges>
#include <span>
#include <stdexcept>
#include <vector>

namespace c13 {

inline double naive_sum(std::span<const double> xs) {
    double sum = 0.0;
    for (double x : xs) sum += x;
    return sum;
}

inline double pairwise_sum(std::span<const double> xs) {
    if (xs.empty()) return 0.0;
    if (xs.size() == 1) return xs.front();
    const auto mid = xs.size() / 2;
    return pairwise_sum(xs.first(mid)) + pairwise_sum(xs.subspan(mid));
}

inline double kahan_sum(std::span<const double> xs) {
    double sum = 0.0;
    double compensation = 0.0;
    for (double x : xs) {
        const double y = x - compensation;
        const double t = sum + y;
        compensation = (t - sum) - y;
        sum = t;
    }
    return sum;
}

inline double neumaier_sum(std::span<const double> xs) {
    double sum = 0.0;
    double compensation = 0.0;
    for (double x : xs) {
        const double t = sum + x;
        if (std::abs(sum) >= std::abs(x)) {
            compensation += (sum - t) + x;
        } else {
            compensation += (x - t) + sum;
        }
        sum = t;
    }
    return sum + compensation;
}

inline double analytic_cancellation_sum(std::size_t triples) {
    if (triples > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        throw std::length_error("analytic sum exceeds exact teaching range");
    return static_cast<double>(triples);
}

inline std::vector<double> cancellation_input(std::size_t pairs) {
    std::vector<double> xs;
    if (pairs > xs.max_size() / 3) throw std::length_error("input is too large");
    xs.reserve(pairs * 3);
    for (std::size_t i = 0; i < pairs; ++i) {
        xs.push_back(1.0e16);
        xs.push_back(1.0);
        xs.push_back(-1.0e16);
    }
    return xs;
}

inline std::vector<double> kahan_neumaier_input() {
    return {1.0e16, 1.0, -1.0e16};
}

inline std::vector<double> fixed_uniform_input(std::size_t count, std::uint32_t seed,
                                               double lo = -1.0, double hi = 1.0) {
    if (!std::isfinite(lo) || !std::isfinite(hi) || !(lo <= hi) || !std::isfinite(hi-lo))
        throw std::invalid_argument("random distribution bounds are invalid");
    if (count > 1'000'000) throw std::length_error("random input exceeds teaching memory budget");
    std::mt19937 engine(seed);
    std::uniform_real_distribution<double> distribution(lo, hi);
    std::vector<double> xs;
    xs.reserve(count);
    for (std::size_t i = 0; i < count; ++i) xs.push_back(distribution(engine));
    return xs;
}

inline auto non_negative_values(std::span<const double> xs) {
    return xs | std::views::filter([](double value) { return value >= 0.0; });
}

inline std::vector<double> first_non_negative_values(std::span<const double> xs, std::size_t count) {
    std::vector<double> out;
    const auto limit=std::min(count,xs.size());
    if (limit > static_cast<std::size_t>(std::numeric_limits<std::ptrdiff_t>::max()))
        throw std::length_error("view count exceeds difference_type");
    out.reserve(limit);
    for (double value : non_negative_values(xs) | std::views::take(static_cast<std::ptrdiff_t>(limit))) out.push_back(value);
    return out;
}

} // namespace c13
#endif
