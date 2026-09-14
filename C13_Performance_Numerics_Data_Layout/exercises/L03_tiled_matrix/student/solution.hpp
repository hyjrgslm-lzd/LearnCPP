#pragma once
#include <cstddef>
#include <span>
#include <vector>

namespace student {
inline std::vector<double> multiply(std::span<const double>, std::size_t, std::size_t,
                                    std::span<const double>, std::size_t, std::size_t, std::size_t) {
    // TODO: validate dimensions and tile, then compute every output element including edge tiles.
    return {};
}
}
