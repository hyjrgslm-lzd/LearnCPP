#include "check.hpp"
#include "lesson_api.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <string_view>

namespace {
void check_contract(const char* label)
{
    check(lesson_abi_version() == LESSON_ABI_VERSION, "ABI version query must return 1");
    check(std::string_view(lesson_status_message(LESSON_OK)) == "ok", "status message must be static and readable");

    lesson_engine* stale = reinterpret_cast<lesson_engine*>(static_cast<unsigned long long>(0x1));
    check(lesson_create(2, 3, &stale) == LESSON_UNSUPPORTED_ABI, "create must reject unsupported ABI");
    check(stale == nullptr, "create must clear out pointer before ABI rejection");

    stale = reinterpret_cast<lesson_engine*>(static_cast<unsigned long long>(0x1));
    check(lesson_create(LESSON_ABI_VERSION, 1001, &stale) == LESSON_OUT_OF_RANGE, "create must reject seed above range");
    check(stale == nullptr, "create must clear out pointer before range rejection");

    check(lesson_create(LESSON_ABI_VERSION, 1, nullptr) == LESSON_INVALID_ARGUMENT, "create must reject null out pointer");

    lesson_engine* engine = nullptr;
    check(lesson_create(LESSON_ABI_VERSION, 40, &engine) == LESSON_OK, "create must accept ABI 1 and seed in range");
    check(engine != nullptr, "create success must write a handle");

    int value = -9999;
    check(lesson_eval(engine, 2, &value) == LESSON_OK, "eval must accept input in range");
    check(value == 42, "eval must compute seed + input");

    value = 12345;
    check(lesson_eval(nullptr, 2, &value) == LESSON_INVALID_ARGUMENT, "eval must reject null handle");
    check(value == 12345, "eval must not modify output on null handle");
    check(lesson_eval(engine, 2, nullptr) == LESSON_INVALID_ARGUMENT, "eval must reject null output");
    check(value == 12345, "eval null-output path must not touch caller sentinel");
    check(lesson_eval(engine, -1001, &value) == LESSON_OUT_OF_RANGE, "eval must reject input below range");
    check(value == 12345, "eval must not modify output on range failure");

    lesson_destroy(engine);
    lesson_destroy(nullptr);

    std::cout << label << " ABI contract passed\n";
}
}

int main(int argc, char** argv)
{
    try {
        check_contract(argc > 1 ? argv[1] : "lesson");
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "check failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
