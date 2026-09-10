#include "twice.hpp"

#include <check.hpp>

#include <iostream>

int main() {
    check(l01_extern::use_a(3) == 6, "use_a reaches explicit instantiation provider");
    check(l01_extern::use_b(3) == 8, "use_b reaches the same explicit instantiation provider");
    std::cout << "L01_templates extern template observation OK\n";
}
