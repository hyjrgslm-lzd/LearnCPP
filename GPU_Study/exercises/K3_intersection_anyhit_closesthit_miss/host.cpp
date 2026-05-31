// K3_intersection_anyhit_closesthit_miss/host.cpp
// ============================================================================
// [K3-T80] Exercise K3: full demo of the five program types
// [K3-T81]
// [K3-T82] Goals:
// [K3-T83]   - raygen: spawn primary rays (depth=0)
// [K3-T84]   - any-hit: transparency / alpha-test skeleton
// [K3-T85]   - closest-hit: Lambertian direct lighting + shadow rays
// [K3-T86]   - miss: check depth, return bg color or unoccluded flag
// [K3-T87]   - intersection: custom sphere intersection (advanced, skeleton ready)
// [K3-T88]
// [K3-T89] New compared to K2:
// [K3-T90]   - PointLight constant
// [K3-T91]   - RayPayload (rgb + depth)
// [K3-T92]   - any-hit program group
// [K3-T93]   - shadow-ray payload
// ============================================================================

#ifdef GPU_STUDY_NO_OPTIX
#include <cstdio>
int main() {
    // [K3-T94]
    std::puts("[K3] GPU_STUDY_NO_OPTIX=1: OptiX SDK is not installed.");
    // [K3-T95]
    std::puts("     See K1/README.md and re-configure CMake after installation.");
    return 0;
}
#else

#include <cuda.h>
#include <cuda_runtime.h>
#include <optix.h>
#include <optix_function_table_definition.h>
#include <optix_stubs.h>

#include "common/cuda_check.cuh"
#include "common/device_info.cuh"
#include "common/nvtx_range.cuh"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>

// ============================================================================
// [K3-T96] OPTIX_CHECK
// ============================================================================
#define OPTIX_CHECK(expr)                                                       \
    do {                                                                        \
        OptixResult _r = (expr);                                                \
        if (_r != OPTIX_SUCCESS) {                                              \
            std::fprintf(stderr,                                                \
                "OptiX Error [%d]: %s\n  expr: %s\n  file: %s:%d\n",          \
                static_cast<int>(_r), optixGetErrorString(_r),                  \
                #expr, __FILE__, __LINE__);                                     \
            std::abort();                                                       \
        }                                                                       \
    } while (0)

static void optix_log_callback(unsigned int level, const char* tag,
                                const char* msg, void*) {
    std::fprintf(stderr, "[OptiX][%u][%s] %s\n", level, tag, msg);
}

static std::vector<char> read_binary_file(const char* path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) {
        // [K3-T97]
        std::fprintf(stderr, "Cannot open: %s\n", path); std::abort();
    }
    auto sz = static_cast<std::size_t>(f.tellg());
    f.seekg(0);
    std::vector<char> buf(sz);
    f.read(buf.data(), static_cast<std::streamsize>(sz));
    return buf;
}

// ============================================================================
// [K3-T98] Shared structs (must match programs.cu)
// ============================================================================

// [K3-T99] TODO [REQUIRED] step 1: PointLight definition
struct PointLight {
    float3 pos;        // [K3-T100] world coordinates
    float3 intensity;  // [K3-T101] per-channel intensity (linear HDR)
};

struct Params {
    float4*                output;
    unsigned int           width;
    unsigned int           height;
    OptixTraversableHandle handle;
    PointLight             light;  // [K3-T102] single point light
};

// ============================================================================
// [K3-T103] SBT record templates
// ============================================================================
template <typename T>
struct alignas(OPTIX_SBT_RECORD_ALIGNMENT) SbtRecord {
    char header[OPTIX_SBT_RECORD_HEADER_SIZE];
    T    data;
};

struct RayGenData  { unsigned int pad; };
struct MissData    { float3 bg_color; };
struct HitGroupData {
    float3* vertices;
    uint3*  indices;
    float3  base_color;  // [K3-T104] Lambertian intrinsic color
};

using RayGenRecord   = SbtRecord<RayGenData>;
using MissRecord     = SbtRecord<MissData>;
using HitGroupRecord = SbtRecord<HitGroupData>;

// ============================================================================
// [K3-T105] Cornell Box geometry (reused from K2)
// ============================================================================
static const float3 cornell_vertices[] = {
    {-1.f, -1.f, -1.f}, { 1.f, -1.f, -1.f},
    { 1.f,  1.f, -1.f}, {-1.f,  1.f, -1.f},
    {-1.f, -1.f,  1.f}, { 1.f, -1.f,  1.f},
    { 1.f,  1.f,  1.f}, {-1.f,  1.f,  1.f},
};
static const uint3 cornell_indices[] = {
    {0, 1, 2}, {0, 2, 3},  // [K3-T106] back wall (white)
    {0, 1, 5}, {0, 5, 4},  // [K3-T107] floor (white)
    {3, 7, 6}, {3, 6, 2},  // [K3-T108] ceiling (white)
    {0, 4, 7}, {0, 7, 3},  // [K3-T109] left wall (red)
    {1, 2, 6}, {1, 6, 5},  // [K3-T110] right wall (blue)
};
static constexpr unsigned int NUM_TRIANGLES =
    static_cast<unsigned int>(sizeof(cornell_indices) / sizeof(cornell_indices[0]));

// [K3-T111] Material color matching the triangle ordering above (5 face groups, 2 tris each)
static const float3 face_colors[] = {
    {0.8f, 0.8f, 0.8f}, {0.8f, 0.8f, 0.8f},  // [K3-T112] back wall white
    {0.8f, 0.8f, 0.8f}, {0.8f, 0.8f, 0.8f},  // [K3-T113] floor white
    {0.8f, 0.8f, 0.8f}, {0.8f, 0.8f, 0.8f},  // [K3-T114] ceiling white
    {0.8f, 0.1f, 0.1f}, {0.8f, 0.1f, 0.1f},  // [K3-T115] left wall red
    {0.1f, 0.1f, 0.8f}, {0.1f, 0.1f, 0.8f},  // [K3-T116] right wall blue
};

// ============================================================================
// [K3-T117] write PPM
// ============================================================================
static void write_ppm(const char* path, const float4* px, int w, int h) {
    std::ofstream f(path, std::ios::binary);
    f << "P6\n" << w << " " << h << "\n255\n";
    for (int i = 0; i < w * h; ++i) {
        auto b = [](float v) -> unsigned char {
            v = v < 0.f ? 0.f : (v > 1.f ? 1.f : v);
            return static_cast<unsigned char>(v * 255.f + .5f);
        };
        unsigned char rgb[3] = {b(px[i].x), b(px[i].y), b(px[i].z)};
        f.write(reinterpret_cast<const char*>(rgb), 3);
    }
    // [K3-T118]
    std::printf("  Output written: %s\n", path);
}

// ============================================================================
// [K3-T119] main
// ============================================================================
int main() {
    // [K3-T120]
    std::puts("[K3] intersection_anyhit_closesthit_miss");
    print_device_info();

    constexpr int WIDTH  = 1024;
    constexpr int HEIGHT = 1024;

    // [K3-T121] === CUDA context ===
    CU_CHECK(cuInit(0));
    CUdevice cu_device = 0;
    CU_CHECK(cuDeviceGet(&cu_device, 0));
    CUcontext cu_ctx = nullptr;
    CU_CHECK(cuCtxCreate(&cu_ctx, 0, cu_device));

    // [K3-T122] === OptiX init ===
    OPTIX_CHECK(optixInit());
    OptixDeviceContext optix_ctx = nullptr;
    {
        OptixDeviceContextOptions opts{};
        opts.logCallbackFunction = optix_log_callback;
        opts.logCallbackLevel    = 4;
        OPTIX_CHECK(optixDeviceContextCreate(cu_ctx, &opts, &optix_ctx));
    }

    // [K3-T123] === upload geometry ===
    float3* d_vertices = nullptr;
    uint3*  d_indices  = nullptr;
    CUDA_CHECK(cudaMalloc(&d_vertices, sizeof(cornell_vertices)));
    CUDA_CHECK(cudaMemcpy(d_vertices, cornell_vertices,
        sizeof(cornell_vertices), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMalloc(&d_indices, sizeof(cornell_indices)));
    CUDA_CHECK(cudaMemcpy(d_indices, cornell_indices,
        sizeof(cornell_indices), cudaMemcpyHostToDevice));

    // [K3-T124] === GAS build ===
    OptixTraversableHandle gas_handle = 0;
    CUdeviceptr d_gas_output = 0;
    {
        OptixAccelBuildOptions accel_opts{};
        accel_opts.buildFlags = OPTIX_BUILD_FLAG_NONE;
        accel_opts.operation  = OPTIX_BUILD_OPERATION_BUILD;

        OptixBuildInput tri_input{};
        tri_input.type = OPTIX_BUILD_INPUT_TYPE_TRIANGLES;
        CUdeviceptr d_vert_ptr = reinterpret_cast<CUdeviceptr>(d_vertices);
        tri_input.triangleArray.vertexBuffers       = &d_vert_ptr;
        tri_input.triangleArray.numVertices         =
            static_cast<unsigned int>(sizeof(cornell_vertices) / sizeof(float3));
        tri_input.triangleArray.vertexFormat        = OPTIX_VERTEX_FORMAT_FLOAT3;
        tri_input.triangleArray.vertexStrideInBytes = sizeof(float3);
        tri_input.triangleArray.indexBuffer         = reinterpret_cast<CUdeviceptr>(d_indices);
        tri_input.triangleArray.numIndexTriplets    = NUM_TRIANGLES;
        tri_input.triangleArray.indexFormat         = OPTIX_INDICES_FORMAT_UNSIGNED_INT3;
        tri_input.triangleArray.indexStrideInBytes  = sizeof(uint3);
        // [K3-T125] one SBT record per triangle (so we can vary material color per face)
        static const unsigned int sbt_flags[10] = {
            OPTIX_GEOMETRY_FLAG_NONE, OPTIX_GEOMETRY_FLAG_NONE,
            OPTIX_GEOMETRY_FLAG_NONE, OPTIX_GEOMETRY_FLAG_NONE,
            OPTIX_GEOMETRY_FLAG_NONE, OPTIX_GEOMETRY_FLAG_NONE,
            OPTIX_GEOMETRY_FLAG_NONE, OPTIX_GEOMETRY_FLAG_NONE,
            OPTIX_GEOMETRY_FLAG_NONE, OPTIX_GEOMETRY_FLAG_NONE,
        };
        tri_input.triangleArray.flags            = sbt_flags;
        tri_input.triangleArray.numSbtRecords    = NUM_TRIANGLES;  // [K3-T126] 1 record per triangle

        OptixAccelBufferSizes buf_sizes{};
        OPTIX_CHECK(optixAccelComputeMemoryUsage(
            optix_ctx, &accel_opts, &tri_input, 1, &buf_sizes));
        CUdeviceptr d_temp = 0;
        CU_CHECK(cuMemAlloc(&d_temp, buf_sizes.tempSizeInBytes));
        CU_CHECK(cuMemAlloc(&d_gas_output, buf_sizes.outputSizeInBytes));
        OPTIX_CHECK(optixAccelBuild(
            optix_ctx, nullptr,
            &accel_opts, &tri_input, 1,
            d_temp, buf_sizes.tempSizeInBytes,
            d_gas_output, buf_sizes.outputSizeInBytes,
            &gas_handle, nullptr, 0));
        CUDA_CHECK(cudaDeviceSynchronize());
        CU_CHECK(cuMemFree(d_temp));
        // [K3-T127]
        std::puts("  GAS build complete");
    }

    // [K3-T128] === load .optixir, create module ===
    auto optixir_data = read_binary_file(K3_OPTIXIR_PATH);

    // [K3-T129] TODO [REQUIRED] step 5: payloadValues = 4 (rgb 3 uints + depth 1 uint)
    OptixPipelineCompileOptions pipeline_opts{};
    pipeline_opts.usesMotionBlur                   = 0;
    pipeline_opts.traversableGraphFlags            = OPTIX_TRAVERSABLE_GRAPH_FLAG_ALLOW_SINGLE_GAS;
    pipeline_opts.numPayloadValues                 = 4;  // [K3-T130] R, G, B, depth
    pipeline_opts.numAttributeValues               = 2;  // [K3-T131] barycentrics
    pipeline_opts.exceptionFlags                   = OPTIX_EXCEPTION_FLAG_NONE;
    pipeline_opts.pipelineLaunchParamsVariableName = "params";

    char log_buf[4096]; size_t log_size = sizeof(log_buf);

    OptixModuleCompileOptions module_opts{};
    module_opts.maxRegisterCount = OPTIX_COMPILE_DEFAULT_MAX_REGISTER_COUNT;
    module_opts.optLevel         = OPTIX_COMPILE_OPTIMIZATION_DEFAULT;
    module_opts.debugLevel       = OPTIX_COMPILE_DEBUG_LEVEL_MINIMAL;

    OptixModule optix_module = nullptr;
    OPTIX_CHECK(optixModuleCreate(
        optix_ctx, &module_opts, &pipeline_opts,
        optixir_data.data(), optixir_data.size(),
        log_buf, &log_size, &optix_module));
    if (log_size > 1) std::fprintf(stderr, "[module] %s\n", log_buf);

    // [K3-T132] === Program groups ===
    OptixProgramGroup pg_raygen   = nullptr;
    OptixProgramGroup pg_miss     = nullptr;
    OptixProgramGroup pg_hitgroup = nullptr;

    // [K3-T133] raygen
    {
        OptixProgramGroupDesc d{};
        d.kind = OPTIX_PROGRAM_GROUP_KIND_RAYGEN;
        d.raygen.module = optix_module;
        d.raygen.entryFunctionName = "__raygen__principal";
        OptixProgramGroupOptions o{};
        log_size = sizeof(log_buf);
        OPTIX_CHECK(optixProgramGroupCreate(optix_ctx, &d, 1, &o,
            log_buf, &log_size, &pg_raygen));
    }
    // [K3-T134] miss
    {
        OptixProgramGroupDesc d{};
        d.kind = OPTIX_PROGRAM_GROUP_KIND_MISS;
        d.miss.module = optix_module;
        d.miss.entryFunctionName = "__miss__background";
        OptixProgramGroupOptions o{};
        log_size = sizeof(log_buf);
        OPTIX_CHECK(optixProgramGroupCreate(optix_ctx, &d, 1, &o,
            log_buf, &log_size, &pg_miss));
    }
    // [K3-T135] TODO [REQUIRED] step 9: hitgroup contains any-hit + closest-hit
    {
        OptixProgramGroupDesc d{};
        d.kind = OPTIX_PROGRAM_GROUP_KIND_HITGROUP;
        d.hitgroup.moduleCH            = optix_module;
        d.hitgroup.entryFunctionNameCH = "__closesthit__radiance";
        d.hitgroup.moduleAH            = optix_module;
        d.hitgroup.entryFunctionNameAH = "__anyhit__opaque";
        d.hitgroup.moduleIS            = nullptr;  // [K3-T136] use built-in triangle intersection
        d.hitgroup.entryFunctionNameIS = nullptr;
        OptixProgramGroupOptions o{};
        log_size = sizeof(log_buf);
        OPTIX_CHECK(optixProgramGroupCreate(optix_ctx, &d, 1, &o,
            log_buf, &log_size, &pg_hitgroup));
    }
    // [K3-T137]
    std::puts("  program groups created");

    // [K3-T138] === pipeline ===
    OptixPipeline pipeline = nullptr;
    {
        // [K3-T139] TODO [REQUIRED] step 9b: maxTraceDepth = 2 (primary + shadow)
        OptixPipelineLinkOptions link_opts{};
        link_opts.maxTraceDepth = 2;
        OptixProgramGroup pgs[3] = {pg_raygen, pg_miss, pg_hitgroup};
        log_size = sizeof(log_buf);
        OPTIX_CHECK(optixPipelineCreate(
            optix_ctx, &pipeline_opts, &link_opts,
            pgs, 3, log_buf, &log_size, &pipeline));
        if (log_size > 1) std::fprintf(stderr, "[pipeline] %s\n", log_buf);
        // [K3-T140]
        std::puts("  pipeline created");
    }

    // [K3-T141] === output buffer ===
    float4* d_output = nullptr;
    CUDA_CHECK(cudaMalloc(&d_output,
        static_cast<size_t>(WIDTH) * HEIGHT * sizeof(float4)));

    // [K3-T142] === Params ===
    Params h_params{};
    h_params.output        = d_output;
    h_params.width         = static_cast<unsigned int>(WIDTH);
    h_params.height        = static_cast<unsigned int>(HEIGHT);
    h_params.handle        = gas_handle;
    // [K3-T143] TODO [REQUIRED] step 1: set point light position and intensity
    h_params.light.pos       = {0.f, 0.8f, 0.f};  // [K3-T144] near the ceiling
    h_params.light.intensity = {1.5f, 1.5f, 1.5f};

    CUdeviceptr d_params = 0;
    CU_CHECK(cuMemAlloc(&d_params, sizeof(Params)));
    CU_CHECK(cuMemcpyHtoD(d_params, &h_params, sizeof(Params)));

    // [K3-T145] === SBT ===
    // [K3-T146] raygen (1 record)
    RayGenRecord h_rg{};
    OPTIX_CHECK(optixSbtRecordPackHeader(pg_raygen, &h_rg));
    CUdeviceptr d_rg = 0;
    CU_CHECK(cuMemAlloc(&d_rg, sizeof(RayGenRecord)));
    CU_CHECK(cuMemcpyHtoD(d_rg, &h_rg, sizeof(RayGenRecord)));

    // [K3-T147] miss (1 record)
    MissRecord h_ms{};
    OPTIX_CHECK(optixSbtRecordPackHeader(pg_miss, &h_ms));
    h_ms.data.bg_color = {0.2f, 0.3f, 0.8f};
    CUdeviceptr d_ms = 0;
    CU_CHECK(cuMemAlloc(&d_ms, sizeof(MissRecord)));
    CU_CHECK(cuMemcpyHtoD(d_ms, &h_ms, sizeof(MissRecord)));

    // [K3-T148] TODO [REQUIRED] step 9c: hitgroup (NUM_TRIANGLES records, one per triangle)
    std::vector<HitGroupRecord> h_hg_vec(NUM_TRIANGLES);
    for (unsigned int i = 0; i < NUM_TRIANGLES; ++i) {
        OPTIX_CHECK(optixSbtRecordPackHeader(pg_hitgroup, &h_hg_vec[i]));
        h_hg_vec[i].data.vertices   = d_vertices;
        h_hg_vec[i].data.indices    = d_indices;
        h_hg_vec[i].data.base_color = face_colors[i];
    }
    CUdeviceptr d_hg = 0;
    size_t hg_total = NUM_TRIANGLES * sizeof(HitGroupRecord);
    CU_CHECK(cuMemAlloc(&d_hg, hg_total));
    CU_CHECK(cuMemcpyHtoD(d_hg, h_hg_vec.data(), hg_total));

    OptixShaderBindingTable sbt{};
    sbt.raygenRecord                = d_rg;
    sbt.missRecordBase              = d_ms;
    sbt.missRecordStrideInBytes     = sizeof(MissRecord);
    sbt.missRecordCount             = 1;
    sbt.hitgroupRecordBase          = d_hg;
    sbt.hitgroupRecordStrideInBytes = sizeof(HitGroupRecord);
    sbt.hitgroupRecordCount         = NUM_TRIANGLES;
    // [K3-T149]
    std::puts("  SBT built");

    // [K3-T150] === optixLaunch ===
    CUstream stream = nullptr;
    CUDA_CHECK(cudaStreamCreate(reinterpret_cast<cudaStream_t*>(&stream)));
    {
        NVTX_RANGE("K3::optixLaunch");
        OPTIX_CHECK(optixLaunch(
            pipeline, stream,
            d_params, sizeof(Params),
            &sbt,
            static_cast<unsigned int>(WIDTH),
            static_cast<unsigned int>(HEIGHT), 1u));
        CUDA_CHECK(cudaStreamSynchronize(reinterpret_cast<cudaStream_t>(stream)));
    }
    // [K3-T151]
    std::puts("  optixLaunch finished");

    // [K3-T152] === read back result + write PPM ===
    std::vector<float4> h_output(static_cast<size_t>(WIDTH) * HEIGHT);
    CUDA_CHECK(cudaMemcpy(h_output.data(), d_output,
        h_output.size() * sizeof(float4), cudaMemcpyDeviceToHost));
    write_ppm("output_k3.ppm", h_output.data(), WIDTH, HEIGHT);

    // [K3-T153] TODO [ADVANCED] custom intersection (sphere light)
    // [K3-T154] TODO [ADVANCED] Phong shading (add specular term)
    // [K3-T155] TODO [ADVANCED] multiple point lights, accumulate

    // [K3-T156] === cleanup ===
    CUDA_CHECK(cudaStreamDestroy(reinterpret_cast<cudaStream_t>(stream)));
    CU_CHECK(cuMemFree(d_params));
    CU_CHECK(cuMemFree(d_rg));
    CU_CHECK(cuMemFree(d_ms));
    CU_CHECK(cuMemFree(d_hg));
    CUDA_CHECK(cudaFree(d_output));
    CUDA_CHECK(cudaFree(d_vertices));
    CUDA_CHECK(cudaFree(d_indices));
    CU_CHECK(cuMemFree(d_gas_output));
    OPTIX_CHECK(optixPipelineDestroy(pipeline));
    OPTIX_CHECK(optixProgramGroupDestroy(pg_hitgroup));
    OPTIX_CHECK(optixProgramGroupDestroy(pg_miss));
    OPTIX_CHECK(optixProgramGroupDestroy(pg_raygen));
    OPTIX_CHECK(optixModuleDestroy(optix_module));
    OPTIX_CHECK(optixDeviceContextDestroy(optix_ctx));
    CU_CHECK(cuCtxDestroy(cu_ctx));

    // [K3-T157]
    std::puts("[K3] done.");
    return 0;
}

#endif // GPU_STUDY_NO_OPTIX
