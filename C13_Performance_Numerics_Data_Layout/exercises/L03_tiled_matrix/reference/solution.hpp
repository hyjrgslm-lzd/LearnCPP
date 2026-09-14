#pragma once
#include "c13/matrix.hpp"

namespace student {
inline std::vector<double> multiply(std::span<const double> a, std::size_t ar, std::size_t ac,
                                    std::span<const double> b, std::size_t br, std::size_t bc, std::size_t tile) {
    return c13::matmul_tiled(a, ar, ac, b, br, bc, tile);
}
}
