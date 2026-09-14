#include <mdspan>
#include <array>
#include "c13/common.hpp"
int main() {
    std::array<int,6> src{0,1,2,3,4,5},dst{};
    std::mdspan<int,std::extents<std::size_t,2,3>> a(src.data());
    std::mdspan<int,std::extents<std::size_t,2,3>,std::layout_left> b(dst.data());
    std::copy(a,b);
    check(dst==std::array<int,6>{0,3,1,4,2,5},"mdspan copy preserves logical indices across layouts");
    std::fill(b,7);
    for(auto x:dst) check(x==7,"mdspan fill covers all logical elements");
}
