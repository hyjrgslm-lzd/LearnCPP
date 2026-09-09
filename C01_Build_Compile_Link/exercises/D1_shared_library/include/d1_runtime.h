#pragma once

#if defined(_WIN32) && defined(D1_RUNTIME_BUILD_SHARED)
#define D1_RUNTIME_API __declspec(dllexport)
#elif defined(_WIN32) && defined(D1_RUNTIME_USE_SHARED)
#define D1_RUNTIME_API __declspec(dllimport)
#elif defined(__GNUC__)
#define D1_RUNTIME_API __attribute__((visibility("default")))
#else
#define D1_RUNTIME_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

D1_RUNTIME_API int lesson_runtime_value(void);
D1_RUNTIME_API int lesson_runtime_init_count(void);
D1_RUNTIME_API void lesson_runtime_record_exit_to(const char* path);

#ifdef __cplusplus
}
#endif
