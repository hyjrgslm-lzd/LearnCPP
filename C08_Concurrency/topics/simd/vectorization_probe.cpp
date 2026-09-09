// Compile with vectorization diagnostics; wrappers call the SAME measured kernels.
#include "concurrency_study/simd_kernels.hpp"
#include <array>
#include <iostream>
#if defined(_MSC_VER)
#define CS_PROBE_NOINLINE __declspec(noinline)
#else
#define CS_PROBE_NOINLINE __attribute__((noinline))
#endif
extern "C" CS_PROBE_NOINLINE void probe_scalar(const float* a,const float* b,float* out,std::size_t n) {
    cs::numeric::add_scalar({a,n},{b,n},{out,n});
}
extern "C" CS_PROBE_NOINLINE void probe_sse2(const float* a,const float* b,float* out,std::size_t n) {
    cs::numeric::add({a,n},{b,n},{out,n},cs::numeric::backend::sse2);
}
extern "C" CS_PROBE_NOINLINE double probe_dot(const float* a,const float* b,std::size_t n) {
    return cs::numeric::dot_scalar({a,n},{b,n});
}
int main() {
    std::array<float,17> a{},b{},out{};
    for (std::size_t i=0;i<a.size();++i) { a[i]=float(i); b[i]=0.5f; }
    probe_scalar(a.data(),b.data(),out.data(),a.size());
    for (std::size_t i=0;i<a.size();++i) cs::check(out[i]==float(i)+0.5f,"probe scalar");
    if (cs::numeric::available(cs::numeric::backend::sse2)) {
        probe_sse2(a.data(),b.data(),out.data(),a.size());
        for (std::size_t i=0;i<a.size();++i) cs::check(out[i]==float(i)+0.5f,"probe SSE2");
    }
    cs::check(probe_dot(a.data(),b.data(),a.size())==68,"probe dot");
    std::cout << "vectorization probe OK; inspect diagnostics/assembly, not the function name\n";
}
