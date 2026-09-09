#include "check.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>

int call_lesson_from_a();
int call_lesson_from_b();
int lesson_value();

int main()
{
    try {
        const int a = call_lesson_from_a();
        const int b = call_lesson_from_b();
        const int direct = lesson_value();
        check(direct == 42, "reference lesson_value must return 42");
        check(a == 42, "reference caller A must return 42");
        check(b == 42, "reference caller B must return 42");
        check(a + b == 84, "reference checker must consume both call paths");

        std::cout << "reference check passed: direct=" << direct << " a=" << a << " b=" << b << '\n';
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "reference check failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
