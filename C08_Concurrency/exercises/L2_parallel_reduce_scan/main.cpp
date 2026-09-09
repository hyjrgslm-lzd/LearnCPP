// OBSERVATION baseline: success covers only the checks below, not every exercise Part.
#include "concurrency_study/numeric_kernels.hpp"
#include <array>
#include <iostream>
int main() {
    std::array<int,4> x{1,2,3,4},prefix{};
    std::partial_sum(x.begin(),x.end(),prefix.begin());
    cs::check(prefix==std::array<int,4>{1,3,6,10},"prefix starter");
    std::cout << "Inclusive: 1 3 6 10; predict exclusive with init=10\n";
}
