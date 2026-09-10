#include <check.hpp>

#include <iostream>
#include <type_traits>

int main() {
    int value = 1;
    int& ref = value;
    const int const_value = 2;

    check((std::is_same_v<decltype(value), int>), "decltype(name) gives declared type");
    check((std::is_same_v<decltype(ref), int&>), "decltype(reference name) keeps declared reference type");
    check((std::is_same_v<decltype((value)), int&>), "decltype((lvalue)) gives lvalue reference");
    check((std::is_same_v<decltype((const_value)), const int&>), "decltype((const lvalue)) gives const lvalue reference");

    std::cout << "L02_deduction decltype observation OK\n";
}
