// K1_optix_hello_pipeline/programs.cu
// ============================================================================
// [K1-T01] Exercise K1 OptiX device programs
// [K1-T02]
// [K1-T03] Build (driven automatically by add_custom_command in CMakeLists.txt):
// [K1-T04]   nvcc --optix-ir -I<OPTIX_INCLUDE_DIR> -std=c++17 -arch=sm_75
// [K1-T05]        -o programs.optixir programs.cu
// [K1-T06]
// [K1-T07] Program types in this file:
// [K1-T08]   __raygen__hello   - write a fixed red pixel to the output buffer
// [K1-T09]   __miss__noop      - K1 does not call optixTrace; this is a placeholder
// ============================================================================

#include <optix.h>
#include <cuda_runtime.h>

// ============================================================================
// [K1-T10] Params - must exactly match the definition in host.cpp
// [K1-T11]          (field order, type, alignment)
// ============================================================================
struct Params {
    float4*      output;
    unsigned int width;
    unsigned int height;
};

// [K1-T12] OptiX requires launch params to be passed via a __constant__ variable
extern "C" __constant__ Params params;

// ============================================================================
// [K1-T13] __raygen__hello
// [K1-T14]
// [K1-T15] Runs once per pixel; writes a fixed RGBA color (red) to output buffer.
// [K1-T16]
// [K1-T17] TODO [REQUIRED] step 3 (matches doc K1 item 3):
// [K1-T18]   - use optixGetLaunchIndex() to get pixel coords (x, y)
// [K1-T19]   - compute linear index idx = y * width + x
// [K1-T20]   - write make_float4(1, 0, 0, 1) (pure red)
// ============================================================================
extern "C" __global__ void __raygen__hello() {
    const uint3 idx  = optixGetLaunchIndex();
    const uint3 dims = optixGetLaunchDimensions();

    const unsigned int x = idx.x;
    const unsigned int y = idx.y;
    const unsigned int w = dims.x;

    // [K1-T21] TODO [REQUIRED] write color to params.output[y * w + x]
    params.output[y * w + x] = make_float4(1.0f, 0.0f, 0.0f, 1.0f);

    // [K1-T22] TODO [ADVANCED] change color to a coordinate-based gradient:
    //   float u = float(x) / float(w - 1);
    //   float v = float(y) / float(dims.y - 1);
    //   params.output[y * w + x] = make_float4(u, v, 0.5f, 1.0f);
}

// ============================================================================
// [K1-T23] __miss__noop
// [K1-T24]
// [K1-T25] In K1 raygen writes output directly without calling optixTrace,
// [K1-T26] so this miss is just a placeholder. K2 extends it to return a
// [K1-T27] background color.
// [K1-T28]
// [K1-T29] TODO [ADVANCED] when K1 raygen calls optixTrace, write the
// [K1-T30]                 background color into the payload here
// ============================================================================
extern "C" __global__ void __miss__noop() {
    // [K1-T31] TODO [ADVANCED] optixSetPayload_0(__float_as_uint(0.2f));  // R
    // [K1-T32] TODO [ADVANCED] optixSetPayload_1(__float_as_uint(0.3f));  // G
    // [K1-T33] TODO [ADVANCED] optixSetPayload_2(__float_as_uint(0.8f));  // B
}
