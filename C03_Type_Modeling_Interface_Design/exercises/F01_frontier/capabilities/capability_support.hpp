#pragma once

#include <iostream>
#include <string_view>

inline int skip(std::string_view reason) {
    std::cout << "SKIP: " << reason << '\n';
    return 77;
}

inline void print_macro(std::string_view name, long long value) {
    std::cout << name << '=' << value << '\n';
}

