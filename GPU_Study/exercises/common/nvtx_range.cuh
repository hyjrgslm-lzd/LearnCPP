#pragma once
// [CMN-T49] nvtx_range.cuh -- NVTX range RAII helper (Nsight Systems timeline)
//
// [CMN-T50] Nsight Systems workflow:
//   1. Insert NVTX_RANGE("label") or construct an NvtxRange in key regions.
//   2. Capture with: nsys profile --trace=cuda,nvtx ./executable
//   3. In the Nsight Systems GUI inspect colored ranges on the NVTX row,
//      aligned with the CUDA kernel timeline.
//   4. Nested calls form hierarchical ranges; useful for marking
//      forward / backward / optimizer phases.
//
// [CMN-T51] Disable switch: defining GPU_STUDY_DISABLE_NVTX compiles all
// macros and the class to no-ops. Use for release builds or environments
// without Nsight; zero overhead.

// --------------------------------------------------------------------------
// [CMN-T52] Implementation branches: enabled vs disabled
// --------------------------------------------------------------------------
#if !defined(GPU_STUDY_DISABLE_NVTX)

// [CMN-T53] The NVTX3 header ships with CUDA Toolkit at include/nvtx3/nvToolsExt.h
#include <nvtx3/nvToolsExt.h>
#include <cstdint>

// --------------------------------------------------------------------------
// [CMN-T54] NvtxRange -- RAII range marker.
// ctor pushes a named range; dtor pops (whether or not the scope exits via
// exception).
// --------------------------------------------------------------------------
struct NvtxRange {
    // [CMN-T55] Name only (default color).
    explicit NvtxRange(const char* name) {
        nvtxRangePushA(name);
    }

    // [CMN-T56] Name plus ARGB color (e.g. 0xFF00FF00 = opaque green).
    NvtxRange(const char* name, uint32_t argb_color) {
        nvtxEventAttributes_t attr{};
        attr.version       = NVTX_VERSION;
        attr.size          = NVTX_EVENT_ATTRIB_STRUCT_SIZE;
        attr.colorType     = NVTX_COLOR_ARGB;
        attr.color         = argb_color;
        attr.messageType   = NVTX_MESSAGE_TYPE_ASCII;
        attr.message.ascii = name;
        nvtxRangePushEx(&attr);
    }

    // [CMN-T57] Forbid copy and move (ranges must nest strictly by scope).
    NvtxRange(const NvtxRange&)            = delete;
    NvtxRange& operator=(const NvtxRange&) = delete;
    NvtxRange(NvtxRange&&)                 = delete;
    NvtxRange& operator=(NvtxRange&&)      = delete;

    ~NvtxRange() {
        nvtxRangePop();
    }
};

// --------------------------------------------------------------------------
// [CMN-T58] NVTX_RANGE(name) -- create a named range in the current scope.
// [CMN-T59] Use __LINE__ to make the variable name unique within a function.
// --------------------------------------------------------------------------
#define NVTX_RANGE(name) ::NvtxRange _nvtx_##__LINE__{name}

// [CMN-T60] NVTX_RANGE_COLOR(name, argb) -- range with explicit color.
#define NVTX_RANGE_COLOR(name, argb) ::NvtxRange _nvtx_##__LINE__{name, argb}

// [CMN-T61] NVTX_MARK(name) -- point event (no duration; appears as a marker).
#define NVTX_MARK(name) ::nvtxMarkA(name)

#else // GPU_STUDY_DISABLE_NVTX -- no-op fallback

#include <cstdint>

// [CMN-T62] No-op struct; the compiler will optimize it away entirely.
struct NvtxRange {
    explicit NvtxRange(const char*) noexcept {}
    NvtxRange(const char*, uint32_t) noexcept {}
    NvtxRange(const NvtxRange&)            = delete;
    NvtxRange& operator=(const NvtxRange&) = delete;
    NvtxRange(NvtxRange&&)                 = delete;
    NvtxRange& operator=(NvtxRange&&)      = delete;
    ~NvtxRange() = default;
};

// [CMN-T63] No-op macros: every call collapses to a void cast.
#define NVTX_RANGE(name)            ((void)0)
#define NVTX_RANGE_COLOR(name, argb) ((void)0)
#define NVTX_MARK(name)             ((void)0)

#endif // GPU_STUDY_DISABLE_NVTX
