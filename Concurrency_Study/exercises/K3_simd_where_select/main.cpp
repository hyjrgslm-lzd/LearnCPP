// OBSERVATION baseline: success covers only the checks below, not every exercise Part.
#include "concurrency_study/numeric_kernels.hpp"
#include <array>
#include <iostream>
int main() {
    std::array<float,5> x{-2,-0.0f,0,0.5f,2},out{};
    cs::numeric::conditional_scalar(x,out,cs::numeric::condition::relu);
    cs::check(out==std::array<float,5>{0,0,0,0.5f,2},"relu starter");
    std::cout << "Relu keeps only x>0; predict NaN and -0 before reading Reference\n";
}
