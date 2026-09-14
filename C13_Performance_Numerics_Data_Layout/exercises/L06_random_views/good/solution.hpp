#pragma once
#include "c13/numerics.hpp"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <random>
#include <ranges>
#include <utility>
#include <vector>

namespace student {
struct summary {
    std::vector<double> samples;
    std::vector<double> first_non_negative;
    double non_negative_sum = 0.0;
};

inline summary summarize(std::size_t count, std::uint32_t seed) {
    std::mt19937 engine(seed);
    std::uniform_real_distribution<double> dist(-2.0, 2.0);
    std::vector<double> samples;
    samples.reserve(count);
    for (std::size_t i = 0; i < count; ++i) samples.push_back(dist(engine));

    std::vector<double> first;
    first.reserve(5);
    for (double value : samples | std::views::filter([](double x) { return x >= 0.0; }) | std::views::take(5)) {
        first.push_back(value);
    }

    double sum = 0.0;
    double lost = 0.0;
    for (double value : first) {
        const double next = sum + value;
        lost += (std::abs(sum) >= std::abs(value)) ? (sum - next) + value : (value - next) + sum;
        sum = next;
    }
    return {std::move(samples), std::move(first), sum + lost};
}
}
