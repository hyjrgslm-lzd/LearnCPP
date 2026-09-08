#include "check.hpp"
#include "d1_runtime.h"

#include <cstdlib>
#include <iostream>

int main()
{
    check(lesson_runtime_value() == 7, "linked library must provide lesson_runtime_value");
    check(lesson_runtime_init_count() == 1, "runtime initialization must have happened before the call");
    std::cout << "linked check passed: value=" << lesson_runtime_value() << '\n';
    return EXIT_SUCCESS;
}
