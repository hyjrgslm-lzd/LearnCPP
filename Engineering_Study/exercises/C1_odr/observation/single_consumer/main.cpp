#include "lesson.hpp"

#include <cstdlib>
#include <iostream>

int main()
{
    const int value = lesson_value();
    std::cout << "single consumer header definition: " << value << '\n';
    return value == 42 ? EXIT_SUCCESS : EXIT_FAILURE;
}

