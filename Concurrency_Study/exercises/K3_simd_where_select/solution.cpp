#include "concurrency_study/simd_kernels.hpp"
#include <iostream>
int main() {
    using namespace cs::numeric;
    const float nan=std::numeric_limits<float>::quiet_NaN(), inf=std::numeric_limits<float>::infinity();
    const std::array<float,9> x{-inf,-2,-0.0f,0,0.5f,2,inf,nan,-0.5f};
    const std::array<float,9> ab{inf,2,0,0,0.5f,2,inf,nan,0.5f};
    const std::array<float,9> cl{-1,-1,-0.0f,0,0.5f,1,1,nan,-0.5f};
    const std::array<float,9> re{0,0,0,0,0.5f,2,inf,0,0};
    for (auto kind : {backend::scalar,backend::sse2,backend::xsimd,backend::std_simd}) {
        if (!available(kind)) { std::cout << "SKIP optional backend " << int(kind) << '\n'; continue; }
        for (auto op : {condition::absolute,condition::clamp,condition::relu}) {
            std::array<float,9> got{};
            conditional(x,got,op,kind);
            const auto& expected=op==condition::absolute ? ab : op==condition::clamp ? cl : re;
            for (std::size_t i=0;i<x.size();++i) {
                cs::check(std::isnan(expected[i]) ? std::isnan(got[i]) : got[i]==expected[i],"conditional oracle");
                if (expected[i]==0) cs::check(std::signbit(got[i])==std::signbit(expected[i]),"signed zero policy");
            }
        }
    }
    std::cout << "K3 Reference OK\n";
}
