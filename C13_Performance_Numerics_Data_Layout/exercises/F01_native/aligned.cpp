#include <mdspan>
#include "c13/common.hpp"
int main() {
    alignas(64) double data[4]{1,2,3,4};
    std::mdspan<double,std::extents<std::size_t,4>,std::layout_right,std::aligned_accessor<double,64>> m(data);
    m[2]=7;
    check(data[2]==7,"aligned accessor uses actually aligned storage");
}
