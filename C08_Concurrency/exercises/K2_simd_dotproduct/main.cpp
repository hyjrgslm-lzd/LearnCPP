// OBSERVATION baseline: success covers only the checks below, not every exercise Part.
#include "concurrency_study/numeric_kernels.hpp"
#include <array>
#include <iostream>
int main() {
    std::array<float,3> a{1,2,3}, b{4,5,6};
    cs::check(cs::numeric::dot_scalar(a,b)==32,"dot starter");
    std::cout << "dot=32; derive lane partial sums before implementing horizontal reduction\n";
}
