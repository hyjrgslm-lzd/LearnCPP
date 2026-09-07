// Compile ONLY this translation unit with /fp:fast (or -ffast-math).
// Both objects must disable LTO: /GL- and /LTCG:OFF, or -fno-lto.
#include "fast_math_api.hpp"
#include "concurrency_study/simd_kernels.hpp"
#if !defined(__FAST_MATH__) && !defined(_M_FP_FAST)
#error fast_math_kernel must be compiled with the documented fast-math option
#endif

namespace cs::fast_probe {
namespace {
cs::numeric::backend implementation(backend kind) {
    switch (kind) {
    case backend::scalar: return cs::numeric::backend::scalar;
    case backend::sse2: return cs::numeric::backend::sse2;
    case backend::xsimd: return cs::numeric::backend::xsimd;
    }
    throw std::invalid_argument("unknown fast backend");
}
}
bool available(backend kind) { return cs::numeric::available(implementation(kind)); }
void add(const float* a,const float* b,float* out,std::size_t n,backend kind) {
    cs::numeric::add({a,n},{b,n},{out,n},implementation(kind));
}
double dot(const float* a,const float* b,std::size_t n,backend kind) {
    return cs::numeric::dot({a,n},{b,n},implementation(kind));
}
void gemm(const float* a,const float* b,float* out,std::size_t n,std::size_t block,backend kind) {
    const auto count=cs::numeric::square_size(n);
    if (kind==backend::scalar) cs::numeric::gemm_tiled({a,count},{b,count},{out,count},n,block);
    else if (kind==backend::sse2) cs::numeric::gemm_sse2({a,count},{b,count},{out,count},n,block);
    else throw std::invalid_argument("no xsimd GEMM implementation in this course");
}
}
