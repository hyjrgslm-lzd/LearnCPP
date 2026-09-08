#include "check.hpp"

#include <cstdlib>
#include <iostream>
#include <type_traits>

struct public_layout_example {
    char tag;
    int value;
};

struct reordered_layout_example {
    int value;
    char tag;
};

int main()
{
    check(std::is_standard_layout_v<public_layout_example>, "layout example must be standard-layout");
    std::cout << "public_layout_example size=" << sizeof(public_layout_example)
              << " align=" << alignof(public_layout_example) << '\n';
    std::cout << "reordered_layout_example size=" << sizeof(reordered_layout_example)
              << " align=" << alignof(reordered_layout_example) << '\n';
    std::cout << "opaque lesson_engine keeps this layout out of the ABI\n";
    return EXIT_SUCCESS;
}
