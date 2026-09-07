// OBSERVATION baseline: success covers only the checks below, not every exercise Part.
#include "concurrency_study/simd_kernels.hpp"
#include <array>
#include <iostream>
int main() {
    std::array<float,5> a{1,2,3,4,5},b{2,3,4,5,6},out{};
    cs::numeric::add_scalar(a,b,out);
    for (std::size_t i=0;i<5;++i) cs::check(out[i]==float(2*i+3),"scalar starter");
    std::cout << "K1 scalar baseline OK; implement a four-lane loop and a safe tail, then run Reference\n";
}
