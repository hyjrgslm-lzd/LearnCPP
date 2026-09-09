#pragma once
#include <cstddef>

// This is the ONLY header shared by the strict checker and fast translation unit.
// No inline arithmetic, production kernel definitions, oracle, or comparisons here.
namespace cs::fast_probe {
enum class backend { scalar, sse2, xsimd };
bool available(backend kind);
void add(const float* a,const float* b,float* out,std::size_t n,backend kind);
double dot(const float* a,const float* b,std::size_t n,backend kind);
void gemm(const float* a,const float* b,float* out,std::size_t n,std::size_t block,backend kind);
}
