#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace student {
struct summary {
    std::vector<double> samples;
    std::vector<double> first_non_negative;
    double non_negative_sum = 0.0;
};

inline summary summarize(std::size_t, std::uint32_t) {
    // TODO: generate the requested sample, select the first nonnegative values, and sum them.
    return {};
}
}
