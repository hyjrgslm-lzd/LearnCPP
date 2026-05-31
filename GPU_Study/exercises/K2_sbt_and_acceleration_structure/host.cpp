// K2_sbt_and_acceleration_structure/host.cpp
// ============================================================================
// [K2-T60] Exercise K2: three-segment SBT + GAS acceleration structure
// [K2-T61]
// [K2-T62] Goals:
// [K2-T63]   - build a GAS (Geometry Acceleration Structure) from triangle mesh
// [K2-T64]     (Cornell Box)
// [K2-T65]   - three-segment SBT: raygen / miss / hitgroup
// [K2-T66]   - closest-hit computes normal and returns a normal-vis color via payload
// [K2-T67]   - output output_k2.ppm
// [K2-T68]
// [K2-T69] New compared to K1: GAS build + IAS (optional) + miss/hitgroup PG
// ============================================================================

#ifdef GPU_STUDY_NO_OPTIX
#include <cstdio>
int main() {
    // [K2-T70]
    std::puts("[K2] GPU_STUDY_NO_OPTIX=1: OptiX SDK is not installed.");
    // [K2-T71]
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

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <vector>

// ============================================================================
// [K2-T72] OPTIX_CHECK
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

// ============================================================================
// [K2-T73] file reader
// ============================================================================
static std::vector<char> read_binary_file(const char* path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) {
        // [K2-T74]
        std::fprintf(stderr, "Cannot open: %s\n", path); std::abort();
    }
    auto sz = static_cast<std::size_t>(f.tellg());
    f.seekg(0);
    std::vector<char> buf(sz);
    f.read(buf.data(), static_cast<std::streamsize>(sz));
    return buf;
}

// ============================================================================
// [K2-T75] Params - shared with programs.cu
// ============================================================================
struct Params {
    float4*             output;
    unsigned int        width;
    unsigned int        height;
    OptixTraversableHandle handle;  // [K2-T76] GAS / IAS traversable handle
};

// ============================================================================
// [K2-T77] SBT record templates
// ============================================================================
template <typename T>
struct alignas(OPTIX_SBT_RECORD_ALIGNMENT) SbtRecord {
    char header[OPTIX_SBT_RECORD_HEADER_SIZE];
    T    data;
};

struct RayGenData  { unsigned int pad; };
struct MissData    { float3 bg_color; };
struct HitGroupData {
    float3* vertices;  // [K2-T78] device-side vertex array pointer (used for normal calc)
    uint3*  indices;   // [K2-T79] device-side index array pointer
};

using RayGenRecord  = SbtRecord<RayGenData>;
using MissRecord    = SbtRecord<MissData>;
using HitGroupRecord = SbtRecord<HitGroupData>;

// ============================================================================
// [K2-T80] Cornell Box geometry (8 vertices, 12 triangles)
// [K2-T81] Coordinate range [-1, 1]^3; camera looks towards -z from z=3
// ============================================================================

// [K2-T82] TODO [REQUIRED] step 1: define Cornell Box vertices and indices.
// [K2-T83] Skeleton has 8 vertices + 12 faces (2 triangles each); fill in if needed.
static const float3 cornell_vertices[] = {
    // [K2-T84] back wall (z = -1)
    {-1.f, -1.f, -1.f}, { 1.f, -1.f, -1.f},
    { 1.f,  1.f, -1.f}, {-1.f,  1.f, -1.f},
    // [K2-T85] front wall (z = 1; usually invisible, behind the camera)
    {-1.f, -1.f,  1.f}, { 1.f, -1.f,  1.f},
    { 1.f,  1.f,  1.f}, {-1.f,  1.f,  1.f},
};

static const uint3 cornell_indices[] = {
    // [K2-T86] back wall (white)
    {0, 1, 2}, {0, 2, 3},
    // [K2-T87] floor (white)
    {0, 1, 5}, {0, 5, 4},
    // [K2-T88] ceiling (white)
    {3, 7, 6}, {3, 6, 2},
    // [K2-T89] left wall (red)
    {0, 4, 7}, {0, 7, 3},
    // [K2-T90] right wall (blue)
    {1, 2, 6}, {1, 6, 5},
    // [K2-T91] TODO [REQUIRED] add front wall if desired (usually omitted to avoid blocking the camera)
};
static constexpr unsigned int NUM_TRIANGLES =
    static_cast<unsigned int>(sizeof(cornell_indices) / sizeof(cornell_indices[0]));

// ============================================================================
// [K2-T92] write PPM
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
    // [K2-T93]
    std::printf("  Output written: %s\n", path);
}

// ============================================================================
// [K2-T94] main
// ============================================================================
int main() {
    // [K2-T95]
    std::puts("[K2] sbt_and_acceleration_structure");
    print_device_info();

    constexpr int WIDTH  = 1024;
    constexpr int HEIGHT = 1024;

    // [K2-T96] === CUDA context ===
    CU_CHECK(cuInit(0));
    CUdevice cu_device = 0;
    CU_CHECK(cuDeviceGet(&cu_device, 0));
    CUcontext cu_ctx = nullptr;
    CU_CHECK(cuCtxCreate(&cu_ctx, 0, cu_device));

    // [K2-T97] === OptiX init ===
    OPTIX_CHECK(optixInit());
    OptixDeviceContext optix_ctx = nullptr;
    {
        OptixDeviceContextOptions opts{};
        opts.logCallbackFunction = optix_log_callback;
        opts.logCallbackLevel    = 4;
        OPTIX_CHECK(optixDeviceContextCreate(cu_ctx, &opts, &optix_ctx));
    }

    // [K2-T98] === TODO [REQUIRED] step 1: upload geometry to device ===
    float3* d_vertices = nullptr;
    uint3*  d_indices  = nullptr;
    const size_t vert_bytes = sizeof(cornell_vertices);
    const size_t idx_bytes  = sizeof(cornell_indices);

    CUDA_CHECK(cudaMalloc(&d_vertices, vert_bytes));
    CUDA_CHECK(cudaMemcpy(d_vertices, cornell_vertices, vert_bytes, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMalloc(&d_indices, idx_bytes));
    CUDA_CHECK(cudaMemcpy(d_indices, cornell_indices, idx_bytes, cudaMemcpyHostToDevice));

    // [K2-T99] === TODO [REQUIRED] step 2: build GAS ===
    OptixTraversableHandle gas_handle = 0;
    CUdeviceptr d_gas_output = 0;
    {
        OptixAccelBuildOptions accel_opts{};
        accel_opts.buildFlags = OPTIX_BUILD_FLAG_NONE;
        accel_opts.operation  = OPTIX_BUILD_OPERATION_BUILD;

        // [K2-T100] TODO [REQUIRED] build OptixBuildInputTriangleArray
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

        static const unsigned int sbt_flags[1] = {OPTIX_GEOMETRY_FLAG_NONE};
        tri_input.triangleArray.flags            = sbt_flags;
        tri_input.triangleArray.numSbtRecords    = 1;

        // [K2-T101] TODO [REQUIRED] query buffer sizes
        OptixAccelBufferSizes buf_sizes{};
        OPTIX_CHECK(optixAccelComputeMemoryUsage(
            optix_ctx, &accel_opts, &tri_input, 1, &buf_sizes));

        CUdeviceptr d_temp = 0;
        CU_CHECK(cuMemAlloc(&d_temp, buf_sizes.tempSizeInBytes));
        CU_CHECK(cuMemAlloc(&d_gas_output, buf_sizes.outputSizeInBytes));

        // [K2-T102] TODO [REQUIRED] optixAccelBuild(...)
        OPTIX_CHECK(optixAccelBuild(
            optix_ctx, nullptr,
            &accel_opts, &tri_input, 1,
            d_temp, buf_sizes.tempSizeInBytes,
            d_gas_output, buf_sizes.outputSizeInBytes,
            &gas_handle, nullptr, 0));
        CUDA_CHECK(cudaDeviceSynchronize());

        CU_CHECK(cuMemFree(d_temp));
        // [K2-T103]
        std::puts("  GAS build complete");

        // [K2-T104] TODO [ADVANCED] use optixAccelCompact to compress GAS and save VRAM
    }

    // [K2-T105] === load .optixir, create module ===
    auto optixir_data = read_binary_file(K2_OPTIXIR_PATH);

    OptixPipelineCompileOptions pipeline_opts{};
    pipeline_opts.usesMotionBlur                   = 0;
    pipeline_opts.traversableGraphFlags            = OPTIX_TRAVERSABLE_GRAPH_FLAG_ALLOW_SINGLE_GAS;
    pipeline_opts.numPayloadValues                 = 3;  // [K2-T106] payload: R, G, B (3 uints)
    pipeline_opts.numAttributeValues               = 2;  // [K2-T107] barycentrics
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

    // [K2-T108] === TODO [REQUIRED] step 7a: create raygen program group ===
    OptixProgramGroup pg_raygen = nullptr;
    {
        OptixProgramGroupDesc d{};
        d.kind                     = OPTIX_PROGRAM_GROUP_KIND_RAYGEN;
        d.raygen.module            = optix_module;
        d.raygen.entryFunctionName = "__raygen__cam";
        OptixProgramGroupOptions o{};
        log_size = sizeof(log_buf);
        OPTIX_CHECK(optixProgramGroupCreate(optix_ctx, &d, 1, &o, log_buf, &log_size, &pg_raygen));
    }

    // [K2-T109] === TODO [REQUIRED] step 7b: create miss program group ===
    OptixProgramGroup pg_miss = nullptr;
    {
        OptixProgramGroupDesc d{};
        d.kind                   = OPTIX_PROGRAM_GROUP_KIND_MISS;
        d.miss.module            = optix_module;
        d.miss.entryFunctionName = "__miss__background";
        OptixProgramGroupOptions o{};
        log_size = sizeof(log_buf);
        OPTIX_CHECK(optixProgramGroupCreate(optix_ctx, &d, 1, &o, log_buf, &log_size, &pg_miss));
    }

    // [K2-T110] === TODO [REQUIRED] step 7c: create hitgroup program group ===
    OptixProgramGroup pg_hitgroup = nullptr;
    {
        OptixProgramGroupDesc d{};
        d.kind                         = OPTIX_PROGRAM_GROUP_KIND_HITGROUP;
        d.hitgroup.moduleCH            = optix_module;
        d.hitgroup.entryFunctionNameCH = "__closesthit__radiance";
        d.hitgroup.moduleAH            = nullptr;  // [K2-T111] K2 has no any-hit
        d.hitgroup.entryFunctionNameAH = nullptr;
        d.hitgroup.moduleIS            = nullptr;  // [K2-T112] use built-in triangle intersection
        d.hitgroup.entryFunctionNameIS = nullptr;
        OptixProgramGroupOptions o{};
        log_size = sizeof(log_buf);
        OPTIX_CHECK(optixProgramGroupCreate(optix_ctx, &d, 1, &o, log_buf, &log_size, &pg_hitgroup));
    }
    // [K2-T113]
    std::puts("  3 program groups created");

    // [K2-T114] === TODO [REQUIRED] step 9: build pipeline ===
    OptixPipeline pipeline = nullptr;
    {
        OptixPipelineLinkOptions link_opts{};
        link_opts.maxTraceDepth = 1;
        OptixProgramGroup pgs[3] = {pg_raygen, pg_miss, pg_hitgroup};
        log_size = sizeof(log_buf);
        OPTIX_CHECK(optixPipelineCreate(
            optix_ctx, &pipeline_opts, &link_opts,
            pgs, 3, log_buf, &log_size, &pipeline));
        if (log_size > 1) std::fprintf(stderr, "[pipeline] %s\n", log_buf);
        // [K2-T115]
        std::puts("  pipeline created");
    }

    // [K2-T116] === output buffer ===
    float4* d_output = nullptr;
    CUDA_CHECK(cudaMalloc(&d_output,
        static_cast<size_t>(WIDTH) * HEIGHT * sizeof(float4)));

    // [K2-T117] === Params ===
    Params h_params{};
    h_params.output = d_output;
    h_params.width  = static_cast<unsigned int>(WIDTH);
    h_params.height = static_cast<unsigned int>(HEIGHT);
    h_params.handle = gas_handle;

    CUdeviceptr d_params = 0;
    CU_CHECK(cuMemAlloc(&d_params, sizeof(Params)));
    CU_CHECK(cuMemcpyHtoD(d_params, &h_params, sizeof(Params)));

    // [K2-T118] === TODO [REQUIRED] step 8: build three-segment SBT ===
    // [K2-T119] raygen segment (1 record)
    RayGenRecord h_rg{};
    OPTIX_CHECK(optixSbtRecordPackHeader(pg_raygen, &h_rg));
    CUdeviceptr d_rg = 0;
    CU_CHECK(cuMemAlloc(&d_rg, sizeof(RayGenRecord)));
    CU_CHECK(cuMemcpyHtoD(d_rg, &h_rg, sizeof(RayGenRecord)));

    // [K2-T120] miss segment (1 record)
    MissRecord h_ms{};
    OPTIX_CHECK(optixSbtRecordPackHeader(pg_miss, &h_ms));
    h_ms.data.bg_color = {0.2f, 0.3f, 0.8f};  // [K2-T121] sky-blue background
    CUdeviceptr d_ms = 0;
    CU_CHECK(cuMemAlloc(&d_ms, sizeof(MissRecord)));
    CU_CHECK(cuMemcpyHtoD(d_ms, &h_ms, sizeof(MissRecord)));

    // [K2-T122] hitgroup segment (1 record - all Cornell Box faces share this closest-hit)
    HitGroupRecord h_hg{};
    OPTIX_CHECK(optixSbtRecordPackHeader(pg_hitgroup, &h_hg));
    h_hg.data.vertices = d_vertices;
    h_hg.data.indices  = d_indices;
    CUdeviceptr d_hg = 0;
    CU_CHECK(cuMemAlloc(&d_hg, sizeof(HitGroupRecord)));
    CU_CHECK(cuMemcpyHtoD(d_hg, &h_hg, sizeof(HitGroupRecord)));

    OptixShaderBindingTable real_sbt{};
    real_sbt.raygenRecord                = d_rg;
    real_sbt.missRecordBase              = d_ms;
    real_sbt.missRecordStrideInBytes     = sizeof(MissRecord);
    real_sbt.missRecordCount             = 1;
    real_sbt.hitgroupRecordBase          = d_hg;
    real_sbt.hitgroupRecordStrideInBytes = sizeof(HitGroupRecord);
    real_sbt.hitgroupRecordCount         = 1;
    // [K2-T123]
    std::puts("  SBT three segments built");

    // [K2-T124] === optixLaunch ===
    CUstream stream = nullptr;
    CUDA_CHECK(cudaStreamCreate(reinterpret_cast<cudaStream_t*>(&stream)));
    {
        NVTX_RANGE("K2::optixLaunch");
        OPTIX_CHECK(optixLaunch(
            pipeline, stream,
            d_params, sizeof(Params),
            &real_sbt,
            static_cast<unsigned int>(WIDTH),
            static_cast<unsigned int>(HEIGHT), 1u));
        CUDA_CHECK(cudaStreamSynchronize(reinterpret_cast<cudaStream_t>(stream)));
    }
    // [K2-T125]
    std::puts("  optixLaunch finished");

    // [K2-T126] === read back and write image ===
    std::vector<float4> h_output(static_cast<size_t>(WIDTH) * HEIGHT);
    CUDA_CHECK(cudaMemcpy(h_output.data(), d_output,
        h_output.size() * sizeof(float4), cudaMemcpyDeviceToHost));
    write_ppm("output_k2.ppm", h_output.data(), WIDTH, HEIGHT);

    // [K2-T127] TODO [ADVANCED] add a 2nd closest-hit (specular vs Lambertian)
    // [K2-T128] TODO [ADVANCED] multiple instances + different transforms (IAS)
    // [K2-T129] TODO [ADVANCED] recursive optixTrace in closest-hit for reflections

    // [K2-T130] === cleanup ===
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

    // [K2-T131]
    std::puts("[K2] done.");
    return 0;
}

#endif // GPU_STUDY_NO_OPTIX
