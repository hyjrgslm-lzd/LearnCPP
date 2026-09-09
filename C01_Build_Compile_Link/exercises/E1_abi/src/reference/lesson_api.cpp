#include "lesson_api.h"

#include <new>
#include <string>

struct lesson_engine {
    int seed;
    std::string note;
};

namespace {
constexpr int min_value = -1000;
constexpr int max_value = 1000;

bool in_range(int value)
{
    return value >= min_value && value <= max_value;
}
}

extern "C" unsigned lesson_abi_version(void)
{
    return LESSON_ABI_VERSION;
}

extern "C" const char* lesson_status_message(lesson_status status)
{
    switch (status) {
    case LESSON_OK:
        return "ok";
    case LESSON_INVALID_ARGUMENT:
        return "invalid argument";
    case LESSON_UNSUPPORTED_ABI:
        return "unsupported abi";
    case LESSON_OUT_OF_RANGE:
        return "out of range";
    case LESSON_INTERNAL_ERROR:
        return "internal error";
    }
    return "unknown status";
}

extern "C" lesson_status lesson_create(unsigned requested_abi, int seed, lesson_engine** out_engine)
{
    if (out_engine == nullptr) return LESSON_INVALID_ARGUMENT;
    *out_engine = nullptr;
    if (requested_abi != LESSON_ABI_VERSION) return LESSON_UNSUPPORTED_ABI;
    if (!in_range(seed)) return LESSON_OUT_OF_RANGE;

    try {
#if defined(LESSON_FORCE_BAD_ALLOC)
        if (seed == 999) throw std::bad_alloc();
#endif
        *out_engine = new lesson_engine{seed, "owned inside library"};
        return LESSON_OK;
    } catch (...) {
        *out_engine = nullptr;
        return LESSON_INTERNAL_ERROR;
    }
}

extern "C" lesson_status lesson_eval(lesson_engine* engine, int input, int* out_value)
{
    if (engine == nullptr || out_value == nullptr) return LESSON_INVALID_ARGUMENT;
    if (!in_range(input)) return LESSON_OUT_OF_RANGE;

    try {
        *out_value = engine->seed + input;
        return LESSON_OK;
    } catch (...) {
        return LESSON_INTERNAL_ERROR;
    }
}

extern "C" void lesson_destroy(lesson_engine* engine)
{
    delete engine;
}
