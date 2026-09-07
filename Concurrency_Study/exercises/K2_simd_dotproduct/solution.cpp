#include "concurrency_study/simd_kernels.hpp"
#include <iostream>
int main() {
    using namespace cs::numeric;
    for (std::size_t n=0;n<=67;++n) {
        std::vector<float> a(n),b(n,0.5f);
        for (std::size_t i=0;i<n;++i) a[i]=float(i+1);
        const double exact=double(n)*double(n+1)/4;
        cs::check(dot_scalar(a,b)==exact,"analytic triangular dot");
        if (available(backend::sse2)) cs::check(dot_sse2(a,b)==exact,"wide SIMD triangular dot");
        for (auto kind:{backend::xsimd,backend::std_simd})
            if (available(kind)) cs::check(dot(a,b,kind)==exact,"optional SIMD dot oracle");
    }
    const std::array<float,5> a{16777216.0f,1,-16777216.0f,0.5f,-0.5f},b{1,1,1,1,1};
    cs::check(dot_scalar(a,b)==1,"widen before multiplying and summing");
    if (available(backend::sse2)) cs::check(dot_sse2(a,b)==1,"SIMD cancellation");
    const float x=1.0f+std::ldexp(1.0f,-13),y=1.0f-std::ldexp(1.0f,-13);
    volatile float rounded=x*y; // Explicit float rounding between multiply and add.
    const float separate=rounded-1.0f;
    cs::check(separate==0 && std::fma(x,y,-1.0f)==-std::ldexp(1.0f,-26),"FMA one-rounding example");
    std::cout << "K2 OK; SSE2=" << available(backend::sse2) << "; FMA differs by design\n";
}
