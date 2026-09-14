#pragma once
#include "c13/common.hpp"
namespace c13 {
inline void check_close(double actual, double expected, double tolerance, std::string_view message) {
    check(tolerance >= 0 && std::isfinite(tolerance) && near(actual, expected, tolerance, 0), message);
}
}
