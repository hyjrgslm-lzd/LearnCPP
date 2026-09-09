// Compile /fp:strict (or strict FP flags), NOT with the fast kernel's flags.
#include "fast_math_api.hpp"
#include "concurrency_study/exercise_check.hpp"
#include <algorithm>
#include <array>
#include <cfenv>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>
#if defined(__FAST_MATH__) || defined(_M_FP_FAST)
#error the oracle and comparisons must be compiled with strict floating-point semantics
#endif

namespace {
long double oracle(const float* a,const float* b,std::size_t n) {
    long double sum=0,correction=0;
    for (std::size_t i=0;i<n;++i) {
        const long double term=static_cast<long double>(a[i])*b[i];
        const long double next=sum+term;
        correction+=std::abs(sum)>=std::abs(term)?(sum-next)+term:(term-next)+sum;
        sum=next;
    }
    return sum+correction;
}
bool acceptable(long double got,long double truth,long double bound) {
    return std::isfinite(got) && std::isfinite(truth) && std::isfinite(bound) && bound>=0 &&
           std::abs(got-truth)<=bound;
}
long double gamma(std::size_t steps,long double u) {
    const long double product=static_cast<long double>(steps)*u;
    cs::check(product<0.5L,"finite-domain gamma precondition");
    return product/(1-product);
}
void check_oracle_and_comparison() {
    // This loses 1 if compensation is algebraically reassociated to zero.
    volatile float large=1e20f;
    const std::array<float,3> a{large,1,-large},b{1,1,1};
    cs::check(oracle(a.data(),b.data(),3)==1,"strict Neumaier sentinel must retain 1");
    cs::check(!acceptable(std::numeric_limits<double>::quiet_NaN(),0,1),"strict comparison rejects NaN fault");
    cs::check(!acceptable(std::numeric_limits<double>::infinity(),0,1),"strict comparison rejects Inf fault");
    cs::check(!acceptable(2,0,1) && acceptable(1,0,1),"comparison outside/on boundary");
}
void check_arrays(cs::fast_probe::backend kind) {
    for (std::size_t n:{0,1,2,3,4,5,7,17,257,1003}) {
        std::vector<float> a(n),b(n),storage(n+2,-777.0f);
        for (std::size_t i=0;i<n;++i) {
            a[i]=std::ldexp(float(int(i%19)-9),int(i%17)-8);
            b[i]=float(int(i%13)-6)/7.0f;
        }
        auto* out=storage.data()+1;
        std::fill_n(out,n,-12345.0f);
        auto verify_add=[&] {
            for (std::size_t i=0;i<n;++i) {
                const long double truth=static_cast<long double>(a[i])+b[i];
                const long double bound=std::numeric_limits<float>::epsilon()*std::abs(truth);
                cs::check(acceptable(out[i],truth,bound),"fast add vs strict independent sum");
            }
            cs::check(storage.front()==-777 && storage.back()==-777,"fast add write canaries");
        };
        if (n>0) {
            bool detected=false;
            try { verify_add(); } catch (const std::runtime_error&) { detected=true; }
            cs::check(detected,"skip-write fast-kernel fault must fail");
        }
        cs::fast_probe::add(a.data(),b.data(),out,n,kind);
        verify_add();
        long double magnitude=0;
        for (std::size_t i=0;i<n;++i) magnitude+=std::abs(static_cast<long double>(a[i])*b[i]);
        const auto bound=2*gamma(n+8,std::numeric_limits<double>::epsilon()/2)*magnitude;
        const auto truth=oracle(a.data(),b.data(),n);
        cs::check(acceptable(cs::fast_probe::dot(a.data(),b.data(),n,kind),truth,bound),"fast dot vs strict compensated oracle");
    }
    const std::array<float,8> a{0x1p60f,0x1p-60f,-0x1p60f,3,0x1p40f,2,-0x1p40f,-1};
    const std::array<float,8> b{0x1p-60f,0x1p60f,0x1p-60f,1,0x1p-40f,1,0x1p-40f,1};
    cs::check(cs::fast_probe::dot(a.data(),b.data(),a.size(),kind)==5,"fast dot independent exact cancellation oracle");
}
void check_matrices(cs::fast_probe::backend kind) {
    for (std::size_t n:{0,1,3,5,17}) {
        std::vector<float> a(n*n),b(n*n),storage(n*n+2,-777.0f);
        for (std::size_t i=0;i<n*n;++i) { a[i]=float(int(i%7)-3)/7; b[i]=float(int(i%5)-2)/3; }
        for (std::size_t block:{1,4,32}) {
            auto* out=storage.data()+1;
            std::fill_n(out,n*n,-12345.0f);
            cs::fast_probe::gemm(a.data(),b.data(),out,n,block,kind);
            for (std::size_t i=0;i<n;++i) for (std::size_t j=0;j<n;++j) {
                long double truth=0,magnitude=0;
                for (std::size_t k=0;k<n;++k) {
                    const long double term=static_cast<long double>(a[i*n+k])*b[k*n+j];
                    truth+=term; magnitude+=std::abs(term);
                }
                const auto bound=gamma(2*n+4,std::numeric_limits<float>::epsilon()/2)*magnitude;
                cs::check(acceptable(out[i*n+j],truth,bound),"fast GEMM vs strict per-element error bound");
            }
            cs::check(storage.front()==-777 && storage.back()==-777,"fast GEMM write canaries");
        }
    }
}
}
int main() try {
    cs::check(std::numeric_limits<float>::is_iec559 && std::numeric_limits<double>::digits>=53,"binary32/64 validation domain");
    cs::check(std::fegetround()==FE_TONEAREST,"round-to-nearest validation environment");
    check_oracle_and_comparison();
    for (auto kind:{cs::fast_probe::backend::scalar,cs::fast_probe::backend::sse2,cs::fast_probe::backend::xsimd}) {
        if (!cs::fast_probe::available(kind)) { std::cerr << "SKIP fast backend " << int(kind) << '\n'; continue; }
        check_arrays(kind);
        if (kind!=cs::fast_probe::backend::xsimd) check_matrices(kind);
    }
    std::cout << "fast_math_check OK: strict oracle/comparison + separately compiled fast kernels; finite-domain only\n";
} catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
