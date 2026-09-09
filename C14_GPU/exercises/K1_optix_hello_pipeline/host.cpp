// K1_optix_hello_pipeline/host.cpp
// ============================================================================
// [K1-T40] Exercise K1: hello_optix_pipeline
// [K1-T41]
// [K1-T42] Goal: minimal OptiX 8.x host-side boilerplate
// [K1-T43]   - initialize CUDA context + OptiX device context
// [K1-T44]   - load module from .optixir
// [K1-T45]   - register raygen program group
// [K1-T46]   - build pipeline
// [K1-T47]   - construct SBT (raygen segment)
// [K1-T48]   - optixLaunch produces a 1024x1024 pure-red image
// [K1-T49]   - write the result as output_k1.ppm
// [K1-T50]
// [K1-T51] Required steps map to module doc K1 items 1-10.
// ============================================================================

#ifdef GPU_STUDY_NO_OPTIX
// [K1-T52] === stub used when the SDK is missing ===
#include <cstdio>
int main() {
    // [K1-T53]
    std::puts("[K1] GPU_STUDY_NO_OPTIX=1: OptiX SDK is not installed.");
    // [K1-T54]
    std::puts("     See README.md section 'OptiX SDK installation' and re-configure CMake.");
    return 0;
}
#else
// [K1-T55] === real implementation ===

// [K1-T56] Driver API - must be included before cuda_check.cuh so CU_CHECK is available
#include <cuda.h>
#include <cuda_runtime.h>

// [K1-T57] OptiX 8.x headers (header-only + dynamic loading; no .lib link needed)
#include <optix.h>
#include <optix_function_table_definition.h>  // [K1-T58] must appear in exactly one binary
#include <optix_stubs.h>

// [K1-T59] Project-common helpers
#include "common/cuda_check.cuh"
#include "common/device_info.cuh"
#include "common/nvtx_range.cuh"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <vector>
#include <string>

// ============================================================================
// [K1-T60] OPTIX_CHECK - check OptixResult; print error and abort on failure
// ============================================================================
#define OPTIX_CHECK(expr)                                                       \
    do {                                                                        \
        OptixResult _r = (expr);                                                \
        if (_r != OPTIX_SUCCESS) {                                              \
            std::fprintf(stderr,                                                \
                "OptiX Error [%d]: %s\n"                                        \
                "  expr : %s\n"                                                 \
                "  file : %s:%d\n",                                             \
                static_cast<int>(_r),                                           \
                optixGetErrorString(_r), #expr, __FILE__, __LINE__);            \
            std::abort();                                                       \
        }                                                                       \
    } while (0)

// ============================================================================
// [K1-T61] OptiX log callback (writes to stderr; level 1=fatal 4=print)
// ============================================================================
static void optix_log_callback(unsigned int level,
                                const char* tag,
                                const char* message,
                                void* /*cbdata*/) {
    std::fprintf(stderr, "[OptiX][%u][%s] %s\n", level, tag, message);
}

// ============================================================================
// [K1-T62] Read a binary file (used to load .optixir)
// ============================================================================
static std::vector<char> read_binary_file(const char* path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) {
        // [K1-T63]
        std::fprintf(stderr, "Cannot open file: %s\n", path);
        std::abort();
    }
    const auto size = static_cast<std::size_t>(f.tellg());
    f.seekg(0);
    std::vector<char> buf(size);
    f.read(buf.data(), static_cast<std::streamsize>(size));
    return buf;
}

// ============================================================================
// [K1-T64] Params - shared constant memory layout with programs.cu
// ============================================================================
struct Params {
    float4*       output;   // [K1-T65] device-side output buffer (RGBA float4)
    unsigned int  width;
    unsigned int  height;
};

// ============================================================================
// [K1-T66] SBT record helper type
// [K1-T67] OptiX requires each SBT record = header (32 bytes) + user data,
// [K1-T68] total size must be 16-byte aligned.
// ============================================================================
template <typename T>
struct alignas(OPTIX_SBT_RECORD_ALIGNMENT) SbtRecord {
    char   header[OPTIX_SBT_RECORD_HEADER_SIZE];
    T      data;
};

struct RayGenData {
    // [K1-T69] K1 raygen has no per-record data; placeholder keeps alignment
    unsigned int pad;
};

using RayGenRecord = SbtRecord<RayGenData>;

// ============================================================================
// [K1-T70] Write float4 output as PPM P6 (8-bit RGB)
// ============================================================================
static void write_ppm(const char* path,
                      const float4* pixels,
                      int width, int height) {
    std::ofstream f(path, std::ios::binary);
    f << "P6\n" << width << " " << height << "\n255\n";
    for (int i = 0; i < width * height; ++i) {
        const float4& p = pixels[i];
        auto to_byte = [](float v) -> unsigned char {
            if (v < 0.f) v = 0.f;
            if (v > 1.f) v = 1.f;
            return static_cast<unsigned char>(v * 255.f + 0.5f);
        };
        unsigned char rgb[3] = {to_byte(p.x), to_byte(p.y), to_byte(p.z)};
        f.write(reinterpret_cast<const char*>(rgb), 3);
    }
    // [K1-T71]
    std::printf("  Output written: %s\n", path);
}

// ============================================================================
// [K1-T72] main
// ============================================================================
int main() {
    // [K1-T73]
    std::puts("[K1] hello_optix_pipeline");
    print_device_info();

    constexpr int WIDTH  = 1024;
    constexpr int HEIGHT = 1024;

    // [K1-T74] === TODO [REQUIRED] step 1: CUDA driver init ===
    // [K1-T75] Must establish a CUDA context before optixInit()
    // [K1-T76] (OptiX depends on a driver-API context).
    CU_CHECK(cuInit(0));
    CUdevice cu_device = 0;
    CU_CHECK(cuDeviceGet(&cu_device, 0));
    CUcontext cu_ctx = nullptr;
    CU_CHECK(cuCtxCreate(&cu_ctx, 0, cu_device));

    // [K1-T77] === TODO [REQUIRED] step 2: OptiX init ===
    // [K1-T78] optixInit() dynamically loads the OptiX function table
    // [K1-T79] (no static .lib link required).
    OPTIX_CHECK(optixInit());

    // [K1-T80] === TODO [REQUIRED] step 2b: create the OptiX device context ===
    OptixDeviceContext optix_ctx = nullptr;
    {
        OptixDeviceContextOptions opts{};
        opts.logCallbackFunction = optix_log_callback;
        opts.logCallbackLevel    = 4;  // [K1-T81] 4 = output every log level
        // [K1-T82] TODO [REQUIRED] optixDeviceContextCreate(cu_ctx, &opts, &optix_ctx)
        OPTIX_CHECK(optixDeviceContextCreate(cu_ctx, &opts, &optix_ctx));
    }
    // [K1-T83]
    std::puts("  OptiX device context created");

    // [K1-T84] === TODO [REQUIRED] step 3: load device program from .optixir ===
    // [K1-T85] K1_OPTIXIR_PATH is injected via target_compile_definitions in CMake.
    const char* optixir_path = K1_OPTIXIR_PATH;
    // [K1-T86]
    std::printf("  Loading .optixir: %s\n", optixir_path);
    // [K1-T87] TODO [REQUIRED] read optixir_path into std::vector<char>
    auto optixir_data = read_binary_file(optixir_path);

    // [K1-T88] === TODO [REQUIRED] step 3b: create OptiX module ===
    OptixModule optix_module = nullptr;
    {
        OptixModuleCompileOptions module_opts{};
        module_opts.maxRegisterCount = OPTIX_COMPILE_DEFAULT_MAX_REGISTER_COUNT;
        module_opts.optLevel         = OPTIX_COMPILE_OPTIMIZATION_DEFAULT;
        module_opts.debugLevel       = OPTIX_COMPILE_DEBUG_LEVEL_MINIMAL;

        OptixPipelineCompileOptions pipeline_opts{};
        pipeline_opts.usesMotionBlur                   = 0;
        pipeline_opts.traversableGraphFlags            = OPTIX_TRAVERSABLE_GRAPH_FLAG_ALLOW_SINGLE_GAS;
        pipeline_opts.numPayloadValues                 = 0;  // [K1-T89] K1 raygen does not use payload
        pipeline_opts.numAttributeValues               = 0;
        pipeline_opts.exceptionFlags                   = OPTIX_EXCEPTION_FLAG_NONE;
        pipeline_opts.pipelineLaunchParamsVariableName = "params";

        char   log_buf[4096];
        size_t log_size = sizeof(log_buf);

        // [K1-T90] TODO [REQUIRED] optixModuleCreate(...)
        OPTIX_CHECK(optixModuleCreate(
            optix_ctx,
            &module_opts,
            &pipeline_opts,
            optixir_data.data(),
            optixir_data.size(),
            log_buf, &log_size,
            &optix_module));
        if (log_size > 1) std::fprintf(stderr, "  [module log] %s\n", log_buf);
        // [K1-T91]
        std::puts("  OptiX module created");

        // [K1-T92] === TODO [REQUIRED] step 4: create raygen program group ===
        OptixProgramGroup raygen_pg = nullptr;
        {
            OptixProgramGroupDesc pg_desc{};
            pg_desc.kind                     = OPTIX_PROGRAM_GROUP_KIND_RAYGEN;
            pg_desc.raygen.module            = optix_module;
            pg_desc.raygen.entryFunctionName = "__raygen__hello";

            OptixProgramGroupOptions pg_opts{};
            log_size = sizeof(log_buf);
            // [K1-T93] TODO [REQUIRED] optixProgramGroupCreate(...)
            OPTIX_CHECK(optixProgramGroupCreate(
                optix_ctx, &pg_desc, 1, &pg_opts,
                log_buf, &log_size, &raygen_pg));
            if (log_size > 1) std::fprintf(stderr, "  [pg log] %s\n", log_buf);
            // [K1-T94]
            std::puts("  raygen program group created");

            // [K1-T95] === TODO [REQUIRED] step 5: build pipeline ===
            OptixPipeline pipeline = nullptr;
            {
                OptixPipelineLinkOptions link_opts{};
                link_opts.maxTraceDepth = 1;

                log_size = sizeof(log_buf);
                // [K1-T96] TODO [REQUIRED] optixPipelineCreate(...)
                OPTIX_CHECK(optixPipelineCreate(
                    optix_ctx,
                    &pipeline_opts,
                    &link_opts,
                    &raygen_pg, 1,
                    log_buf, &log_size,
                    &pipeline));
                if (log_size > 1) std::fprintf(stderr, "  [pipeline log] %s\n", log_buf);
                // [K1-T97]
                std::puts("  pipeline created");

                // [K1-T98] === TODO [REQUIRED] step 6: allocate output buffer ===
                float4* d_output = nullptr;
                CUDA_CHECK(cudaMalloc(&d_output,
                    static_cast<size_t>(WIDTH) * HEIGHT * sizeof(float4)));

                // [K1-T99] === TODO [REQUIRED] step 7: upload Params to device ===
                Params h_params{};
                h_params.output = d_output;
                h_params.width  = static_cast<unsigned int>(WIDTH);
                h_params.height = static_cast<unsigned int>(HEIGHT);

                CUdeviceptr d_params = 0;
                CU_CHECK(cuMemAlloc(&d_params, sizeof(Params)));
                CU_CHECK(cuMemcpyHtoD(d_params, &h_params, sizeof(Params)));

                // [K1-T100] === TODO [REQUIRED] step 8: build SBT (raygen segment) ===
                // [K1-T101] SBT record = header (32-byte program-group handle) + user data
                RayGenRecord h_rg_record{};
                // [K1-T102] TODO [REQUIRED] optixSbtRecordPackHeader(raygen_pg, &h_rg_record)
                OPTIX_CHECK(optixSbtRecordPackHeader(raygen_pg, &h_rg_record));
                h_rg_record.data.pad = 0u;

                CUdeviceptr d_rg_record = 0;
                CU_CHECK(cuMemAlloc(&d_rg_record, sizeof(RayGenRecord)));
                CU_CHECK(cuMemcpyHtoD(d_rg_record, &h_rg_record, sizeof(RayGenRecord)));

                OptixShaderBindingTable sbt{};
                sbt.raygenRecord = d_rg_record;
                // [K1-T103] miss / hitgroup left empty in K1 (no optixTrace call)
                std::puts("  SBT constructed");

                // [K1-T104] === TODO [REQUIRED] step 9: launch ray tracing ===
                CUstream stream = nullptr;
                CUDA_CHECK(cudaStreamCreate(reinterpret_cast<cudaStream_t*>(&stream)));

                {
                    NVTX_RANGE("K1::optixLaunch");
                    // [K1-T105] TODO [REQUIRED] optixLaunch(...)
                    OPTIX_CHECK(optixLaunch(
                        pipeline,
                        stream,
                        d_params, sizeof(Params),
                        &sbt,
                        static_cast<unsigned int>(WIDTH),
                        static_cast<unsigned int>(HEIGHT),
                        1u));
                    // [K1-T106] TODO [REQUIRED] wait for the GPU to finish (stream sync)
                    CUDA_CHECK(cudaStreamSynchronize(reinterpret_cast<cudaStream_t>(stream)));
                }
                // [K1-T107]
                std::puts("  optixLaunch finished");

                // [K1-T108] === TODO [REQUIRED] step 10: read back result and write image ===
                std::vector<float4> h_output(static_cast<size_t>(WIDTH) * HEIGHT);
                // [K1-T109] TODO [REQUIRED] cudaMemcpy(...) device->host
                CUDA_CHECK(cudaMemcpy(
                    h_output.data(), d_output,
                    h_output.size() * sizeof(float4),
                    cudaMemcpyDeviceToHost));

                write_ppm("output_k1.ppm",
                          h_output.data(), WIDTH, HEIGHT);

                // [K1-T110] === TODO [ADVANCED] gradient color from threadIdx/blockDim ===
                // [K1-T111] === TODO [ADVANCED] call optixTrace from raygen and talk to miss ===
                // [K1-T112] === TODO [ADVANCED] add closest-hit returning a different color ===

                // [K1-T113] === cleanup ===
                CUDA_CHECK(cudaStreamDestroy(reinterpret_cast<cudaStream_t>(stream)));
                CU_CHECK(cuMemFree(d_params));
                CU_CHECK(cuMemFree(d_rg_record));
                CUDA_CHECK(cudaFree(d_output));
                OPTIX_CHECK(optixPipelineDestroy(pipeline));
            }
            OPTIX_CHECK(optixProgramGroupDestroy(raygen_pg));
        }
        OPTIX_CHECK(optixModuleDestroy(optix_module));
    }

    OPTIX_CHECK(optixDeviceContextDestroy(optix_ctx));
    CU_CHECK(cuCtxDestroy(cu_ctx));

    // [K1-T114]
    std::puts("[K1] done.");
    return 0;
}

#endif // GPU_STUDY_NO_OPTIX
