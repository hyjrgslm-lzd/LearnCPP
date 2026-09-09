// [E2-T01] E2_async_mem_pool/main.cu
// [E2-T02] Goal: understand cudaMallocAsync / cudaFreeAsync, the stream-ordered
//          memory allocator, and compare cudaMalloc vs cudaMallocAsync latency.
//          Custom memory pool via cudaMemPoolCreate.
//
// [E2-T03] Required tasks (Module E exercise E2):
//   [REQUIRED-1] loop 100x alloc/free 64 MB, compare cudaMalloc vs cudaMallocAsync latency
//   [REQUIRED-2] cudaMemPoolCreate to make a custom pool, allocate from it, compare latency
//   [REQUIRED-3] cudaMallocAsync -> kernel launch -> cudaFreeAsync (no intermediate sync)
//   [REQUIRED-4] loop 10x "alloc -> kernel -> free", compare total time of both styles
//   [REQUIRED-5] Nsight Compute samples to observe pool reuse in Memory Workload Analysis
//
// [E2-T04] Advanced tasks (TODO [ADVANCED]):
//   [ADVANCED-A] multiple memory pools, different sizes, observe fragmentation
//   [ADVANCED-B] cudaMemPoolSetAttribute to set release threshold
//   [ADVANCED-C] compare cudaMallocAsync vs cudaMallocManaged (page fault behavior)

#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <cuda_runtime.h>

#include "common/cuda_check.cuh"
#include "common/timer.cuh"
#include "common/device_info.cuh"
#include "common/nvtx_range.cuh"

// ---------------------------------------------------------------------------
// [E2-T05] Constants
// ---------------------------------------------------------------------------
static constexpr size_t ALLOC_SIZE   = 64ULL * 1024 * 1024; // 64 MB
static constexpr int    ALLOC_ITERS  = 100;
static constexpr int    LOOP_ITERS   = 10;
static constexpr int    BLOCK        = 256;
static constexpr int    N_ELEM       = static_cast<int>(ALLOC_SIZE / sizeof(float));

// ---------------------------------------------------------------------------
// [E2-T06] Kernel (TODO area)
// ---------------------------------------------------------------------------

// [E2-T07] kernel_fill: simple per-element write so the allocation is actually used.
// [E2-T08] TODO [REQUIRED-3] implement: out[idx] = val (each thread writes one element).
__global__ void kernel_fill(float* out, float val, int n)
{
    // [E2-T09] TODO [REQUIRED-3] implement kernel_fill
    (void)out; (void)val; (void)n;
}

// ---------------------------------------------------------------------------
// [E2-T10] Test A: cudaMalloc / cudaFree loop (baseline)
// ---------------------------------------------------------------------------
static void benchmark_cudaMalloc(cudaStream_t stream)
{
    NVTX_RANGE("E2/cudaMalloc_benchmark");
    // [E2-T11]
    std::fprintf(stdout, "--- cudaMalloc benchmark (%d iters x %.0f MB) ---\n",
                 ALLOC_ITERS, ALLOC_SIZE / 1048576.0);

    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < ALLOC_ITERS; ++i) {
        void* ptr = nullptr;
        // [E2-T12] TODO [REQUIRED-1] cudaMalloc(&ptr, ALLOC_SIZE)
        // [E2-T13] TODO [REQUIRED-1] cudaFree(ptr)
        (void)ptr;
    }
    CUDA_CHECK(cudaStreamSynchronize(stream));
    auto t1 = std::chrono::steady_clock::now();

    double total_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    // [E2-T14]
    std::fprintf(stdout, "  total = %.3f ms, avg = %.3f ms/alloc\n\n",
                 total_ms, total_ms / ALLOC_ITERS);
}

// ---------------------------------------------------------------------------
// [E2-T15] Test B: cudaMallocAsync / cudaFreeAsync (default pool)
// ---------------------------------------------------------------------------
static void benchmark_cudaMallocAsync_default(cudaStream_t stream)
{
    NVTX_RANGE("E2/cudaMallocAsync_default_benchmark");
    // [E2-T16]
    std::fprintf(stdout, "--- cudaMallocAsync (default pool) benchmark (%d iters x %.0f MB) ---\n",
                 ALLOC_ITERS, ALLOC_SIZE / 1048576.0);

    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < ALLOC_ITERS; ++i) {
        void* ptr = nullptr;
        // [E2-T17] TODO [REQUIRED-1] cudaMallocAsync(&ptr, ALLOC_SIZE, stream)
        // [E2-T18] TODO [REQUIRED-1] cudaFreeAsync(ptr, stream)
        (void)ptr;
    }
    CUDA_CHECK(cudaStreamSynchronize(stream));
    auto t1 = std::chrono::steady_clock::now();

    double total_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    // [E2-T19]
    std::fprintf(stdout, "  total = %.3f ms, avg = %.3f ms/alloc\n\n",
                 total_ms, total_ms / ALLOC_ITERS);
}

// ---------------------------------------------------------------------------
// [E2-T20] Test C: cudaMemPoolCreate custom pool
// ---------------------------------------------------------------------------
static void benchmark_custom_pool(cudaStream_t stream)
{
    NVTX_RANGE("E2/custom_pool_benchmark");
    // [E2-T21]
    std::fprintf(stdout, "--- cudaMallocAsync (custom pool) benchmark (%d iters x %.0f MB) ---\n",
                 ALLOC_ITERS, ALLOC_SIZE / 1048576.0);

    // [E2-T22] TODO [REQUIRED-2] create custom memory pool
    cudaMemPool_t pool = nullptr;
    // cudaMemPoolProps props{};
    // props.allocType   = cudaMemAllocationTypePinned;
    // props.handleTypes = cudaMemHandleTypeNone;
    // props.location.type = cudaMemLocationTypeDevice;
    // props.location.id   = 0; // device 0
    // CUDA_CHECK(cudaMemPoolCreate(&pool, &props));

    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < ALLOC_ITERS; ++i) {
        void* ptr = nullptr;
        // [E2-T23] TODO [REQUIRED-2] cudaMallocFromPoolAsync(&ptr, ALLOC_SIZE, pool, stream)
        // [E2-T24] TODO [REQUIRED-2] cudaFreeAsync(ptr, stream)
        (void)ptr;
    }
    CUDA_CHECK(cudaStreamSynchronize(stream));
    auto t1 = std::chrono::steady_clock::now();

    double total_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    // [E2-T25]
    std::fprintf(stdout, "  total = %.3f ms, avg = %.3f ms/alloc\n\n",
                 total_ms, total_ms / ALLOC_ITERS);

    // [E2-T26] TODO [REQUIRED-2] cudaMemPoolDestroy(pool)
    (void)pool;
}

// ---------------------------------------------------------------------------
// [E2-T27] Test D: alloc -> kernel -> free pipeline with no intermediate sync
// ---------------------------------------------------------------------------
static void pipeline_async(cudaStream_t stream)
{
    NVTX_RANGE("E2/pipeline_async");
    // [E2-T28]
    std::fprintf(stdout, "--- pipeline alloc->kernel->free (%d rounds) ---\n", LOOP_ITERS);

    int grid = (N_ELEM + BLOCK - 1) / BLOCK;

    CudaEventTimer timer;
    timer.start(stream);

    for (int i = 0; i < LOOP_ITERS; ++i) {
        float* d_buf = nullptr;

        // [E2-T29] TODO [REQUIRED-3] cudaMallocAsync(&d_buf, ALLOC_SIZE, stream)

        // [E2-T30] TODO [REQUIRED-3] kernel_fill<<<grid, BLOCK, 0, stream>>>(d_buf, (float)i, N_ELEM)
        CUDA_CHECK_LAST();

        // [E2-T31] TODO [REQUIRED-3] cudaFreeAsync(d_buf, stream)

        (void)d_buf;
    }
    CUDA_CHECK(cudaStreamSynchronize(stream));
    timer.stop(stream);

    // [E2-T32]
    std::fprintf(stdout, "  cudaMallocAsync pipeline total = %.3f ms\n\n", timer.elapsed_ms());
    (void)grid;
}

// ---------------------------------------------------------------------------
// [E2-T33] Test E: same pipeline using cudaMalloc for comparison
// ---------------------------------------------------------------------------
static void pipeline_sync(cudaStream_t stream)
{
    NVTX_RANGE("E2/pipeline_sync");
    // [E2-T34]
    std::fprintf(stdout, "--- pipeline alloc->kernel->free (cudaMalloc version, %d rounds) ---\n", LOOP_ITERS);

    int grid = (N_ELEM + BLOCK - 1) / BLOCK;

    CudaEventTimer timer;
    timer.start(stream);

    for (int i = 0; i < LOOP_ITERS; ++i) {
        float* d_buf = nullptr;
        // [E2-T35] TODO [REQUIRED-4] cudaMalloc(&d_buf, ALLOC_SIZE)

        // [E2-T36] TODO [REQUIRED-4] kernel_fill<<<grid, BLOCK, 0, stream>>>(d_buf, (float)i, N_ELEM)
        CUDA_CHECK_LAST();

        // [E2-T37] TODO [REQUIRED-4] cudaFree(d_buf)

        (void)d_buf;
    }
    CUDA_CHECK(cudaStreamSynchronize(stream));
    timer.stop(stream);

    // [E2-T38]
    std::fprintf(stdout, "  cudaMalloc pipeline total = %.3f ms\n\n", timer.elapsed_ms());
    (void)grid;
}

// ---------------------------------------------------------------------------
// [E2-T39] main
// ---------------------------------------------------------------------------
int main()
{
    NVTX_RANGE("E2/main");
    print_device_info();

    // [E2-T40]
    std::fprintf(stdout, "=== E2: async memory allocation and memory pools ===\n\n");

    // [E2-T41] print currently available device memory
    size_t free_mem = 0, total_mem = 0;
    CUDA_CHECK(cudaMemGetInfo(&free_mem, &total_mem));
    // [E2-T42]
    std::fprintf(stdout, "device memory: free %.1f GB / total %.1f GB\n\n",
                 free_mem / 1073741824.0, total_mem / 1073741824.0);

    // [E2-T43] create work stream
    cudaStream_t stream = nullptr;
    CUDA_CHECK(cudaStreamCreate(&stream));

    // [E2-T44] -- run all tests --
    benchmark_cudaMalloc(stream);
    benchmark_cudaMallocAsync_default(stream);
    benchmark_custom_pool(stream);
    pipeline_async(stream);
    pipeline_sync(stream);

    // [E2-T45] -- TODO [ADVANCED-B] set memory pool release threshold --
    // cudaMemPool_t defaultPool;
    // CUDA_CHECK(cudaDeviceGetDefaultMemPool(&defaultPool, 0));
    // uint64_t threshold = UINT64_MAX; // never release back to OS
    // CUDA_CHECK(cudaMemPoolSetAttribute(defaultPool,
    //             cudaMemPoolAttrReleaseThreshold, &threshold));

    // [E2-T46] TODO [ADVANCED-C] compare cudaMallocManaged page fault behavior

    // [E2-T47] -- cleanup --
    CUDA_CHECK(cudaStreamDestroy(stream));

    // [E2-T48]
    std::fprintf(stdout, "hint: capture with 'ncu --set full -o e2_report ./E2_async_mem_pool',\n");
    // [E2-T49]
    std::fprintf(stdout, "      check Memory Workload Analysis for pool reuse.\n");
    return 0;
}
