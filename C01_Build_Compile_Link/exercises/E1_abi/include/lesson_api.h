#pragma once

#define LESSON_ABI_VERSION 1u

#if defined(LESSON_STATIC)
#define LESSON_API
#elif defined(_WIN32) && defined(LESSON_BUILD_SHARED)
#define LESSON_API __declspec(dllexport)
#elif defined(_WIN32)
#define LESSON_API __declspec(dllimport)
#elif defined(__GNUC__)
#define LESSON_API __attribute__((visibility("default")))
#else
#define LESSON_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct lesson_engine lesson_engine;

typedef enum lesson_status {
    LESSON_OK = 0,
    LESSON_INVALID_ARGUMENT = 1,
    LESSON_UNSUPPORTED_ABI = 2,
    LESSON_OUT_OF_RANGE = 3,
    LESSON_INTERNAL_ERROR = 4
} lesson_status;

LESSON_API unsigned lesson_abi_version(void);
LESSON_API const char* lesson_status_message(lesson_status status);
LESSON_API lesson_status lesson_create(unsigned requested_abi, int seed, lesson_engine** out_engine);
LESSON_API lesson_status lesson_eval(lesson_engine* engine, int input, int* out_value);
LESSON_API void lesson_destroy(lesson_engine* engine);

#ifdef __cplusplus
}
#endif
