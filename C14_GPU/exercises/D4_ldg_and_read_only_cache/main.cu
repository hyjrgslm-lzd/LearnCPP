// D4_ldg_and_read_only_cache/main.cu
// [D4-T01] Exercise D4: __ldg and read-only cache
// [D4-T02] Memory access optimization and L1TEX hit rate
//
// [D4-T03] Learning goals:
// [D4-T04]   - __ldg generates ld.global.nc, uses read-only cache (bypasses L1 data cache)
// [D4-T05]   - compare PTX of plain pointer / __ldg / const __restrict__ pointer
// [D4-T06]   - in memory-bound scenarios, __ldg can improve L2 / DRAM hit rate
// [D4-T07]   - in compute-bound scenarios, __ldg has limited effect
// [D4-T08]   - Nsight Compute: view L1TEX hit rate and Memory Throughput
//
// Build: cmake --build build --target D4_ldg_and_read_only_cache
// PTX inspection: nvcc -arch sm_90a -ptx main.cu -o main.ptx && grep "ld.global" main.ptx
// Nsight: ncu --set full -o d4_profile ./D4_ldg_and_read_only_cache

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cuda_runtime.h>

#include "common/cuda_check.cuh"
#include "common/timer.cuh"
#include "common/device_info.cuh"
#include "common/nvtx_range.cuh"

// ------------------------------------------------------------
// [D4-T09] Constants
// ------------------------------------------------------------
constexpr int N          = 1 << 22; // ~4M floats
constexpr int BLOCK_SIZE = 256;
constexpr int GRID_SIZE  = (N + BLOCK_SIZE - 1) / BLOCK_SIZE;

// ------------------------------------------------------------
// [D4-T10] Kernel 1: plain pointer read (no optimization)
// [D4-T11] TODO [REQUIRED-1] control group A
// ------------------------------------------------------------
__global__ void copy_plain(const float* __restrict__ in, float* out, int n)
{
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid < n) {
        // [D4-T12] TODO [REQUIRED-1] plain read: generates ld.global.ca (with L1 cache)
        out[tid] = in[tid]; // [D4-T13] plain read
    }
}

// ------------------------------------------------------------
// [D4-T14] Kernel 2: __ldg read (uses read-only / texture cache)
// [D4-T15] TODO [REQUIRED-1] control group B
// ------------------------------------------------------------
__global__ void copy_with_ldg(const float* in, float* out, int n)
{
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid < n) {
        // [D4-T16] TODO [REQUIRED-1] use __ldg, generates ld.global.nc
        // out[tid] = __ldg(&in[tid]); // TODO: uncomment

        out[tid] = in[tid]; // [D4-T17] stub: remove this line after uncommenting above
    }
}

// ------------------------------------------------------------
// [D4-T18] Kernel 3: const __restrict__ pointer (compiler may auto-emit __ldg)
// [D4-T19] TODO [REQUIRED-5]
// ------------------------------------------------------------
__global__ void copy_restrict(const float* __restrict__ in, float* out, int n)
{
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid < n) {
        // [D4-T20] TODO [REQUIRED-5] observe whether const __restrict__ auto-emits ld.global.nc
        out[tid] = in[tid];
    }
}

// ------------------------------------------------------------
// [D4-T21] Kernel 4: memory-bound scenario (read 1 float per thread, minimal compute)
// [D4-T22] TODO [REQUIRED-4] - here __ldg has visible effect
// ------------------------------------------------------------
__global__ void memory_bound_plain(const float* in, float* out, int n)
{
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid < n) {
        float v = in[tid];
        out[tid] = v + 1.0f; // [D4-T23] minimal compute, bottleneck in memory
    }
}

__global__ void memory_bound_ldg(const float* in, float* out, int n)
{
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid < n) {
        // [D4-T24] TODO [REQUIRED-4]:
        // float v = __ldg(&in[tid]);
        // out[tid] = v + 1.0f;

        out[tid] = in[tid] + 1.0f; // stub
    }
}

// ------------------------------------------------------------
// [D4-T25] Kernel 5: compute-bound scenario (heavy float compute, memory not bottleneck)
// [D4-T26] TODO [REQUIRED-4] - here __ldg has limited effect
// ------------------------------------------------------------
__global__ void compute_bound_plain(const float* in, float* out, int n)
{
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid < n) {
        float v = in[tid];
        // [D4-T27] heavy compute (~64 float ops)
        for (int i = 0; i < 64; ++i) v = v * 1.0001f + 0.0001f;
        out[tid] = v;
    }
}

__global__ void compute_bound_ldg(const float* in, float* out, int n)
{
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid < n) {
        // [D4-T28] TODO [REQUIRED-4]:
        // float v = __ldg(&in[tid]);
        float v = in[tid]; // stub
        for (int i = 0; i < 64; ++i) v = v * 1.0001f + 0.0001f;
        out[tid] = v;
    }
}

// ------------------------------------------------------------
// [D4-T29] Main program
// ------------------------------------------------------------
int main()
{
    print_device_info(0);
    NVTX_RANGE("D4/main");

    // [D4-T30]
    printf("Array size: N=%d (%.1f MB)\n\n", N, N * sizeof(float) / 1e6f);

    float *d_in = nullptr, *d_out = nullptr;
    CUDA_CHECK(cudaMalloc(&d_in,  N * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_out, N * sizeof(float)));

    // [D4-T31] initialize input
    {
        float* h_init = new float[N];
        for (int i = 0; i < N; ++i) h_init[i] = static_cast<float>(i) * 0.001f;
        CUDA_CHECK(cudaMemcpy(d_in, h_init, N * sizeof(float), cudaMemcpyHostToDevice));
        delete[] h_init;
    }

    CudaEventTimer timer;
    const int REPS = 10; // [D4-T32] average over multiple runs

    // ------------------------------------------------------------
    // [D4-T33] Test 1: plain copy vs __ldg copy
    // ------------------------------------------------------------
    {
        NVTX_RANGE("D4/copy_plain");
        float total = 0;
        for (int r = 0; r < REPS; ++r) {
            timer.start();
            copy_plain<<<GRID_SIZE, BLOCK_SIZE>>>(d_in, d_out, N);
            CUDA_CHECK(cudaGetLastError());
            timer.stop();
            total += timer.elapsed_ms();
        }
        printf("[copy_plain]   avg=%.3f ms  bw=%.1f GB/s\n",
               total/REPS, 2.0f*N*sizeof(float)/(total/REPS*1e6f));
    }
    {
        NVTX_RANGE("D4/copy_ldg");
        float total = 0;
        for (int r = 0; r < REPS; ++r) {
            timer.start();
            copy_with_ldg<<<GRID_SIZE, BLOCK_SIZE>>>(d_in, d_out, N);
            CUDA_CHECK(cudaGetLastError());
            timer.stop();
            total += timer.elapsed_ms();
        }
        // [D4-T34]
        printf("[copy_ldg]     avg=%.3f ms  bw=%.1f GB/s  (stub=plain, difference visible after TODO)\n",
               total/REPS, 2.0f*N*sizeof(float)/(total/REPS*1e6f));
    }
    {
        NVTX_RANGE("D4/copy_restrict");
        float total = 0;
        for (int r = 0; r < REPS; ++r) {
            timer.start();
            copy_restrict<<<GRID_SIZE, BLOCK_SIZE>>>(d_in, d_out, N);
            CUDA_CHECK(cudaGetLastError());
            timer.stop();
            total += timer.elapsed_ms();
        }
        printf("[copy_restrict] avg=%.3f ms  bw=%.1f GB/s\n",
               total/REPS, 2.0f*N*sizeof(float)/(total/REPS*1e6f));
    }

    // ------------------------------------------------------------
    // [D4-T35] Test 2: memory-bound scenario
    // ------------------------------------------------------------
    // [D4-T36]
    printf("\n--- memory-bound scenario (minimal compute) ---\n");
    {
        NVTX_RANGE("D4/mem_bound_plain");
        float total = 0;
        for (int r = 0; r < REPS; ++r) {
            timer.start();
            memory_bound_plain<<<GRID_SIZE, BLOCK_SIZE>>>(d_in, d_out, N);
            CUDA_CHECK(cudaGetLastError());
            timer.stop();
            total += timer.elapsed_ms();
        }
        printf("[mem_plain]    avg=%.3f ms\n", total/REPS);
    }
    {
        NVTX_RANGE("D4/mem_bound_ldg");
        float total = 0;
        for (int r = 0; r < REPS; ++r) {
            timer.start();
            memory_bound_ldg<<<GRID_SIZE, BLOCK_SIZE>>>(d_in, d_out, N);
            CUDA_CHECK(cudaGetLastError());
            timer.stop();
            total += timer.elapsed_ms();
        }
        // [D4-T37]
        printf("[mem_ldg]      avg=%.3f ms  (after TODO should be slightly faster than plain)\n", total/REPS);
    }

    // ------------------------------------------------------------
    // [D4-T38] Test 3: compute-bound scenario
    // ------------------------------------------------------------
    // [D4-T39]
    printf("\n--- compute-bound scenario (heavy float compute) ---\n");
    {
        NVTX_RANGE("D4/compute_bound_plain");
        float total = 0;
        for (int r = 0; r < REPS; ++r) {
            timer.start();
            compute_bound_plain<<<GRID_SIZE, BLOCK_SIZE>>>(d_in, d_out, N);
            CUDA_CHECK(cudaGetLastError());
            timer.stop();
            total += timer.elapsed_ms();
        }
        printf("[cmp_plain]    avg=%.3f ms\n", total/REPS);
    }
    {
        NVTX_RANGE("D4/compute_bound_ldg");
        float total = 0;
        for (int r = 0; r < REPS; ++r) {
            timer.start();
            compute_bound_ldg<<<GRID_SIZE, BLOCK_SIZE>>>(d_in, d_out, N);
            CUDA_CHECK(cudaGetLastError());
            timer.stop();
            total += timer.elapsed_ms();
        }
        // [D4-T40]
        printf("[cmp_ldg]      avg=%.3f ms  (expected close to plain - compute-bound)\n",
               total/REPS);
    }
    CUDA_CHECK(cudaDeviceSynchronize());

    // ------------------------------------------------------------
    // [D4-T41] TODO [REQUIRED-3] PTX inspection: nvcc -arch sm_90a -ptx main.cu
    // [D4-T42]                    search ld.global.ca vs ld.global.nc
    // [D4-T43] TODO [REQUIRED-6] Nsight Compute: view L1TEX hit rate and Memory Throughput
    // [D4-T44] TODO [ADVANCED]   texture cache version comparison
    // [D4-T45] TODO [ADVANCED]   __ldg in compute-bound kernel: observe no clear change
    // ------------------------------------------------------------

    CUDA_CHECK(cudaFree(d_in));
    CUDA_CHECK(cudaFree(d_out));

    // [D4-T46]
    printf("\n[D4] done. Use ncu --metrics l1tex__t_sector_hit_rate.pct to view L1TEX hit rate.\n");
    return 0;
}
