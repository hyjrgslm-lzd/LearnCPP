#include <cstdlib>
#include <iostream>

int call_lesson_from_a_inline();
int call_lesson_from_b_inline();

int main()
{
    const int a = call_lesson_from_a_inline();
    const int b = call_lesson_from_b_inline();
    if (a != 42 || b != 42 || a + b != 84) {
        std::cerr << "inline reference failed: a=" << a << " b=" << b << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "inline reference passed: a=" << a << " b=" << b << '\n';
    return EXIT_SUCCESS;
}

