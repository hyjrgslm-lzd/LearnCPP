#include <check.hpp>
#include <array>
#include <cmath>
#include <iostream>

int main() {
    std::array<double, 3> a{1, 2, 3}, b{4, 5, 6}, c{10, 20, 30}, out{};
    for (std::size_t i = 0; i < out.size(); ++i) out[i] = a[i] + b[i] * 2.0 + c[out.size() - 1 - i];
    check(std::abs(out[0] - 39.0) < 1e-9 && std::abs(out[2] - 25.0) < 1e-9,
        "eager numeric reference matches the expression contract");
    std::cout << "A05 eager numeric reference OK\n";
}
