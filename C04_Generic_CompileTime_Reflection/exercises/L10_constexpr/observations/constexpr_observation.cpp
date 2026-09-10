#include <check.hpp>

#include <iostream>

constexpr int twice(int value) {
    return value * 2;
}

constexpr int phase(int value) {
    if consteval {
        return value + 1;
    } else {
        return value + 2;
    }
}

int main() {
    static_assert(twice(4) == 8);
    int runtime = 4;
    check(twice(runtime) == 8, "constexpr function can also run at runtime");
    static_assert(phase(1) == 2);
    check(phase(runtime) == 6, "if consteval observes runtime evaluation here");
    std::cout << "L10 constexpr observation OK\n";
}
