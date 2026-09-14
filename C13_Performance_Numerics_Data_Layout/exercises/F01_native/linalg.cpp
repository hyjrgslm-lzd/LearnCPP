#include <linalg>
#include <mdspan>
#include <array>
#include "c13/common.hpp"
int main() {
    std::array<double,4> a{1,2,3,4},b{1,0,0,1},c{-1,-1,-1,-1};
    using M=std::mdspan<double,std::extents<std::size_t,2,2>>;
    std::linalg::matrix_product(M(a.data()),M(b.data()),M(c.data()));
    check(c==a,"native linalg matrix product");
}
