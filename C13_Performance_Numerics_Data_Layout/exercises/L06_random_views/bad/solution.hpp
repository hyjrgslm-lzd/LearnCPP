#pragma once
#include "c13/numerics.hpp"
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace student {
struct summary {
    std::vector<double> samples;
    std::vector<double> first_non_negative;
    double non_negative_sum = 0.0;
};

inline summary summarize(std::size_t count, std::uint32_t seed) {
    static std::uint32_t call_count = 0;
    auto samples = c13::fixed_uniform_input(count, seed + ++call_count, -2.0, 2.0);
    auto first = c13::first_non_negative_values(samples, 5);
    return {std::move(samples), std::move(first), c13::neumaier_sum(first)};
}
}
