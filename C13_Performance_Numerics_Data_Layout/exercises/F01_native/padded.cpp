#include <mdspan>
#include <array>
#include "c13/common.hpp"
int main() {
    std::array<double,8> data{};
    std::mdspan<double,std::extents<std::size_t,2,3>,std::layout_right_padded<4>> m(data.data());
    m[1,2]=9;
    check(m.size()==6 && m.mapping().stride(0)==4 && data[6]==9,"padded layout separates logical extent and pitch");
    check(data[3]==0 && data[7]==0,"padding remains untouched");
}
