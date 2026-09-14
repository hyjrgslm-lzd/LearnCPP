#pragma once
#include <check.hpp>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace c13 {
using ::check;
inline bool near(double actual, double expected, double absolute = 1e-12, double relative = 1e-12) {
    return std::isfinite(actual) && std::isfinite(expected)
        && std::abs(actual - expected) <= absolute + relative * std::max(std::abs(actual), std::abs(expected));
}
inline std::size_t checked_product(std::size_t a, std::size_t b) {
    if (b != 0 && a > std::numeric_limits<std::size_t>::max() / b)
        throw std::length_error("shape product overflows size_t");
    return a * b;
}
template<class F> int run(F&& body) {
    try { body(); return 0; }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
} // namespace c13
