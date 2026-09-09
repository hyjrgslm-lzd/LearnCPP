// K3_intersection_anyhit_closesthit_miss/programs.cu
// ============================================================================
// [K3-T01] Exercise K3 OptiX device programs - five program types
// [K3-T02]
// [K3-T03] Program types in this file:
// [K3-T04]   __raygen__principal     - generate primary rays (depth=0)
// [K3-T05]   __miss__background      - check depth, return bg color or shadow flag
// [K3-T06]   __anyhit__opaque        - alpha test placeholder (always passes)
// [K3-T07]   __closesthit__radiance  - Lambertian direct lighting + shadow ray
// [K3-T08]
// [K3-T09] Advanced (skeleton, commented out by default):
// [K3-T10]   __intersection__sphere  - custom sphere intersection
// ============================================================================

#include <optix.h>
#include <cuda_runtime.h>

// ============================================================================
// [K3-T11] Shared structs - keep exactly in sync with host.cpp
// ============================================================================
struct PointLight {
    float3 pos;
    float3 intensity;
};

struct Params {
    float4*                output;
    unsigned int           width;
    unsigned int           height;
    OptixTraversableHandle handle;
    PointLight             light;
};

struct HitGroupData {
    float3* vertices;
    uint3*  indices;
    float3  base_color;
};

extern "C" __constant__ Params params;

// ============================================================================
// [K3-T12] Payload helpers
// [K3-T13] payload[0..2] = R/G/B (float bit-cast)
// [K3-T14] payload[3]    = depth (uint, 0 = primary ray, 1 = shadow ray)
// ============================================================================
__device__ __forceinline__ void set_payload_rgb(float r, float g, float b) {
    optixSetPayload_0(__float_as_uint(r));
    optixSetPayload_1(__float_as_uint(g));
    optixSetPayload_2(__float_as_uint(b));
}
__device__ __forceinline__ float3 get_payload_rgb() {
    return {
        __uint_as_float(optixGetPayload_0()),
        __uint_as_float(optixGetPayload_1()),
        __uint_as_float(optixGetPayload_2()),
    };
}
__device__ __forceinline__ unsigned int get_payload_depth() {
    return optixGetPayload_3();
}
__device__ __forceinline__ void set_payload_depth(unsigned int d) {
    optixSetPayload_3(d);
}

// [K3-T15] float3 helpers
__device__ __forceinline__ float3 normalize3(float3 v) {
    float l = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
    return l > 0.f ? float3{v.x/l, v.y/l, v.z/l} : float3{0.f, 0.f, 0.f};
}
__device__ __forceinline__ float dot3(float3 a, float3 b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}
__device__ __forceinline__ float3 cross3(float3 a, float3 b) {
    return {a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x};
}

// ============================================================================
// [K3-T16] __raygen__principal
// [K3-T17]
// [K3-T18] TODO [REQUIRED] step 7:
// [K3-T19]   - set payload.depth = 0
// [K3-T20]   - compute perspective ray direction
// [K3-T21]   - zero-init payload
// [K3-T22]   - call optixTrace (4 payload slots)
// [K3-T23]   - write payload rgb into output buffer
// ============================================================================
extern "C" __global__ void __raygen__principal() {
    const uint3 idx  = optixGetLaunchIndex();
    const uint3 dims = optixGetLaunchDimensions();

    const float u = (static_cast<float>(idx.x) + 0.5f) / static_cast<float>(dims.x);
    const float v = (static_cast<float>(idx.y) + 0.5f) / static_cast<float>(dims.y);

    const float3 eye   = {0.f, 0.f, 3.f};
    const float3 lower = {-1.f, -1.f, 2.f};
    const float3 right = { 2.f,  0.f, 0.f};
    const float3 up    = { 0.f,  2.f, 0.f};

    const float3 target = {
        lower.x + u * right.x + v * up.x,
        lower.y + u * right.y + v * up.y,
        lower.z + u * right.z + v * up.z,
    };
    const float3 dir_n = normalize3({
        target.x - eye.x, target.y - eye.y, target.z - eye.z
    });

    // [K3-T24] TODO [REQUIRED] init payload (depth = 0, color = 0)
    unsigned int p0 = 0, p1 = 0, p2 = 0;
    unsigned int p3 = 0;  // [K3-T25] depth = 0 (primary ray)

    // [K3-T26] TODO [REQUIRED] call optixTrace (4 payload slots)
    optixTrace(
        params.handle,
        eye, dir_n,
        1e-3f, 1e16f, 0.f,
        OptixVisibilityMask(255),
        OPTIX_RAY_FLAG_NONE,
        0, 1, 0,
        p0, p1, p2, p3);

    params.output[idx.y * dims.x + idx.x] = make_float4(
        __uint_as_float(p0),
        __uint_as_float(p1),
        __uint_as_float(p2),
        1.f);
}

// ============================================================================
// [K3-T27] __miss__background
// [K3-T28]
// [K3-T29] TODO [REQUIRED] step 6:
// [K3-T30]   - check payload depth
// [K3-T31]   - depth == 0 (primary ray): return gradient bg color
// [K3-T32]   - depth == 1 (shadow ray): return (1,1,1) meaning unoccluded (light visible)
// ============================================================================
extern "C" __global__ void __miss__background() {
    const unsigned int depth = get_payload_depth();

    if (depth == 0) {
        // [K3-T33] primary ray miss -> sky-blue gradient
        const float3 dir = normalize3(optixGetWorldRayDirection());
        float t = 0.5f * (dir.y + 1.f);
        set_payload_rgb(
            0.2f * (1.f - t) + 0.5f * t,
            0.3f * (1.f - t) + 0.7f * t,
            0.8f * (1.f - t) + 1.0f * t);
        set_payload_depth(0);
    } else {
        // [K3-T34] shadow ray miss -> light is visible, return "unoccluded" flag
        // [K3-T35] closest-hit treats depth=2 as "unoccluded"
        set_payload_depth(2);
    }
}

// ============================================================================
// [K3-T36] __anyhit__opaque
// [K3-T37]
// [K3-T38] TODO [REQUIRED] step 2: alpha test placeholder
// [K3-T39]   - this implementation always passes (alpha=1.0, opaque)
// [K3-T40]   - if transparent, call optixIgnoreIntersection() to skip current hit
// ============================================================================
extern "C" __global__ void __anyhit__opaque() {
    // [K3-T41] TODO [REQUIRED] always pass (opaque)
    // [K3-T42] alpha-test example:
    //   float alpha = sample_alpha_texture(...);
    //   if (alpha < 0.5f) optixIgnoreIntersection();
    // [K3-T43] no action needed here (OptiX keeps the current hit by default)
}

// ============================================================================
// [K3-T44] __closesthit__radiance
// [K3-T45]
// [K3-T46] TODO [REQUIRED] step 4: Lambertian direct lighting
// [K3-T47]   1. read vertices/indices from HitGroupData, compute normal
// [K3-T48]   2. compute hit point (ray origin + t * direction)
// [K3-T49]   3. spawn shadow ray (depth=1), check occlusion
// [K3-T50]   4. compute Lambertian cosine term
// [K3-T51]   5. if unoccluded: color = base_color * intensity * cosine
// [K3-T52]      if occluded:  color = 0 (shadow)
// ============================================================================
extern "C" __global__ void __closesthit__radiance() {
    const HitGroupData* hg =
        reinterpret_cast<const HitGroupData*>(optixGetSbtDataPointer());

    const unsigned int prim_idx = optixGetPrimitiveIndex();
    const uint3 tri = hg->indices[prim_idx];

    const float3& v0 = hg->vertices[tri.x];
    const float3& v1 = hg->vertices[tri.y];
    const float3& v2 = hg->vertices[tri.z];

    // [K3-T53] TODO [REQUIRED] compute normal
    const float3 e1 = {v1.x-v0.x, v1.y-v0.y, v1.z-v0.z};
    const float3 e2 = {v2.x-v0.x, v2.y-v0.y, v2.z-v0.z};
    const float3 normal = normalize3(cross3(e1, e2));

    // [K3-T54] TODO [REQUIRED] compute hit point (offset along normal to avoid self-intersection)
    const float  t_hit     = optixGetRayTmax();
    const float3 ray_orig  = optixGetWorldRayOrigin();
    const float3 ray_dir   = optixGetWorldRayDirection();
    const float3 hit_point = {
        ray_orig.x + t_hit * ray_dir.x + normal.x * 1e-4f,
        ray_orig.y + t_hit * ray_dir.y + normal.y * 1e-4f,
        ray_orig.z + t_hit * ray_dir.z + normal.z * 1e-4f,
    };

    // [K3-T55] TODO [REQUIRED] compute light direction + cosine term
    const PointLight& light = params.light;
    const float3 light_vec = {
        light.pos.x - hit_point.x,
        light.pos.y - hit_point.y,
        light.pos.z - hit_point.z,
    };
    const float  light_dist = sqrtf(dot3(light_vec, light_vec));
    const float3 light_dir  = normalize3(light_vec);
    const float  cosine     = fmaxf(0.f, dot3(normal, light_dir));

    // [K3-T56] TODO [REQUIRED] spawn shadow ray (depth=1)
    unsigned int sp0 = 0, sp1 = 0, sp2 = 0;
    unsigned int sp3 = 1;  // [K3-T57] depth = 1 (shadow ray)

    optixTrace(
        params.handle,
        hit_point, light_dir,
        1e-4f,             // [K3-T58] tmin (avoid self-intersection)
        light_dist - 1e-3f,// [K3-T59] tmax (do not go past the light)
        0.f,
        OptixVisibilityMask(255),
        OPTIX_RAY_FLAG_TERMINATE_ON_FIRST_HIT | OPTIX_RAY_FLAG_DISABLE_CLOSESTHIT,
        0, 1, 0,
        sp0, sp1, sp2, sp3);

    // [K3-T60] sp3 == 2 means shadow ray missed (light visible); otherwise hit something (occluded)
    const float visibility = (sp3 == 2) ? 1.f : 0.f;

    // [K3-T61] TODO [REQUIRED] Lambertian shading
    const float3& base = hg->base_color;
    set_payload_rgb(
        base.x * light.intensity.x * cosine * visibility,
        base.y * light.intensity.y * cosine * visibility,
        base.z * light.intensity.z * cosine * visibility);
    set_payload_depth(0);

    // [K3-T62] TODO [ADVANCED] Phong specular term
    // [K3-T63] TODO [ADVANCED] loop over multiple lights and accumulate
}

// ============================================================================
// [K3-T64] __intersection__sphere (advanced skeleton, disabled by default)
// [K3-T65]
// [K3-T66] When CMakeLists.txt enables custom primitives, the host side
// [K3-T67] must add a GAS of OPTIX_BUILD_INPUT_TYPE_CUSTOM_PRIMITIVES.
// [K3-T68]
// [K3-T69] TODO [ADVANCED] step 3: compute ray-sphere intersection t,
// [K3-T70]                 then call optixReportIntersection
// ============================================================================
// extern "C" __global__ void __intersection__sphere() {
//     float3 center = {0.f, 0.f, 0.f};
//     float  radius = 0.3f;
//
//     float3 orig = optixGetObjectRayOrigin();
//     float3 dir  = optixGetObjectRayDirection();
//     float3 oc   = {orig.x - center.x, orig.y - center.y, orig.z - center.z};
//
//     float a = dot3(dir, dir);
//     float b = 2.f * dot3(oc, dir);
//     float c = dot3(oc, oc) - radius * radius;
//     float disc = b*b - 4.f*a*c;
//     if (disc < 0.f) return;
//
//     float t = (-b - sqrtf(disc)) / (2.f * a);
//     if (t < optixGetRayTmin() || t > optixGetRayTmax()) {
//         t = (-b + sqrtf(disc)) / (2.f * a);
//     }
//     if (t < optixGetRayTmin() || t > optixGetRayTmax()) return;
//
//     // TODO [ADVANCED] optixReportIntersection(t, 0 /* hit_kind */);
// }
