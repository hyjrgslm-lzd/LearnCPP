#include "check.hpp"
#include "lesson_api.h"

#include <cstdlib>
#include <iostream>

int main()
{
    lesson_engine* engine = reinterpret_cast<lesson_engine*>(static_cast<unsigned long long>(0x1));
    check(lesson_create(LESSON_ABI_VERSION, 999, &engine) == LESSON_INTERNAL_ERROR, "bad_alloc variant must convert exception to status");
    check(engine == nullptr, "bad_alloc variant must leave no handle");
    lesson_destroy(engine);
    std::cout << "bad_alloc conversion check passed\n";
    return EXIT_SUCCESS;
}
