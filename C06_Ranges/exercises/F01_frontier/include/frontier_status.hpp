#pragma once
#include <check.hpp>
#include <iostream>
#include <string_view>
#include <version>

inline int unavailable(std::string_view feature, std::string_view reason) {
    std::cout << "SKIP " << feature << ": " << reason << '\n';
    return 77;
}
inline int verified(std::string_view feature) {
    std::cout << "PASS " << feature << ": real body instantiated, linked and checked\n";
    return 0;
}
