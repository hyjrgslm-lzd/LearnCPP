// Isolated target: this TU uses the reference mdspan, never native <mdspan>.
#include <experimental/mdspan>
#include <experimental/linalg>
#include "c13/common.hpp"
#include <array>
int main() { return c13::run([] {
    namespace ex=std::experimental;
    namespace la=std::experimental::linalg;
    std::array<double,6> a{1,2,3,4,5,6};
    std::array<double,6> b{7,8,9,10,11,12};
    std::array<double,4> c{-999,-999,-999,-999};
    ex::mdspan<double,ex::extents<std::size_t,2,3>> A(a.data());
    ex::mdspan<double,ex::extents<std::size_t,3,2>> B(b.data());
    ex::mdspan<double,ex::extents<std::size_t,2,2>> C(c.data());
    la::matrix_product(A,B,C);
    check(c==std::array<double,4>{58,64,139,154},"stdBLAS overwrites a non-square matrix product");
    std::array<double,3> x{1,2,3}, y{4,5,6};
    ex::mdspan<double,ex::extents<std::size_t,3>> X(x.data()),Y(y.data());
    check(c13::near(la::dot(X,Y,0.0),32.0),"stdBLAS dot actual interface");
    std::cout << "stdBLAS reference dot and matrix_product passed\n";
}); }
