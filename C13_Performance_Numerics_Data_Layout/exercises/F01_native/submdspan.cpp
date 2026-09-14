#include <mdspan>
#include <array>
#include "c13/common.hpp"
int main() {
    std::array<int,6> values{0,1,2,3,4,5};
    std::mdspan<int,std::extents<std::size_t,2,3>> matrix(values.data());
    auto column=std::submdspan(matrix,std::full_extent,1);
    check(column.extent(0)==2 && column[0]==1 && column[1]==4,"strided native submdspan column");
    column[1]=42;
    check(matrix[1,1]==42,"submdspan borrows original storage");
}
