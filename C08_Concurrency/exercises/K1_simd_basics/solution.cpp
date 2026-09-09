#include "concurrency_study/simd_kernels.hpp"
#include <iostream>
int main() {
    using namespace cs::numeric;
    for (auto kind : {backend::scalar,backend::sse2,backend::xsimd,backend::std_simd}) {
        if (!available(kind)) { std::cout << "SKIP optional backend " << int(kind) << '\n'; continue; }
        for (std::size_t n=0;n<=35;++n) {
            std::vector<float> a(n),b(n),out(n,-12345.0f);
            for (std::size_t i=0;i<n;++i) { a[i]=float(i); b[i]=float(2*i+1); }
            add(a,b,out,kind);
            for (std::size_t i=0;i<n;++i) cs::check(out[i]==float(3*i+1),"analytic add oracle");
        }
    }
    if (available(backend::sse2)) {
        alignas(16) float a[8]{1,2,3,4,5,6,7,8},b[8]{},storage[16]{};
        auto run=[&](std::size_t n,auto operation) {
            std::fill(std::begin(storage),std::end(storage),-777.0f);
            output out(storage+4,n); // Still aligned; guards before and after live range.
            std::fill(out.begin(),out.end(),-12345.0f);
            operation(out);
            for (std::size_t i=0;i<n;++i) cs::check(out[i]==a[i],"every aligned/tail output");
            cs::check(storage[3]==-777 && storage[4+n]==-777,"aligned/tail write boundaries");
        };
        run(8,[&](output out) { add(a,b,out,backend::sse2,true); });
        run(7,[&](output out) { add_sse2_tail_mask(input(a,7),input(b,7),out); });
        bool skipped=false;
        try { run(7,[](output) {}); } catch (const std::runtime_error&) { skipped=true; }
        cs::check(skipped,"masked-tail no-op must fail the same checker");
    }
    std::cout << "K1 Reference OK\n";
}
