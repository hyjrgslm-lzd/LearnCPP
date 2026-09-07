#pragma once
#include "concurrency_study/numeric_kernels.hpp"
#include <array>
#if defined(_M_X64) || defined(__x86_64__)
#define CS_NUMERIC_SSE2 1
#include <emmintrin.h>
#else
#define CS_NUMERIC_SSE2 0
#endif
#if CS_HAS_XSIMD
#include <xsimd/xsimd.hpp>
#endif
#if CS_HAS_STD_SIMD
#include <simd>
#endif

namespace cs::numeric {
enum class backend { scalar, sse2, xsimd, std_simd };
inline bool available(backend b) {
    switch (b) {
    case backend::scalar: return true;
    case backend::sse2: return CS_NUMERIC_SSE2;
    // This course fixes xsimd to SSE2, so it never silently raises the ISA.
    case backend::xsimd:
#if CS_HAS_XSIMD && CS_NUMERIC_SSE2
        return true;
#else
        return false;
#endif
    case backend::std_simd:
#if CS_HAS_STD_SIMD
        return true; // Native target must itself be built for a supported ISA.
#else
        return false;
#endif
    }
    return false;
}
inline backend parse_backend(std::string_view name) {
    if (name == "scalar") return backend::scalar;
    if (name == "sse2") return backend::sse2;
    if (name == "xsimd") return backend::xsimd;
    if (name == "std_simd") return backend::std_simd;
    throw std::invalid_argument("unknown SIMD backend");
}
inline void add(input a, input b, output out, backend kind, bool aligned = false) {
    binary_shape(a,b,out);
    cs::check(available(kind), "SIMD backend unavailable");
    cs::check(!aligned || kind == backend::sse2, "aligned option is for SSE2 only");
    if (aligned && !a.empty())
        cs::check(reinterpret_cast<std::uintptr_t>(a.data()) % 16 == 0 &&
                  reinterpret_cast<std::uintptr_t>(b.data()) % 16 == 0 &&
                  reinterpret_cast<std::uintptr_t>(out.data()) % 16 == 0, "SSE2 alignment must be 16");
    std::size_t i = 0;
#if CS_NUMERIC_SSE2
    if (kind == backend::sse2) {
        for (; a.size() - i >= 4; i += 4) {
            auto va = aligned ? _mm_load_ps(a.data()+i) : _mm_loadu_ps(a.data()+i);
            auto vb = aligned ? _mm_load_ps(b.data()+i) : _mm_loadu_ps(b.data()+i);
            if (aligned) _mm_store_ps(out.data()+i, _mm_add_ps(va,vb));
            else _mm_storeu_ps(out.data()+i, _mm_add_ps(va,vb));
        }
    }
#endif
#if CS_HAS_XSIMD && CS_NUMERIC_SSE2
    if (kind == backend::xsimd) {
        using V = xsimd::batch<float, xsimd::sse2>;
        for (; a.size()-i >= V::size; i += V::size)
            (V::load_unaligned(a.data()+i) + V::load_unaligned(b.data()+i)).store_unaligned(out.data()+i);
    }
#endif
#if CS_HAS_STD_SIMD
    if (kind == backend::std_simd) {
        using V = std::simd::vec<float,4>; // N5050 [simd.syn], not the Parallelism TS.
        for (; a.size()-i >= 4; i += 4) {
            const auto va = std::simd::unchecked_load<V>(a.subspan(i,4));
            const auto vb = std::simd::unchecked_load<V>(b.subspan(i,4));
            std::simd::unchecked_store(va+vb, out.subspan(i,4));
        }
        if (i < a.size()) {
            const auto va = std::simd::partial_load<V>(a.subspan(i));
            const auto vb = std::simd::partial_load<V>(b.subspan(i));
            std::simd::partial_store(va+vb, out.subspan(i));
            return;
        }
    }
#endif
    for (; i < a.size(); ++i) out[i] = a[i] + b[i];
}

// SSE2 has no fault-suppressing masked load. Copy just the live tail to a
// real four-object array, mask the result, then copy just the live lanes back.
inline void add_sse2_tail_mask(input a, input b, output out) {
    binary_shape(a,b,out);
    cs::check(available(backend::sse2), "SSE2 unavailable");
    const auto bulk = a.size() - a.size()%4;
    add(a.first(bulk),b.first(bulk),out.first(bulk),backend::sse2);
#if CS_NUMERIC_SSE2
    if (bulk == a.size()) return;
    alignas(16) float ta[4]{}, tb[4]{}, result[4]{};
    alignas(16) std::int32_t mask[4]{};
    for (std::size_t j=0; j<a.size()-bulk; ++j) { ta[j]=a[bulk+j]; tb[j]=b[bulk+j]; mask[j]=-1; }
    const auto m = _mm_castsi128_ps(_mm_load_si128(reinterpret_cast<const __m128i*>(mask)));
    _mm_store_ps(result,_mm_and_ps(m,_mm_add_ps(_mm_load_ps(ta),_mm_load_ps(tb))));
    for (std::size_t j=0; j<a.size()-bulk; ++j) out[bulk+j]=result[j];
#endif
}
inline double dot_sse2(input a, input b) {
    cs::check(a.size()==b.size() && available(backend::sse2), "dot shape/ISA");
    double sum = 0;
    std::size_t i = 0;
#if CS_NUMERIC_SSE2
    __m128d even = _mm_setzero_pd(), odd = _mm_setzero_pd();
    for (; a.size()-i >= 4; i+=4) {
        const auto va = _mm_loadu_ps(a.data()+i), vb = _mm_loadu_ps(b.data()+i);
        even = _mm_add_pd(even,_mm_mul_pd(_mm_cvtps_pd(va),_mm_cvtps_pd(vb)));
        odd = _mm_add_pd(odd,_mm_mul_pd(_mm_cvtps_pd(_mm_movehl_ps(va,va)),
                                     _mm_cvtps_pd(_mm_movehl_ps(vb,vb))));
    }
    alignas(16) double lanes[2];
    _mm_store_pd(lanes,_mm_add_pd(even,odd));
    sum = lanes[0]+lanes[1]; // Horizontal reduction, outside the hot loop.
#endif
    for (; i<a.size(); ++i) sum += static_cast<double>(a[i])*static_cast<double>(b[i]);
    return sum;
}

inline double dot(input a,input b,backend kind) {
    cs::check(a.size()==b.size() && available(kind),"dot shape/backend");
    if (kind==backend::sse2) return dot_sse2(a,b);
    std::size_t i=0;
    double sum=0;
#if CS_HAS_XSIMD && CS_NUMERIC_SSE2
    if (kind==backend::xsimd) {
        using V=xsimd::batch<double,xsimd::sse2>;
        V acc(0.0);
        for (; a.size()-i>=V::size; i+=V::size)
            acc+=V::load_unaligned(a.data()+i)*V::load_unaligned(b.data()+i);
        sum=xsimd::reduce_add(acc);
    }
#endif
#if CS_HAS_STD_SIMD
    if (kind==backend::std_simd) {
        using V=std::simd::vec<double,4>;
        V acc(0.0);
        for (; a.size()-i>=4; i+=4)
            acc+=std::simd::unchecked_load<V>(a.subspan(i,4))*std::simd::unchecked_load<V>(b.subspan(i,4));
        sum=std::simd::reduce(acc);
    }
#endif
    for (; i<a.size(); ++i) sum+=static_cast<double>(a[i])*static_cast<double>(b[i]);
    return sum;
}

inline void conditional(input a, output out, condition op, backend kind) {
    unary_shape(a,out);
    cs::check(available(kind), "conditional backend unavailable");
    std::size_t i=0;
#if CS_NUMERIC_SSE2
    if (kind==backend::sse2) {
        auto choose=[](__m128 m, __m128 yes, __m128 no) { return _mm_or_ps(_mm_and_ps(m,yes),_mm_andnot_ps(m,no)); };
        for (; a.size()-i>=4; i+=4) {
            const auto x=_mm_loadu_ps(a.data()+i);
            auto r=x;
            if (op==condition::absolute) r=_mm_andnot_ps(_mm_set1_ps(-0.0f),x);
            else if (op==condition::relu) r=choose(_mm_cmpgt_ps(x,_mm_setzero_ps()),x,_mm_setzero_ps());
            else {
                r=choose(_mm_cmplt_ps(x,_mm_set1_ps(-1)),_mm_set1_ps(-1),x);
                r=choose(_mm_cmpgt_ps(r,_mm_set1_ps(1)),_mm_set1_ps(1),r);
            }
            _mm_storeu_ps(out.data()+i,r);
        }
    }
#endif
#if CS_HAS_XSIMD && CS_NUMERIC_SSE2
    if (kind==backend::xsimd) {
        using V=xsimd::batch<float,xsimd::sse2>;
        for (; a.size()-i>=V::size; i+=V::size) {
            const auto x=V::load_unaligned(a.data()+i);
            auto r=x;
            if (op==condition::absolute) r=xsimd::abs(x);
            else if (op==condition::relu) r=xsimd::select(x>V(0.0f),x,V(0.0f));
            else { r=xsimd::select(x<V(-1.0f),V(-1.0f),x); r=xsimd::select(r>V(1.0f),V(1.0f),r); }
            r.store_unaligned(out.data()+i);
        }
    }
#endif
#if CS_HAS_STD_SIMD
    if (kind==backend::std_simd) {
        using V=std::simd::vec<float,4>;
        for (; i<a.size(); i+=std::min<std::size_t>(4,a.size()-i)) {
            const auto live=std::min<std::size_t>(4,a.size()-i);
            const auto x=std::simd::partial_load<V>(a.subspan(i,live));
            auto r=x;
            if (op==condition::absolute) {
                r=std::simd::select(x<V(0.0f),-x,x);
                r=std::simd::select(x==V(0.0f),V(0.0f),r);
            } else if (op==condition::relu) r=std::simd::select(x>V(0.0f),x,V(0.0f));
            else { r=std::simd::select(x<V(-1.0f),V(-1.0f),x); r=std::simd::select(r>V(1.0f),V(1.0f),r); }
            std::simd::partial_store(r,out.subspan(i,live));
        }
    }
#endif
    for (; i<a.size(); ++i) out[i]=conditional_value(a[i],op);
}

// Checked gather/scatter semantics remain those of numeric_kernels.hpp. This
// SSE2 implementation uses scalar gathers + vector arithmetic; no HW gather claim.
inline void gather_add_sse2(input source, std::span<const std::size_t> index, output out) {
    cs::check(available(backend::sse2),"SSE2 unavailable");
    gather(source,index,out); // Validates the whole index set before writing.
#if CS_NUMERIC_SSE2
    std::size_t i=0;
    for (; out.size()-i>=4; i+=4)
        _mm_storeu_ps(out.data()+i,_mm_add_ps(_mm_loadu_ps(out.data()+i),_mm_set1_ps(1)));
    for (; i<out.size(); ++i) out[i]+=1;
#else
    throw std::runtime_error("SSE2 unavailable");
#endif
}
inline void gemm_sse2(input a,input b,output c,std::size_t n,std::size_t block) {
    matrix_shape(a,b,c,n);
    cs::check(block>0 && available(backend::sse2),"invalid tile/ISA");
    std::fill(c.begin(),c.end(),0.0f);
#if CS_NUMERIC_SSE2
    for (std::size_t ii=0; ii<n; ii+=std::min(block,n-ii))
        for (std::size_t kk=0; kk<n; kk+=std::min(block,n-kk))
            for (std::size_t jj=0; jj<n; jj+=std::min(block,n-jj))
                for (auto i=ii; i<ii+std::min(block,n-ii); ++i)
                    for (auto k=kk; k<kk+std::min(block,n-kk); ++k) {
                        auto j=jj;
                        const auto end=jj+std::min(block,n-jj);
                        const auto av=_mm_set1_ps(a[i*n+k]);
                        for (; end-j>=4; j+=4)
                            _mm_storeu_ps(c.data()+i*n+j,_mm_add_ps(_mm_loadu_ps(c.data()+i*n+j),
                                        _mm_mul_ps(av,_mm_loadu_ps(b.data()+k*n+j))));
                        for (; j<end; ++j) c[i*n+j]+=a[i*n+k]*b[k*n+j];
                    }
#endif
}
} // namespace cs::numeric
