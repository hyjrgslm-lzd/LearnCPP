#include "lesson_api.h"

struct lesson_engine {
    int seed;
};

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
    (void)requested_abi;
    (void)seed;
    if (out_engine != nullptr) *out_engine = nullptr;
    return LESSON_INTERNAL_ERROR;
}

extern "C" lesson_status lesson_eval(lesson_engine* engine, int input, int* out_value)
{
    (void)engine;
    (void)input;
    (void)out_value;
    return LESSON_INTERNAL_ERROR;
}

extern "C" void lesson_destroy(lesson_engine* engine)
{
    delete engine;
}
