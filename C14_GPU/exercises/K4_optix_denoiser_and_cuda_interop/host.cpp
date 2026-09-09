// K4_optix_denoiser_and_cuda_interop/host.cpp
// ============================================================================
// [K4-T50] Exercise K4: OptiX Denoiser + CUDA Interop
// [K4-T51]
// [K4-T52] Goals:
// [K4-T53]   - generate a high-noise image with 32 spp Monte Carlo path tracing
// [K4-T54]   - smooth the noise with the OptiX Denoiser (AI denoising)
// [K4-T55]   - convert HDR float4 to LDR uint8 with a CUDA kernel
// [K4-T56]     (kernel_tonemap_aces)
// [K4-T57]   - emit two PPMs: output_k4_noisy.ppm (raw)
// [K4-T58]                  + output_k4_denoised.ppm (denoised)
// [K4-T59]
// [K4-T60] New compared to K3:
// [K4-T61]   - optixDenoiserCreate / optixDenoiserSetup / optixDenoiserInvoke
// [K4-T62]   - albedo / normal guide buffers (optional, improves denoise quality)
// [K4-T63]   - CUDA tonemap kernel (declaring __global__ in host.cpp would
// [K4-T64]     require splitting it into a .cu file, or invoking via runtime API;
// [K4-T65]     this exercise defines tonemap at the bottom of this file)
// [K4-T66]
// [K4-T67] Note: host.cpp is a pure C++ TU (.cpp); it does not contain a __global__
// [K4-T68]       kernel. A real CUDA tonemap kernel must be compiled with nvcc, so
// [K4-T69]       it would either live in a separate .cu file (invoked through
// [K4-T70]       cudaLaunchKernel), or be moved here at compile-time integration.
// [K4-T71]       The skeleton uses a CPU-side tonemap (no extra .cu file needed).
// [K4-T72]       TODO [ADVANCED] move tonemap into its own .cu file and accelerate
// [K4-T73]                       it with a CUDA kernel.
// ============================================================================

#ifdef GPU_STUDY_NO_OPTIX
#include <cstdio>
int main() {
    // [K4-T74]
    std::puts("[K4] GPU_STUDY_NO_OPTIX=1: OptiX SDK is not installed.");
    // [K4-T75]
    std::puts("     See K1/README.md and re-configure CMake after installation.");
    return 0;
}
#else

#include <cuda.h>
#include <cuda_runtime.h>
#include <optix.h>
#include <optix_function_table_definition.h>
#include <optix_stubs.h>
#include <optix_denoiser_tiling.h>  // [K4-T76] OptiX 8 denoiser helper

#include "common/cuda_check.cuh"
#include "common/device_info.cuh"
#include "common/nvtx_range.cuh"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>

// ============================================================================
// [K4-T77] OPTIX_CHECK
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
        // [K4-T78]
        std::fprintf(stderr, "Cannot open: %s\n", path); std::abort();
    }
    auto sz = static_cast<std::size_t>(f.tellg());
    f.seekg(0);
    std::vector<char> buf(sz);
    f.read(buf.data(), static_cast<std::streamsize>(sz));
    return buf;
}

// ============================================================================
// [K4-T79] Shared structs (match programs.cu)
// ============================================================================
struct PointLight {
    float3 pos;
    float3 intensity;
};

struct Params {
    float4*                color_buffer;   // [K4-T80] main color output (float4 HDR)
    float4*                albedo_buffer;  // [K4-T81] albedo guide (optional)
    float4*                normal_buffer;  // [K4-T82] normal guide (optional)
    unsigned int           width;
    unsigned int           height;
    OptixTraversableHandle handle;
    PointLight             light;
    unsigned int           samples_per_pixel;  // [K4-T83] MC sample count
    unsigned int           seed;               // [K4-T84] base random seed
};

// ============================================================================
// [K4-T85] SBT record templates
// ============================================================================
template <typename T>
struct alignas(OPTIX_SBT_RECORD_ALIGNMENT) SbtRecord {
    char header[OPTIX_SBT_RECORD_HEADER_SIZE];
    T    data;
};

struct RayGenData   { unsigned int pad; };
struct MissData     { float3 bg_color; };
struct HitGroupData {
    float3* vertices;
    uint3*  indices;
    float3  base_color;
};

using RayGenRecord   = SbtRecord<RayGenData>;
using MissRecord     = SbtRecord<MissData>;
using HitGroupRecord = SbtRecord<HitGroupData>;

// ============================================================================
// [K4-T86] Cornell Box geometry (reused from K2/K3)
// ============================================================================
static const float3 cornell_vertices[] = {
    {-1.f, -1.f, -1.f}, { 1.f, -1.f, -1.f},
    { 1.f,  1.f, -1.f}, {-1.f,  1.f, -1.f},
    {-1.f, -1.f,  1.f}, { 1.f, -1.f,  1.f},
    { 1.f,  1.f,  1.f}, {-1.f,  1.f,  1.f},
};
static const uint3 cornell_indices[] = {
    {0,1,2},{0,2,3}, {0,1,5},{0,5,4}, {3,7,6},{3,6,2},
    {0,4,7},{0,7,3}, {1,2,6},{1,6,5},
};
static constexpr unsigned int NUM_TRIANGLES =
    static_cast<unsigned int>(sizeof(cornell_indices)/sizeof(cornell_indices[0]));
static const float3 face_colors[] = {
    {.8f,.8f,.8f},{.8f,.8f,.8f},{.8f,.8f,.8f},{.8f,.8f,.8f},
    {.8f,.8f,.8f},{.8f,.8f,.8f},{.8f,.1f,.1f},{.8f,.1f,.1f},
    {.1f,.1f,.8f},{.1f,.1f,.8f},
};

// ============================================================================
// [K4-T87] CPU-side ACES tonemap (CPU implementation; advanced moves to a CUDA kernel)
// ============================================================================
static inline float aces_channel(float x) {
    constexpr float a = 2.51f, b = 0.03f, c = 2.43f, d = 0.59f, e = 0.14f;
    float v = (x * (a * x + b)) / (x * (c * x + d) + e);
    return v < 0.f ? 0.f : (v > 1.f ? 1.f : v);
}
static inline unsigned char to_srgb(float linear) {
    float g = std::pow(linear, 1.f / 2.2f);
    return static_cast<unsigned char>(g * 255.f + .5f);
}

// ============================================================================
// [K4-T88] write PPM (float4 HDR)
// ============================================================================
static void write_ppm_hdr(const char* path, const float4* px, int w, int h) {
    std::ofstream f(path, std::ios::binary);
    f << "P6\n" << w << " " << h << "\n255\n";
    for (int i = 0; i < w*h; ++i) {
        unsigned char rgb[3] = {
            to_srgb(aces_channel(px[i].x)),
            to_srgb(aces_channel(px[i].y)),
            to_srgb(aces_channel(px[i].z))
        };
        f.write(reinterpret_cast<const char*>(rgb), 3);
    }
    // [K4-T89]
    std::printf("  Output written: %s\n", path);
}

// ============================================================================
// [K4-T90] main
// ============================================================================
int main() {
    // [K4-T91]
    std::puts("[K4] optix_denoiser_and_cuda_interop");
    print_device_info();

    constexpr int WIDTH   = 1024;
    constexpr int HEIGHT  = 1024;
    constexpr int SPP     = 32;   // [K4-T92] MC sample count (intentionally low to show noise)

    // [K4-T93] === CUDA context ===
    CU_CHECK(cuInit(0));
    CUdevice cu_device = 0;
    CU_CHECK(cuDeviceGet(&cu_device, 0));
    CUcontext cu_ctx = nullptr;
    CU_CHECK(cuCtxCreate(&cu_ctx, 0, cu_device));

    // [K4-T94] === OptiX init ===
    OPTIX_CHECK(optixInit());
    OptixDeviceContext optix_ctx = nullptr;
    {
        OptixDeviceContextOptions opts{};
        opts.logCallbackFunction = optix_log_callback;
        opts.logCallbackLevel    = 4;
        OPTIX_CHECK(optixDeviceContextCreate(cu_ctx, &opts, &optix_ctx));
    }

    // [K4-T95] === upload geometry ===
    float3* d_vertices = nullptr;
    uint3*  d_indices  = nullptr;
    CUDA_CHECK(cudaMalloc(&d_vertices, sizeof(cornell_vertices)));
    CUDA_CHECK(cudaMemcpy(d_vertices, cornell_vertices,
        sizeof(cornell_vertices), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMalloc(&d_indices, sizeof(cornell_indices)));
    CUDA_CHECK(cudaMemcpy(d_indices, cornell_indices,
        sizeof(cornell_indices), cudaMemcpyHostToDevice));

    // [K4-T96] === GAS build ===
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
            static_cast<unsigned int>(sizeof(cornell_vertices)/sizeof(float3));
        tri_input.triangleArray.vertexFormat        = OPTIX_VERTEX_FORMAT_FLOAT3;
        tri_input.triangleArray.vertexStrideInBytes = sizeof(float3);
        tri_input.triangleArray.indexBuffer         = reinterpret_cast<CUdeviceptr>(d_indices);
        tri_input.triangleArray.numIndexTriplets    = NUM_TRIANGLES;
        tri_input.triangleArray.indexFormat         = OPTIX_INDICES_FORMAT_UNSIGNED_INT3;
        tri_input.triangleArray.indexStrideInBytes  = sizeof(uint3);
        static const unsigned int sbt_flags[10] = {
            OPTIX_GEOMETRY_FLAG_NONE,OPTIX_GEOMETRY_FLAG_NONE,
            OPTIX_GEOMETRY_FLAG_NONE,OPTIX_GEOMETRY_FLAG_NONE,
            OPTIX_GEOMETRY_FLAG_NONE,OPTIX_GEOMETRY_FLAG_NONE,
            OPTIX_GEOMETRY_FLAG_NONE,OPTIX_GEOMETRY_FLAG_NONE,
            OPTIX_GEOMETRY_FLAG_NONE,OPTIX_GEOMETRY_FLAG_NONE,
        };
        tri_input.triangleArray.flags         = sbt_flags;
        tri_input.triangleArray.numSbtRecords = NUM_TRIANGLES;

        OptixAccelBufferSizes buf_sizes{};
        OPTIX_CHECK(optixAccelComputeMemoryUsage(
            optix_ctx, &accel_opts, &tri_input, 1, &buf_sizes));
        CUdeviceptr d_temp = 0;
        CU_CHECK(cuMemAlloc(&d_temp, buf_sizes.tempSizeInBytes));
        CU_CHECK(cuMemAlloc(&d_gas_output, buf_sizes.outputSizeInBytes));
        OPTIX_CHECK(optixAccelBuild(
            optix_ctx, nullptr, &accel_opts, &tri_input, 1,
            d_temp, buf_sizes.tempSizeInBytes,
            d_gas_output, buf_sizes.outputSizeInBytes,
            &gas_handle, nullptr, 0));
        CUDA_CHECK(cudaDeviceSynchronize());
        CU_CHECK(cuMemFree(d_temp));
        // [K4-T97]
        std::puts("  GAS build complete");
    }

    // [K4-T98] === load .optixir, create module ===
    auto optixir_data = read_binary_file(K4_OPTIXIR_PATH);

    OptixPipelineCompileOptions pipeline_opts{};
    pipeline_opts.usesMotionBlur                   = 0;
    pipeline_opts.traversableGraphFlags            = OPTIX_TRAVERSABLE_GRAPH_FLAG_ALLOW_SINGLE_GAS;
    pipeline_opts.numPayloadValues                 = 4;
    pipeline_opts.numAttributeValues               = 2;
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

    // [K4-T99] === Program groups ===
    OptixProgramGroup pg_raygen   = nullptr;
    OptixProgramGroup pg_miss     = nullptr;
    OptixProgramGroup pg_hitgroup = nullptr;

    {
        OptixProgramGroupDesc d{};
        d.kind = OPTIX_PROGRAM_GROUP_KIND_RAYGEN;
        d.raygen.module = optix_module;
        d.raygen.entryFunctionName = "__raygen__mc";
        OptixProgramGroupOptions o{}; log_size = sizeof(log_buf);
        OPTIX_CHECK(optixProgramGroupCreate(optix_ctx, &d, 1, &o,
            log_buf, &log_size, &pg_raygen));
    }
    {
        OptixProgramGroupDesc d{};
        d.kind = OPTIX_PROGRAM_GROUP_KIND_MISS;
        d.miss.module = optix_module;
        d.miss.entryFunctionName = "__miss__background";
        OptixProgramGroupOptions o{}; log_size = sizeof(log_buf);
        OPTIX_CHECK(optixProgramGroupCreate(optix_ctx, &d, 1, &o,
            log_buf, &log_size, &pg_miss));
    }
    {
        OptixProgramGroupDesc d{};
        d.kind = OPTIX_PROGRAM_GROUP_KIND_HITGROUP;
        d.hitgroup.moduleCH            = optix_module;
        d.hitgroup.entryFunctionNameCH = "__closesthit__radiance";
        d.hitgroup.moduleAH            = nullptr;
        d.hitgroup.entryFunctionNameAH = nullptr;
        d.hitgroup.moduleIS            = nullptr;
        d.hitgroup.entryFunctionNameIS = nullptr;
        OptixProgramGroupOptions o{}; log_size = sizeof(log_buf);
        OPTIX_CHECK(optixProgramGroupCreate(optix_ctx, &d, 1, &o,
            log_buf, &log_size, &pg_hitgroup));
    }

    // [K4-T100] === pipeline ===
    OptixPipeline pipeline = nullptr;
    {
        OptixPipelineLinkOptions link_opts{};
        link_opts.maxTraceDepth = 2;
        OptixProgramGroup pgs[3] = {pg_raygen, pg_miss, pg_hitgroup};
        log_size = sizeof(log_buf);
        OPTIX_CHECK(optixPipelineCreate(
            optix_ctx, &pipeline_opts, &link_opts,
            pgs, 3, log_buf, &log_size, &pipeline));
        // [K4-T101]
        std::puts("  pipeline created");
    }

    // [K4-T102] === TODO [REQUIRED] step 5: allocate color / albedo / normal buffers ===
    const size_t buf_bytes = static_cast<size_t>(WIDTH) * HEIGHT * sizeof(float4);
    float4* d_color  = nullptr;
    float4* d_albedo = nullptr;
    float4* d_normal = nullptr;
    CUDA_CHECK(cudaMalloc(&d_color,  buf_bytes));
    CUDA_CHECK(cudaMalloc(&d_albedo, buf_bytes));
    CUDA_CHECK(cudaMalloc(&d_normal, buf_bytes));
    // [K4-T103] Init albedo/normal (all white / normal up by default)
    CUDA_CHECK(cudaMemset(d_albedo, 0, buf_bytes));  // [K4-T104] filled later by closest-hit
    CUDA_CHECK(cudaMemset(d_normal, 0, buf_bytes));

    // [K4-T105] === Params ===
    Params h_params{};
    h_params.color_buffer        = d_color;
    h_params.albedo_buffer       = d_albedo;
    h_params.normal_buffer       = d_normal;
    h_params.width               = static_cast<unsigned int>(WIDTH);
    h_params.height              = static_cast<unsigned int>(HEIGHT);
    h_params.handle              = gas_handle;
    h_params.light.pos           = {0.f, 0.8f, 0.f};
    h_params.light.intensity     = {2.f, 2.f, 2.f};
    h_params.samples_per_pixel   = static_cast<unsigned int>(SPP);
    h_params.seed                = 42u;

    CUdeviceptr d_params = 0;
    CU_CHECK(cuMemAlloc(&d_params, sizeof(Params)));
    CU_CHECK(cuMemcpyHtoD(d_params, &h_params, sizeof(Params)));

    // [K4-T106] === SBT ===
    RayGenRecord h_rg{};
    OPTIX_CHECK(optixSbtRecordPackHeader(pg_raygen, &h_rg));
    CUdeviceptr d_rg = 0;
    CU_CHECK(cuMemAlloc(&d_rg, sizeof(RayGenRecord)));
    CU_CHECK(cuMemcpyHtoD(d_rg, &h_rg, sizeof(RayGenRecord)));

    MissRecord h_ms{};
    OPTIX_CHECK(optixSbtRecordPackHeader(pg_miss, &h_ms));
    h_ms.data.bg_color = {0.2f, 0.3f, 0.8f};
    CUdeviceptr d_ms = 0;
    CU_CHECK(cuMemAlloc(&d_ms, sizeof(MissRecord)));
    CU_CHECK(cuMemcpyHtoD(d_ms, &h_ms, sizeof(MissRecord)));

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

    // [K4-T107] === TODO [REQUIRED] step 1: optixLaunch (32 spp path tracing) ===
    CUstream stream = nullptr;
    CUDA_CHECK(cudaStreamCreate(reinterpret_cast<cudaStream_t*>(&stream)));
    {
        NVTX_RANGE("K4::optixLaunch_noisy");
        OPTIX_CHECK(optixLaunch(
            pipeline, stream,
            d_params, sizeof(Params),
            &sbt,
            static_cast<unsigned int>(WIDTH),
            static_cast<unsigned int>(HEIGHT), 1u));
        CUDA_CHECK(cudaStreamSynchronize(reinterpret_cast<cudaStream_t>(stream)));
    }
    // [K4-T108]
    std::puts("  Path tracing complete (32 spp, high noise)");

    // [K4-T109] === read back and save the noisy image ===
    std::vector<float4> h_noisy(static_cast<size_t>(WIDTH) * HEIGHT);
    CUDA_CHECK(cudaMemcpy(h_noisy.data(), d_color,
        h_noisy.size() * sizeof(float4), cudaMemcpyDeviceToHost));
    write_ppm_hdr("output_k4_noisy.ppm", h_noisy.data(), WIDTH, HEIGHT);

    // [K4-T110] === TODO [REQUIRED] steps 2-7: OptiX Denoiser ===
    OptixDenoiser denoiser = nullptr;
    {
        // [K4-T111] TODO [REQUIRED] step 2: optixDenoiserCreate
        OptixDenoiserOptions dn_opts{};
        dn_opts.guideAlbedo = 1;  // [K4-T112] use albedo guide
        dn_opts.guideNormal = 1;  // [K4-T113] use normal guide
        OPTIX_CHECK(optixDenoiserCreate(
            optix_ctx,
            OPTIX_DENOISER_MODEL_KIND_HDR,
            &dn_opts,
            &denoiser));

        // [K4-T114] TODO [REQUIRED] step 6: query and allocate state / scratch buffers
        OptixDenoiserSizes dn_sizes{};
        OPTIX_CHECK(optixDenoiserComputeMemoryResources(
            denoiser,
            static_cast<unsigned int>(WIDTH),
            static_cast<unsigned int>(HEIGHT),
            &dn_sizes));

        CUdeviceptr d_dn_state   = 0;
        CUdeviceptr d_dn_scratch = 0;
        CU_CHECK(cuMemAlloc(&d_dn_state,   dn_sizes.stateSizeInBytes));
        CU_CHECK(cuMemAlloc(&d_dn_scratch, dn_sizes.withOverlapScratchSizeInBytes));

        OPTIX_CHECK(optixDenoiserSetup(
            denoiser, stream,
            static_cast<unsigned int>(WIDTH),
            static_cast<unsigned int>(HEIGHT),
            d_dn_state,   dn_sizes.stateSizeInBytes,
            d_dn_scratch, dn_sizes.withOverlapScratchSizeInBytes));
        CUDA_CHECK(cudaStreamSynchronize(reinterpret_cast<cudaStream_t>(stream)));
        // [K4-T115]
        std::puts("  Denoiser setup complete");

        // [K4-T116] TODO [REQUIRED] step 3: set denoiser parameters
        OptixDenoiserParams dn_params{};
        dn_params.blendFactor  = 0.f;  // [K4-T117] 0 = trust denoiser output completely
        dn_params.hdrIntensity = 0;    // [K4-T118] do not use intensity pre-computation

        // [K4-T119] Allocate denoised output buffer
        float4* d_denoised = nullptr;
        CUDA_CHECK(cudaMalloc(&d_denoised, buf_bytes));

        // [K4-T120] TODO [REQUIRED] step 4: set up albedo / normal guide buffers (optional)
        OptixDenoiserGuideLayer guide_layer{};
        guide_layer.albedo.data               = reinterpret_cast<CUdeviceptr>(d_albedo);
        guide_layer.albedo.width              = static_cast<unsigned int>(WIDTH);
        guide_layer.albedo.height             = static_cast<unsigned int>(HEIGHT);
        guide_layer.albedo.rowStrideInBytes   = WIDTH * sizeof(float4);
        guide_layer.albedo.pixelStrideInBytes = sizeof(float4);
        guide_layer.albedo.format             = OPTIX_PIXEL_FORMAT_FLOAT4;

        guide_layer.normal.data               = reinterpret_cast<CUdeviceptr>(d_normal);
        guide_layer.normal.width              = static_cast<unsigned int>(WIDTH);
        guide_layer.normal.height             = static_cast<unsigned int>(HEIGHT);
        guide_layer.normal.rowStrideInBytes   = WIDTH * sizeof(float4);
        guide_layer.normal.pixelStrideInBytes = sizeof(float4);
        guide_layer.normal.format             = OPTIX_PIXEL_FORMAT_FLOAT4;

        // [K4-T121] input layer (color)
        OptixDenoiserLayer dn_layer{};
        dn_layer.input.data               = reinterpret_cast<CUdeviceptr>(d_color);
        dn_layer.input.width              = static_cast<unsigned int>(WIDTH);
        dn_layer.input.height             = static_cast<unsigned int>(HEIGHT);
        dn_layer.input.rowStrideInBytes   = WIDTH * sizeof(float4);
        dn_layer.input.pixelStrideInBytes = sizeof(float4);
        dn_layer.input.format             = OPTIX_PIXEL_FORMAT_FLOAT4;

        dn_layer.output.data               = reinterpret_cast<CUdeviceptr>(d_denoised);
        dn_layer.output.width              = static_cast<unsigned int>(WIDTH);
        dn_layer.output.height             = static_cast<unsigned int>(HEIGHT);
        dn_layer.output.rowStrideInBytes   = WIDTH * sizeof(float4);
        dn_layer.output.pixelStrideInBytes = sizeof(float4);
        dn_layer.output.format             = OPTIX_PIXEL_FORMAT_FLOAT4;

        // [K4-T122] TODO [REQUIRED] step 7: optixDenoiserInvoke
        {
            NVTX_RANGE("K4::optixDenoiserInvoke");
            OPTIX_CHECK(optixDenoiserInvoke(
                denoiser, stream,
                &dn_params,
                d_dn_state, dn_sizes.stateSizeInBytes,
                &guide_layer,
                &dn_layer, 1,
                0, 0,  // [K4-T123] input offset x/y
                d_dn_scratch, dn_sizes.withOverlapScratchSizeInBytes));
            CUDA_CHECK(cudaStreamSynchronize(reinterpret_cast<cudaStream_t>(stream)));
        }
        // [K4-T124]
        std::puts("  Denoiser invoke complete");

        // [K4-T125] === TODO [REQUIRED] steps 8-9: read back denoised result and write image ===
        std::vector<float4> h_denoised(static_cast<size_t>(WIDTH) * HEIGHT);
        CUDA_CHECK(cudaMemcpy(h_denoised.data(), d_denoised,
            h_denoised.size() * sizeof(float4), cudaMemcpyDeviceToHost));
        // [K4-T126] TODO [REQUIRED] step 8: tonemap + write output_k4_denoised.ppm
        write_ppm_hdr("output_k4_denoised.ppm", h_denoised.data(), WIDTH, HEIGHT);

        // [K4-T127] TODO [ADVANCED] move tonemap into a standalone CUDA kernel (kernel_tonemap_aces<<<...>>>)
        // [K4-T128] TODO [ADVANCED] compare blendFactor = 0.0 / 0.5 / 1.0 denoise output
        // [K4-T129] TODO [ADVANCED] albedo-guided + normal-guided denoising A/B comparison

        CUDA_CHECK(cudaFree(d_denoised));
        CU_CHECK(cuMemFree(d_dn_state));
        CU_CHECK(cuMemFree(d_dn_scratch));
        OPTIX_CHECK(optixDenoiserDestroy(denoiser));
    }

    // [K4-T130] === cleanup ===
    CUDA_CHECK(cudaStreamDestroy(reinterpret_cast<cudaStream_t>(stream)));
    CU_CHECK(cuMemFree(d_params));
    CU_CHECK(cuMemFree(d_rg));
    CU_CHECK(cuMemFree(d_ms));
    CU_CHECK(cuMemFree(d_hg));
    CUDA_CHECK(cudaFree(d_color));
    CUDA_CHECK(cudaFree(d_albedo));
    CUDA_CHECK(cudaFree(d_normal));
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

    // [K4-T131]
    std::puts("[K4] done.");
    return 0;
}

#endif // GPU_STUDY_NO_OPTIX
