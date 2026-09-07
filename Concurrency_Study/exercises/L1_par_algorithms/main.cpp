// OBSERVATION baseline: success covers only the checks below, not every exercise Part.
#include "concurrency_study/numeric_kernels.hpp"
#include <array>
#include <iostream>
int main() {
    std::array<float,4> x{-2,-1,0,3},out{};
    cs::numeric::map(x,out,cs::numeric::policy::plain);
    cs::check(out==std::array<float,4>{6,3,2,11},"map starter");
    std::cout << "Plain transform OK; compare policy contracts before enabling par_unseq\n";
}
