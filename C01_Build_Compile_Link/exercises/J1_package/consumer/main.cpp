#include "lesson_api.h"

#include <cstdlib>
#include <iostream>

int main()
{
    lesson_engine* engine = nullptr;
    lesson_status status = lesson_create(LESSON_ABI_VERSION, 40, &engine);
    if (status != LESSON_OK || engine == nullptr) {
        std::cerr << "create failed: " << lesson_status_message(status) << '\n';
        return EXIT_FAILURE;
    }

    int value = 0;
    status = lesson_eval(engine, 2, &value);
    lesson_destroy(engine);
    if (status != LESSON_OK || value != 42) {
        std::cerr << "eval failed: " << lesson_status_message(status) << " value=" << value << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "package consumer passed: value=" << value << '\n';
    return EXIT_SUCCESS;
}
