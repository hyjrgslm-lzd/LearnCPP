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
    const auto a = student::summarize(32, 0xC013u);
    const auto b = student::summarize(32, 0xC013u);
    const auto c = student::summarize(32, 0xC014u);

    check(a.samples == b.samples, "fixed seed");
    check(a.samples != c.samples, "different seed changes input");
    check(a.samples.size() == 32, "sample count");
    for (double value : a.samples) check(value >= -2.0 && value <= 2.0, "distribution bounds");

    const auto expected = c13::first_non_negative_values(a.samples, 5);
    check(a.first_non_negative == expected, "view observes generated storage");
    check_close(a.non_negative_sum, c13::neumaier_sum(expected), 0.0, "view sum");

    std::vector<double> borrowed{-1.0, 2.0, -3.0, 4.0};
    auto view = c13::non_negative_values(borrowed);
    borrowed[1] = -2.0;
    double observed = 0.0;
    for (double value : view) observed += value;
    check_close(observed, 4.0, 0.0, "view borrows source");

    std::cout << "random views checks passed\n";
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}
