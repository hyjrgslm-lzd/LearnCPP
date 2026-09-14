#include "c13/numerics.hpp"
#include "solution.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
void check(bool condition, std::string_view message) {
    if (!condition) throw std::runtime_error("check failed: " + std::string{message});
}

void check_close(double actual, double expected, double tolerance, std::string_view message) {
    check(std::isfinite(actual) && std::abs(actual - expected) <= tolerance, message);
}
}

int main() try {
    const auto xs = c13::cancellation_input(8);
    check_close(student::stable_sum(xs), c13::analytic_cancellation_sum(8), 0.0, "stable reduction");

    const auto hard = c13::kahan_neumaier_input();
    check(c13::kahan_sum(hard) == 0.0, "kahan misses reordered cancellation");
    check_close(student::stable_sum(hard), 1.0, 0.0, "neumaier handles reordered cancellation");

    const std::vector<double> signed_values{1.0e16, 1.0, 2.0, -1.0e16, -3.0, 4.0};
    check_close(student::stable_sum(signed_values), 4.0, 0.0, "mixed signs keep small terms");
    check(student::stable_sum({}) == 0.0, "empty reduction");
    std::cout << "stable reduction checks passed\n";
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}
