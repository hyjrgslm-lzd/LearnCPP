#pragma once
#include "concurrency_study/numeric_kernels.hpp"
#include <cmath>
#include <limits>

namespace cs::policy_bench {
inline void verify(cs::numeric::input input,cs::numeric::input out,bool heavy) {
    cs::check(input.size()==out.size(),"map checker size");
    for (std::size_t i=0;i<input.size();++i) {
        if (heavy) {
            const double exact_infinite=1.0/(1.0-static_cast<double>(input[i]));
            const double bound=2*std::numeric_limits<float>::epsilon()*std::abs(exact_infinite)+0x1p-96;
            cs::check(std::isfinite(out[i]) && std::abs(double(out[i])-exact_infinite)<=bound,
                      "geometric map vs independent closed-form and tail bound");
        } else {
            const int x=static_cast<int>(input[i]);
            cs::check(out[i]==float(x*x+2),"light integer oracle");
        }
    }
}
} // namespace cs::policy_bench
