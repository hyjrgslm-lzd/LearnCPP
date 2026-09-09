#pragma once
// [CMN-T01] cuda_check.cuh -- CUDA / driver API error-check macros
//
// [CMN-T02] Usage:
//   CUDA_CHECK      : wrap any cudaXxx() runtime API call (cuda_runtime.h)
//   CUDA_CHECK_LAST : check async error after a kernel launch (cudaGetLastError)
//   CU_CHECK        : wrap driver API cuXxx() calls (cuda.h / libcuda)
//                     The driver API is lower-level (TMA / cuTensorMap / module
//                     load); the runtime API is built on top of it. Do not mix
//                     both APIs on the same context.

#include <cstdio>
#include <cstdlib>
#include <cuda_runtime.h>

// --------------------------------------------------------------------------
// [CMN-T03] Runtime API check (most common path)
// --------------------------------------------------------------------------
#define CUDA_CHECK(expr)                                                        \
    do {                                                                        \
        cudaError_t _e = (expr);                                                \
        if (_e != cudaSuccess) {                                                \
            std::fprintf(stderr,                                                \
                "CUDA Runtime Error: %s\n"                                      \
                "  expr : %s\n"                                                 \
                "  file : %s:%d\n",                                             \
                cudaGetErrorString(_e), #expr, __FILE__, __LINE__);             \
            std::abort();                                                       \
        }                                                                       \
    } while (0)

// [CMN-T04] Check immediately after a kernel launch (catches async errors
// that a non-blocking launch would otherwise leave behind).
#define CUDA_CHECK_LAST()                                                       \
    do {                                                                        \
        cudaError_t _e = cudaGetLastError();                                    \
        if (_e != cudaSuccess) {                                                \
            std::fprintf(stderr,                                                \
                "CUDA Launch Error: %s\n"                                       \
                "  file : %s:%d\n",                                             \
                cudaGetErrorString(_e), __FILE__, __LINE__);                    \
            std::abort();                                                       \
        }                                                                       \
    } while (0)

// --------------------------------------------------------------------------
// [CMN-T05] Driver API check (requires <cuda.h>; ships with CUDA Toolkit).
// [CMN-T06] Only enabled when cuda.h is included or CUDA_VERSION is defined,
// to avoid breaking pure runtime-only translation units.
// --------------------------------------------------------------------------
#if defined(__CUDA_API_VERSION) || defined(CUDA_VERSION)
#include <cuda.h>

#define CU_CHECK(expr)                                                          \
    do {                                                                        \
        CUresult _r = (expr);                                                   \
        if (_r != CUDA_SUCCESS) {                                               \
            const char* _msg = nullptr;                                         \
            cuGetErrorString(_r, &_msg);                                        \
            std::fprintf(stderr,                                                \
                "CUDA Driver Error: %s\n"                                       \
                "  expr : %s\n"                                                 \
                "  file : %s:%d\n",                                             \
                _msg ? _msg : "unknown", #expr, __FILE__, __LINE__);            \
            std::abort();                                                       \
        }                                                                       \
    } while (0)

#endif // __CUDA_API_VERSION || CUDA_VERSION
