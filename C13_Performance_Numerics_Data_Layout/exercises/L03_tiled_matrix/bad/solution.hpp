#pragma once
#include <cstddef>
#include <span>
#include <vector>

namespace student {
inline std::vector<double> multiply(std::span<const double>, std::size_t ar, std::size_t,
                                    std::span<const double>, std::size_t, std::size_t bc, std::size_t) {
    return std::vector<double>(ar * bc);
}
}
