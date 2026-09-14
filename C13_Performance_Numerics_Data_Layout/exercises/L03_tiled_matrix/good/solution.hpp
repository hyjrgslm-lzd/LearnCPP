#pragma once
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <span>
#include <vector>

namespace student {
inline void require(bool condition, const char* message) {
    if (!condition) throw std::invalid_argument(message);
}

inline std::size_t checked_product(std::size_t a, std::size_t b) {
    if (b != 0 && a > std::numeric_limits<std::size_t>::max() / b)
        throw std::length_error("shape product overflows size_t");
    return a * b;
}

inline std::vector<double> multiply(std::span<const double> a, std::size_t ar, std::size_t ac,
                                    std::span<const double> b, std::size_t br, std::size_t bc, std::size_t tile) {
    require(ac == br, "matrix inner shape mismatch");
    require(tile > 0, "tile must be positive");
    require(checked_product(ar, ac) == a.size(), "matrix input shape mismatch");
    require(checked_product(br, bc) == b.size(), "matrix input shape mismatch");
    std::vector<double> c(checked_product(ar, bc));
    for (std::size_t i = 0; i < ar; ++i)
        for (std::size_t k = 0; k < ac; ++k)
            for (std::size_t j = 0; j < bc; ++j)
                c[i * bc + j] += a[i * ac + k] * b[k * bc + j];
    return c;
}
}
