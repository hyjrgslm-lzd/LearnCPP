// K4_optix_denoiser_and_cuda_interop/programs.cu
// ============================================================================
// [K4-T01] Exercise K4 OptiX device programs - Monte Carlo path tracing
// [K4-T02]                                       + albedo/normal guide buffers
// [K4-T03]
// [K4-T04] Program types:
// [K4-T05]   __raygen__mc            - 32 spp MC path tracing (random dirs, accum/avg)
// [K4-T06]   __miss__background      - bg color (depth=0) or shadow flag (depth=1)
// [K4-T07]   __closesthit__radiance  - Lambertian + writes albedo/normal buffers
// ============================================================================

#include <optix.h>
#include <cuda_runtime.h>

// ============================================================================
// [K4-T08] Shared structs (must match host.cpp)
// ============================================================================
struct PointLight {
    float3 pos;
    float3 intensity;
};

struct Params {
    float4*                color_buffer;
    float4*                albedo_buffer;
    float4*                normal_buffer;
    unsigned int           width;
    unsigned int           height;
    OptixTraversableHandle handle;
    PointLight             light;
    unsigned int           samples_per_pixel;
    unsigned int           seed;
};

struct HitGroupData {
    float3* vertices;
    uint3*  indices;
    float3  base_color;
};

extern "C" __constant__ Params params;

// ============================================================================
// [K4-T09] Simple LCG random number generator (device side)
// ============================================================================
__device__ __forceinline__ unsigned int lcg_step(unsigned int& state) {
    state = 1664525u * state + 1013904223u;
    return state;
}
__device__ __forceinline__ float rand01(unsigned int& state) {
    return static_cast<float>(lcg_step(state) & 0x00FFFFFFu) /
           static_cast<float>(0x01000000u);
}

// ============================================================================
// [K4-T10] float3 helpers
// ============================================================================
__device__ __forceinline__ float3 normalize3(float3 v) {
    float l = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
    return l > 0.f ? float3{v.x/l, v.y/l, v.z/l} : float3{0.f,0.f,0.f};
}
__device__ __forceinline__ float dot3(float3 a, float3 b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}
__device__ __forceinline__ float3 cross3(float3 a, float3 b) {
    return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x};
}

// ============================================================================
// [K4-T11] Payload helpers (4 slots: R, G, B, depth)
// ============================================================================
__device__ __forceinline__ void set_payload_rgb(float r, float g, float b) {
    optixSetPayload_0(__float_as_uint(r));
    optixSetPayload_1(__float_as_uint(g));
    optixSetPayload_2(__float_as_uint(b));
}
__device__ __forceinline__ void set_payload_depth(unsigned int d) {
    optixSetPayload_3(d);
}
__device__ __forceinline__ unsigned int get_payload_depth() {
    return optixGetPayload_3();
}

// ============================================================================
// [K4-T12] __raygen__mc
// [K4-T13]
// [K4-T14] TODO [REQUIRED] step 1:
// [K4-T15]   - outer loop runs samples_per_pixel times
// [K4-T16]   - jitter pixel coordinates each iteration (stratified jitter)
// [K4-T17]   - call optixTrace (depth=0)
// [K4-T18]   - accumulate color, divide by spp, write color_buffer
// ============================================================================
extern "C" __global__ void __raygen__mc() {
    const uint3 idx  = optixGetLaunchIndex();
    const uint3 dims = optixGetLaunchDimensions();

    const unsigned int pixel_id = idx.y * dims.x + idx.x;

    // [K4-T19] Init RNG state (pixel ID xor'd with seed-derived perturbation)
    unsigned int rng = pixel_id ^ (params.seed * 1234567u);

    const float3 eye   = {0.f, 0.f, 3.f};
    const float3 lower = {-1.f, -1.f, 2.f};
    const float3 right = { 2.f,  0.f, 0.f};
    const float3 up    = { 0.f,  2.f, 0.f};

    float3 accum = {0.f, 0.f, 0.f};

    // [K4-T20] TODO [REQUIRED] Monte Carlo sampling loop
    for (unsigned int s = 0; s < params.samples_per_pixel; ++s) {
        // [K4-T21] in-pixel random jitter (stratified jitter)
        const float u = (static_cast<float>(idx.x) + rand01(rng)) /
                        static_cast<float>(dims.x);
        const float v = (static_cast<float>(idx.y) + rand01(rng)) /
                        static_cast<float>(dims.y);

        const float3 target = {
            lower.x + u*right.x + v*up.x,
            lower.y + u*right.y + v*up.y,
            lower.z + u*right.z + v*up.z,
        };
        const float3 dir_n = normalize3({
            target.x - eye.x, target.y - eye.y, target.z - eye.z
        });

        unsigned int p0 = 0, p1 = 0, p2 = 0;
        unsigned int p3 = 0;  // [K4-T22] depth = 0

        optixTrace(
            params.handle,
            eye, dir_n,
            1e-3f, 1e16f, 0.f,
            OptixVisibilityMask(255),
            OPTIX_RAY_FLAG_NONE,
            0, 1, 0,
            p0, p1, p2, p3);

        accum.x += __uint_as_float(p0);
        accum.y += __uint_as_float(p1);
        accum.z += __uint_as_float(p2);
    }

    // [K4-T23] Average over spp samples
    const float inv_spp = 1.f / static_cast<float>(params.samples_per_pixel);
    params.color_buffer[pixel_id] = make_float4(
        accum.x * inv_spp,
        accum.y * inv_spp,
        accum.z * inv_spp,
        1.f);

    // [K4-T24] TODO [ADVANCED] adaptive sampling (more samples in high-variance regions)
}

// ============================================================================
// [K4-T25] __miss__background
// ============================================================================
extern "C" __global__ void __miss__background() {
    const unsigned int depth = get_payload_depth();
    if (depth == 0) {
        const float3 dir = normalize3(optixGetWorldRayDirection());
        float t = 0.5f * (dir.y + 1.f);
        set_payload_rgb(
            0.2f*(1.f-t) + 0.5f*t,
            0.3f*(1.f-t) + 0.7f*t,
            0.8f*(1.f-t) + 1.0f*t);
        set_payload_depth(0);
    } else {
        set_payload_depth(2);  // [K4-T26] shadow miss -> unoccluded flag
    }
}

// ============================================================================
// [K4-T27] __closesthit__radiance
// [K4-T28]
// [K4-T29] TODO [REQUIRED] step 4:
// [K4-T30]   - compute normal -> write to normal_buffer (float4, xyz=normal, w=1)
// [K4-T31]   - take base_color -> write to albedo_buffer (float4, xyz=albedo, w=1)
// [K4-T32]   - compute Lambertian + shadow ray (reuse K3 logic)
// ============================================================================
extern "C" __global__ void __closesthit__radiance() {
    const HitGroupData* hg =
        reinterpret_cast<const HitGroupData*>(optixGetSbtDataPointer());

    const unsigned int prim_idx = optixGetPrimitiveIndex();
    const uint3 tri = hg->indices[prim_idx];
    const float3& v0 = hg->vertices[tri.x];
    const float3& v1 = hg->vertices[tri.y];
    const float3& v2 = hg->vertices[tri.z];

    const float3 e1 = {v1.x-v0.x, v1.y-v0.y, v1.z-v0.z};
    const float3 e2 = {v2.x-v0.x, v2.y-v0.y, v2.z-v0.z};
    const float3 normal = normalize3(cross3(e1, e2));

    const float  t_hit    = optixGetRayTmax();
    const float3 orig     = optixGetWorldRayOrigin();
    const float3 dir      = optixGetWorldRayDirection();
    const float3 hit_pt   = {
        orig.x + t_hit*dir.x + normal.x*1e-4f,
        orig.y + t_hit*dir.y + normal.y*1e-4f,
        orig.z + t_hit*dir.z + normal.z*1e-4f,
    };

    // [K4-T33] TODO [REQUIRED] step 4: write albedo / normal guide buffers
    const uint3 launch_idx  = optixGetLaunchIndex();
    const uint3 launch_dims = optixGetLaunchDimensions();
    const unsigned int pixel_id = launch_idx.y * launch_dims.x + launch_idx.x;

    params.albedo_buffer[pixel_id] = make_float4(
        hg->base_color.x, hg->base_color.y, hg->base_color.z, 1.f);
    params.normal_buffer[pixel_id] = make_float4(
        normal.x * 0.5f + 0.5f,
        normal.y * 0.5f + 0.5f,
        normal.z * 0.5f + 0.5f,
        1.f);

    // [K4-T34] Lambertian + shadow
    const PointLight& light = params.light;
    const float3 light_vec = {
        light.pos.x - hit_pt.x,
        light.pos.y - hit_pt.y,
        light.pos.z - hit_pt.z,
    };
    const float  light_dist = sqrtf(dot3(light_vec, light_vec));
    const float3 light_dir  = normalize3(light_vec);
    const float  cosine     = fmaxf(0.f, dot3(normal, light_dir));

    unsigned int sp0=0, sp1=0, sp2=0, sp3=1;
    optixTrace(
        params.handle,
        hit_pt, light_dir,
        1e-4f, light_dist - 1e-3f, 0.f,
        OptixVisibilityMask(255),
        OPTIX_RAY_FLAG_TERMINATE_ON_FIRST_HIT | OPTIX_RAY_FLAG_DISABLE_CLOSESTHIT,
        0, 1, 0,
        sp0, sp1, sp2, sp3);

    const float vis = (sp3 == 2) ? 1.f : 0.f;
    const float3& bc = hg->base_color;
    set_payload_rgb(
        bc.x * light.intensity.x * cosine * vis,
        bc.y * light.intensity.y * cosine * vis,
        bc.z * light.intensity.z * cosine * vis);
    set_payload_depth(0);

    // [K4-T35] TODO [ADVANCED] implement multi-bounce path tracing (recursive optixTrace)
    // [K4-T36] TODO [ADVANCED] importance sampling on the hemisphere (replace uniform random)
}
