#pragma once
#include "concurrency_study/simd_kernels.hpp"
#include <cstdint>

namespace cs::cap3 {
inline void fill_matrices(cs::numeric::output a,cs::numeric::output b,std::size_t n) {
    for (std::size_t i=0;i<n;++i)
        for (std::size_t j=0;j<n;++j) {
            a[i*n+j]=float(int((i+2*j)%7)-3);
            b[i*n+j]=float(int((3*i+j)%5)-2);
        }
}
// Independent integer arithmetic oracle for the generated small-integer domain.
// This validates all outputs, not only C[0] or a checksum which can cancel errors.
inline void verify_matrix(cs::numeric::input c,std::size_t n) {
    cs::check(c.size()==cs::numeric::square_size(n),"oracle extent");
    for (std::size_t i=0;i<n;++i)
        for (std::size_t j=0;j<n;++j) {
            std::int64_t expected=0;
            for (std::size_t k=0;k<n;++k)
                expected+=(int((i+2*k)%7)-3)*(int((3*k+j)%5)-2);
            cs::check(c[i*n+j]==float(expected),"GEMM integer oracle");
        }
}
} // namespace cs::cap3
