// ============================================================
// [F1-T01] Exercise F1: Nsight Systems timeline + NVTX annotation
// [F1-T02] Goals:
//   - 3-stage pipeline: HtoD copy / kernel compute / DtoH copy,
//     each wrapped with NVTX_RANGE
//   - Two kernels launched on different streams; observe overlap
//   - nsys profile -o trace --stats=true, then view in GUI
//   - NVTX ranges link app logic to underlying GPU operations
// [F1-T03] Build:  cmake --build build --target F1_nsight_systems_timeline
// [F1-T04] Capture: nsys profile -o trace --stats=true --trace=cuda,nvtx ./F1_nsight_systems_timeline
// [F1-T05] Run:    ./F1_nsight_systems_timeline
// ============================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cuda_runtime.h>

#include "common/cuda_check.cuh"
#include "common/timer.cuh"
#include "common/device_info.cuh"
#include "common/nvtx_range.cuh"

// ------------------------------------------------------------
// [F1-T06] Constants
// ------------------------------------------------------------
// [F1-T07] 256 MB / sizeof(float) = 64M elements
constexpr int   N_ELEMS     = 64 * 1024 * 1024;   // [F1-T08] 256 MB data volume
constexpr int   BLOCK_SIZE  = 256;
constexpr int   ITERS       = 32;                   // [F1-T09] kernel work amount (controls runtime)

// ------------------------------------------------------------
// [F1-T10] Kernel A: vector add (Stream 0)
//   Trait: simple arithmetic, ~10-50 ms (driven by ITERS)
//   arithmetic intensity ~ 4 ops / 12 bytes ~ 0.33 FLOP/byte (memory-bound)
// [F1-T11] TODO [REQUIRED-1] complete kernel body: each thread runs ITERS accumulations
// ------------------------------------------------------------
__global__ void kernel_vector_add(
    const float* __restrict__ a,
    const float* __restrict__ b,
    float* __restrict__ c,
    int n)
{
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid >= n) return;

    // [F1-T12] TODO [REQUIRED-1]: implement vector add with ITERS-iter loop
    //   to artificially extend execution time
    //   float val = a[tid] + b[tid];
    //   for (int i = 0; i < ITERS; ++i) val = val * 1.0001f + b[tid];
    //   c[tid] = val;
    c[tid] = 0.0f; // stub
}

// ------------------------------------------------------------
// [F1-T13] Kernel B: vector scale (Stream 1, concurrent with Kernel A)
//   Trait: single op, lighter than Kernel A; helps observe overlap
// [F1-T14] TODO [REQUIRED-1] complete kernel body: each thread multiplies input by scale
// ------------------------------------------------------------
__global__ void kernel_vector_scale(
    const float* __restrict__ in,
    float* __restrict__ out,
    float scale,
    int n)
{
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid >= n) return;

    // [F1-T15] TODO [REQUIRED-1]: out[tid] = in[tid] * scale;
    out[tid] = 0.0f; // stub
}

// ------------------------------------------------------------
// [F1-T16] Helper: print bandwidth utilization
// ------------------------------------------------------------
static void print_bandwidth(const char* tag, long long bytes, float ms)
{
    float gb_s = static_cast<float>(bytes) / (ms * 1e6f);
    printf("  %-20s  %.3f ms  %.1f GB/s\n", tag, ms, gb_s);
}

// ------------------------------------------------------------
// [F1-T17] main
// ------------------------------------------------------------
int main()
{
    std::puts("[F1_nsight_systems_timeline]");
    print_device_info(0);

    NVTX_RANGE("F1/main");

    // ------------------------------------------------------------
    // [F1-T18] Allocate host / device memory
    // ------------------------------------------------------------
    const size_t bytes = static_cast<size_t>(N_ELEMS) * sizeof(float);

    float* h_a   = nullptr;
    float* h_b   = nullptr;
    float* h_c   = nullptr;
    float* h_out = nullptr;

    // [F1-T19] use pinned memory to fully exploit PCIe bandwidth
    CUDA_CHECK(cudaMallocHost(&h_a,   bytes));
    CUDA_CHECK(cudaMallocHost(&h_b,   bytes));
    CUDA_CHECK(cudaMallocHost(&h_c,   bytes));
    CUDA_CHECK(cudaMallocHost(&h_out, bytes));

    float* d_a   = nullptr;
    float* d_b   = nullptr;
    float* d_c   = nullptr;
    float* d_out = nullptr;

    CUDA_CHECK(cudaMalloc(&d_a,   bytes));
    CUDA_CHECK(cudaMalloc(&d_b,   bytes));
    CUDA_CHECK(cudaMalloc(&d_c,   bytes));
    CUDA_CHECK(cudaMalloc(&d_out, bytes));

    // [F1-T20] initialize host data
    for (int i = 0; i < N_ELEMS; ++i) {
        h_a[i] = static_cast<float>(i % 1024) / 1024.0f;
        h_b[i] = static_cast<float>((i + 1) % 1024) / 1024.0f;
    }

    // ------------------------------------------------------------
    // [F1-T21] Create two streams (stream1 hosts Kernel B for concurrency)
    // ------------------------------------------------------------
    cudaStream_t stream0, stream1;
    CUDA_CHECK(cudaStreamCreate(&stream0));
    CUDA_CHECK(cudaStreamCreate(&stream1));

    CudaEventTimer timer;

    // ------------------------------------------------------------
    // [F1-T22] Stage 1: HtoD copy (256 MB)
    // ------------------------------------------------------------
    printf("\n--- Stage 1: HtoD copy (256 MB x2) ---\n");
    {
        NVTX_RANGE_COLOR("HtoD_256MB", 0xFF4080FF); // [F1-T23] blue
        timer.start();
        // [F1-T24] TODO [REQUIRED-2]: async copy d_a, d_b on stream0
        CUDA_CHECK(cudaMemcpyAsync(d_a, h_a, bytes, cudaMemcpyHostToDevice, stream0));
        CUDA_CHECK(cudaMemcpyAsync(d_b, h_b, bytes, cudaMemcpyHostToDevice, stream0));
        CUDA_CHECK(cudaStreamSynchronize(stream0));
        timer.stop();
        print_bandwidth("HtoD", static_cast<long long>(bytes) * 2, timer.elapsed_ms());
    }

    // ------------------------------------------------------------
    // [F1-T25] Stage 2A: Kernel A on stream0 (compute phase)
    // ------------------------------------------------------------
    printf("\n--- Stage 2A: Kernel A -- vector_add (stream0) ---\n");
    int grid = (N_ELEMS + BLOCK_SIZE - 1) / BLOCK_SIZE;
    printf("  launch: grid=%d block=%d stream=0\n", grid, BLOCK_SIZE);
    {
        NVTX_RANGE_COLOR("Kernel_A_stream0", 0xFF40FF40); // [F1-T26] green
        timer.start();
        // [F1-T27] TODO [REQUIRED-2]: launch on stream0 with proper args
        kernel_vector_add<<<grid, BLOCK_SIZE, 0, stream0>>>(d_a, d_b, d_c, N_ELEMS);
        CUDA_CHECK(cudaGetLastError());
        // [F1-T28] note: do not sync here so Kernel A and Kernel B can overlap
    }

    // ------------------------------------------------------------
    // [F1-T29] Stage 2B: Kernel B on stream1 (concurrent with Kernel A)
    // ------------------------------------------------------------
    printf("--- Stage 2B: Kernel B -- vector_scale (stream1, concurrent with A) ---\n");
    printf("  launch: grid=%d block=%d stream=1\n", grid, BLOCK_SIZE);
    {
        NVTX_RANGE_COLOR("Kernel_B_Stream1", 0xFFFF8040); // [F1-T30] orange
        // [F1-T31] TODO [REQUIRED-2]: launch on stream1 with scale=2.0f
        kernel_vector_scale<<<grid, BLOCK_SIZE, 0, stream1>>>(d_a, d_out, 2.0f, N_ELEMS);
        CUDA_CHECK(cudaGetLastError());
    }

    // [F1-T32] wait for both kernels before timing
    CUDA_CHECK(cudaStreamSynchronize(stream0));
    CUDA_CHECK(cudaStreamSynchronize(stream1));
    timer.stop();
    printf("  Kernel A+B total elapsed (with concurrency) = %.3f ms\n", timer.elapsed_ms());

    // ------------------------------------------------------------
    // [F1-T33] Stage 3: DtoH copy result
    // ------------------------------------------------------------
    printf("\n--- Stage 3: DtoH copy result (256 MB) ---\n");
    {
        NVTX_RANGE_COLOR("DtoH", 0xFFFF4040); // [F1-T34] red
        timer.start();
        // [F1-T35] TODO [REQUIRED-2]: async copy d_c back to h_c on stream0
        CUDA_CHECK(cudaMemcpyAsync(h_c, d_c, bytes, cudaMemcpyDeviceToHost, stream0));
        CUDA_CHECK(cudaStreamSynchronize(stream0));
        timer.stop();
        print_bandwidth("DtoH", static_cast<long long>(bytes), timer.elapsed_ms());
    }

    // ------------------------------------------------------------
    // [F1-T36] Stage 4: CPU-side computation (sleep simulation)
    // ------------------------------------------------------------
    printf("\n--- Stage 4: CPU compute (simulating host work) ---\n");
    {
        NVTX_RANGE_COLOR("CPU_Work", 0xFFFFFF40); // [F1-T37] yellow
        // [F1-T38] TODO [REQUIRED-3]: simulate host work (CPU loop or sleep_for)
        volatile float sum = 0.0f;
        for (int i = 0; i < N_ELEMS; i += 64) sum += h_c[i];
        printf("  CPU sum (every 64 elements) = %f\n", static_cast<float>(sum));
    }

    CUDA_CHECK(cudaDeviceSynchronize());

    // ------------------------------------------------------------
    // [F1-T39] TODO [REQUIRED-4] re-sample with nsys profile and
    //   verify in Nsight Systems GUI:
    //   - colored NVTX bars line up with CUDA operations
    //   - Kernel A and Kernel B overlap in time
    //   - DtoH overlaps with Kernel B (or not)
    //   Record observations as comments:
    //   // observation: Kernel A and B concurrent window = ??? ms
    //   // observation: DtoH overlaps Kernel B = ???

    // [F1-T40] TODO [REQUIRED-5] capture screenshot of timeline,
    //   annotate the meaning of each NVTX range color

    // [F1-T41] TODO [ADVANCED-1] add a third stream + Kernel C, more complex timeline
    // [F1-T42] TODO [ADVANCED-2] use NVTX domain to separate "data transfer" vs "compute"
    // [F1-T43] TODO [ADVANCED-3] tweak ITERS to elongate Kernel A and compare overlap

    // ------------------------------------------------------------
    // [F1-T44] Cleanup
    // ------------------------------------------------------------
    CUDA_CHECK(cudaStreamDestroy(stream0));
    CUDA_CHECK(cudaStreamDestroy(stream1));
    CUDA_CHECK(cudaFree(d_a));
    CUDA_CHECK(cudaFree(d_b));
    CUDA_CHECK(cudaFree(d_c));
    CUDA_CHECK(cudaFree(d_out));
    CUDA_CHECK(cudaFreeHost(h_a));
    CUDA_CHECK(cudaFreeHost(h_b));
    CUDA_CHECK(cudaFreeHost(h_c));
    CUDA_CHECK(cudaFreeHost(h_out));

    // [F1-T45]
    printf("\n[F1] done. nsys profile -o trace --stats=true and open trace.nsys-rep in Nsight Systems GUI.\n");
    return 0;
}
