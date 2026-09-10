#include <check.hpp>
#include <array>
#include <iostream>

int main() {
    std::array<double, 4> v{1, 2, 3, 4};
    for (std::size_t i = 0; i < v.size(); ++i) v[i] = v[v.size() - 1 - i];
    check(v != std::array<double, 4>{4, 3, 2, 1}, "direct reverse write demonstrates alias corruption");
    std::cout << "A05 direct write counterexample observed\n";
}
