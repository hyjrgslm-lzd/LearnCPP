// K2_sbt_and_acceleration_structure/programs.cu
// ============================================================================
// [K2-T01] Exercise K2 OptiX device programs
// [K2-T02]
// [K2-T03] Program types:
// [K2-T04]   __raygen__cam            - generate perspective rays, call optixTrace
// [K2-T05]   __miss__background       - return sky-blue background (via payload)
// [K2-T06]   __closesthit__radiance   - compute triangle normal, write a normal-vis color
// ============================================================================

#include <optix.h>
#include <cuda_runtime.h>

// ============================================================================
// [K2-T07] Shared structs - keep exactly in sync with host.cpp
// ============================================================================
struct Params {
    float4*                output;
    unsigned int           width;
    unsigned int           height;
    OptixTraversableHandle handle;
};

struct HitGroupData {
    float3* vertices;
    uint3*  indices;
};

extern "C" __constant__ Params params;

// ============================================================================
// [K2-T08] Payload helpers (3 uints carry R/G/B float bit-pattern)
// ============================================================================
__device__ __forceinline__ float3 get_payload_rgb() {
    return {
        __uint_as_float(optixGetPayload_0()),
        __uint_as_float(optixGetPayload_1()),
        __uint_as_float(optixGetPayload_2()),
    };
}
__device__ __forceinline__ void set_payload_rgb(float r, float g, float b) {
    optixSetPayload_0(__float_as_uint(r));
    optixSetPayload_1(__float_as_uint(g));
    optixSetPayload_2(__float_as_uint(b));
}

// ============================================================================
// [K2-T09] __raygen__cam
// [K2-T10]
// [K2-T11] Generate perspective camera rays (eye at z=3, looking towards -z,
// [K2-T12] FOV ~60 deg). Call optixTrace and write the color returned via
// [K2-T13] payload (from miss / closest-hit) into the output buffer.
// [K2-T14]
// [K2-T15] TODO [REQUIRED] step 10 (matches doc K2 item 10):
// [K2-T16]   - compute ray direction from (u, v)
// [K2-T17]   - initialize payload to zero
// [K2-T18]   - call optixTrace(params.handle, origin, direction, ...)
// [K2-T19]   - write payload rgb to params.output
// ============================================================================
extern "C" __global__ void __raygen__cam() {
    const uint3  idx  = optixGetLaunchIndex();
    const uint3  dims = optixGetLaunchDimensions();

    const float u = (static_cast<float>(idx.x) + 0.5f) / static_cast<float>(dims.x);
    const float v = (static_cast<float>(idx.y) + 0.5f) / static_cast<float>(dims.y);

    // [K2-T20] Perspective camera parameters
    const float3 eye    = {0.f, 0.f, 3.f};
    const float3 lower  = {-1.f, -1.f, 2.f};  // [K2-T21] near plane lower-left
    const float3 right  = { 2.f,  0.f, 0.f};  // [K2-T22] near plane horizontal vector
    const float3 up     = { 0.f,  2.f, 0.f};  // [K2-T23] near plane vertical vector

    // [K2-T24] TODO [REQUIRED] compute ray direction
    const float3 target    = {
        lower.x + u * right.x + v * up.x,
        lower.y + u * right.y + v * up.y,
        lower.z + u * right.z + v * up.z
    };
    const float3 direction = {
        target.x - eye.x,
        target.y - eye.y,
        target.z - eye.z
    };
    // [K2-T25] normalize
    float len = sqrtf(direction.x*direction.x +
                      direction.y*direction.y +
                      direction.z*direction.z);
    const float3 dir_n = {direction.x/len, direction.y/len, direction.z/len};

    // [K2-T26] TODO [REQUIRED] init payload so we never read undefined values
    unsigned int p0 = 0, p1 = 0, p2 = 0;

    // [K2-T27] TODO [REQUIRED] call optixTrace
    optixTrace(
        params.handle,
        eye, dir_n,
        1e-3f,   // [K2-T28] tmin
        1e16f,   // [K2-T29] tmax
        0.f,     // [K2-T30] rayTime
        OptixVisibilityMask(255),
        OPTIX_RAY_FLAG_NONE,
        0,       // [K2-T31] SBT offset
        1,       // [K2-T32] SBT stride
        0,       // [K2-T33] miss SBT index
        p0, p1, p2);

    const float3 color = {
        __uint_as_float(p0),
        __uint_as_float(p1),
        __uint_as_float(p2)
    };

    // [K2-T34] TODO [REQUIRED] write result (step 11)
    params.output[idx.y * dims.x + idx.x] =
        make_float4(color.x, color.y, color.z, 1.f);
}

// ============================================================================
// [K2-T35] __miss__background
// [K2-T36]
// [K2-T37] Runs when the ray hits no geometry.
// [K2-T38] TODO [REQUIRED] step 4: return a gradient sky color from ray direction
// ============================================================================
extern "C" __global__ void __miss__background() {
    // [K2-T39] TODO [REQUIRED] take ray direction and compute a sky gradient
    const float3 dir = optixGetWorldRayDirection();
    // [K2-T40] simple: linear blend of sky-blue and white based on y component
    float t = 0.5f * (dir.y / sqrtf(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z) + 1.f);
    set_payload_rgb(
        0.2f * (1.f - t) + 0.5f * t,
        0.3f * (1.f - t) + 0.7f * t,
        0.8f * (1.f - t) + 1.0f * t);
}

// ============================================================================
// [K2-T41] __closesthit__radiance
// [K2-T42]
// [K2-T43] Runs when the ray hits a triangle. Reads vertex/index buffers from
// [K2-T44] SBT data, computes the normal, writes a normal-visualization color.
// [K2-T45]
// [K2-T46] TODO [REQUIRED] step 5 (matches doc K2 item 5):
// [K2-T47]   - use optixGetPrimitiveIndex() to get the triangle id
// [K2-T48]   - read vertices/indices from HitGroupData
// [K2-T49]   - cross product two edges to get the normal
// [K2-T50]   - write normal * 0.5 + 0.5 into payload (normal visualization)
// ============================================================================
extern "C" __global__ void __closesthit__radiance() {
    // [K2-T51] TODO [REQUIRED] get per-record SBT data
    const HitGroupData* hg_data =
        reinterpret_cast<const HitGroupData*>(optixGetSbtDataPointer());

    // [K2-T52] TODO [REQUIRED] get triangle index
    const unsigned int prim_idx = optixGetPrimitiveIndex();
    const uint3 tri = hg_data->indices[prim_idx];

    const float3& v0 = hg_data->vertices[tri.x];
    const float3& v1 = hg_data->vertices[tri.y];
    const float3& v2 = hg_data->vertices[tri.z];

    // [K2-T53] TODO [REQUIRED] compute normal (cross product of two edges)
    const float3 e1 = {v1.x - v0.x, v1.y - v0.y, v1.z - v0.z};
    const float3 e2 = {v2.x - v0.x, v2.y - v0.y, v2.z - v0.z};
    float3 n = {
        e1.y * e2.z - e1.z * e2.y,
        e1.z * e2.x - e1.x * e2.z,
        e1.x * e2.y - e1.y * e2.x
    };
    float nlen = sqrtf(n.x*n.x + n.y*n.y + n.z*n.z);
    if (nlen > 0.f) { n.x /= nlen; n.y /= nlen; n.z /= nlen; }

    // [K2-T54] Normal visualization: n * 0.5 + 0.5 maps to [0, 1]
    set_payload_rgb(
        n.x * 0.5f + 0.5f,
        n.y * 0.5f + 0.5f,
        n.z * 0.5f + 0.5f);

    // [K2-T55] TODO [ADVANCED] return different material color per face
    //          (extend HitGroupData with a color field)
}
