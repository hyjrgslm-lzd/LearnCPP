#include <simd>
#include <array>
#include <span>
#include "c13/common.hpp"
int main() {
    using V=std::simd::vec<float,4>;
    const std::array<float,5> input{1,2,3,4,5};
    std::array<float,5> out{};
    const auto a=std::simd::unchecked_load<V>(std::span(input).first<4>());
    std::simd::unchecked_store(a+V(1),std::span(out).first<4>());
    const auto tail=std::simd::partial_load<V>(std::span(input).subspan(4));
    std::simd::partial_store(tail+V(1),std::span(out).subspan(4));
    check(out==std::array<float,5>{2,3,4,5,6},"native SIMD full and partial stores");
}
