// D5_occupancy_and_launch_bounds/main.cu
// [D5-T01] Exercise D5: Occupancy and __launch_bounds__
// [D5-T02] Register pressure, spill, and occupancy trade-offs
//
// [D5-T03] Learning goals:
// [D5-T04]   - cudaOccupancyMaxPotentialBlockSize queries recommended blockDim
// [D5-T05]   - __launch_bounds__(maxThreads, minBlocksPerSM) three-way comparison
// [D5-T06]   - register spill visible as st.local / ld.local in PTX
// [D5-T07]   - Nsight Compute occupancy data and -Xptxas=-v stats
//
// Build (with register stats):
//   cmake --build build --target D5_occupancy_and_launch_bounds
//   or manual: nvcc -arch sm_90a -Xptxas=-v main.cu
// Run: ./D5_occupancy_and_launch_bounds

#include <cstdio>
#include <cstdlib>
#include <cuda_runtime.h>

#include "common/cuda_check.cuh"
#include "common/timer.cuh"
#include "common/device_info.cuh"
#include "common/nvtx_range.cuh"

// ------------------------------------------------------------
// [D5-T08] Constants
// ------------------------------------------------------------
constexpr int N          = 1 << 20; // 1M elements
constexpr int BLOCK_SIZE = 256;

// ------------------------------------------------------------
// [D5-T09] Kernel 1: no __launch_bounds__ (baseline)
// [D5-T10] TODO [REQUIRED-1] - use cudaOccupancyMaxPotentialBlockSize to query recommended blockDim
// ------------------------------------------------------------
__global__ void kernel_no_bounds(const float* in, float* out, int n)
{
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid < n) {
        float v = in[tid];
        out[tid] = v * 2.0f + 1.0f;
    }
}

// ------------------------------------------------------------
// [D5-T11] Kernel 2: __launch_bounds__(256, 0) - max 256 threads, no minBlocks constraint
// [D5-T12] TODO [REQUIRED-2] - control group A
// ------------------------------------------------------------
__launch_bounds__(256, 0)
__global__ void kernel_lb_256_0(const float* in, float* out, int n)
{
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid < n) {
        float v = in[tid];
        out[tid] = v * 2.0f + 1.0f;
    }
}

// ------------------------------------------------------------
// [D5-T13] Kernel 3: __launch_bounds__(256, 2) - at least 2 blocks/SM
// [D5-T14] TODO [REQUIRED-2] - control group B (compiler may reduce registers)
// ------------------------------------------------------------
__launch_bounds__(256, 2)
__global__ void kernel_lb_256_2(const float* in, float* out, int n)
{
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid < n) {
        float v = in[tid];
        out[tid] = v * 2.0f + 1.0f;
    }
}

// ------------------------------------------------------------
// [D5-T15] Kernel 4: __launch_bounds__(256, 4) - more aggressive minBlocks
// [D5-T16] TODO [REQUIRED-2] - control group C
// ------------------------------------------------------------
__launch_bounds__(256, 4)
__global__ void kernel_lb_256_4(const float* in, float* out, int n)
{
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid < n) {
        float v = in[tid];
        out[tid] = v * 2.0f + 1.0f;
    }
}

// ------------------------------------------------------------
// [D5-T17] Kernel 5: register-heavy kernel (induce spill)
// [D5-T18] TODO [REQUIRED-5] - use nvcc -Xptxas -v to check local memory usage
// ------------------------------------------------------------
__global__ void kernel_heavy_registers(const float* in, float* out, int n)
{
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid >= n) return;

    // [D5-T19] deliberately use many register variables (avoid optimizing away)
    float v0  = in[tid];
    float v1  = v0  * 1.01f;  float v2  = v1  + 0.01f;
    float v3  = v2  * 1.01f;  float v4  = v3  + 0.01f;
    float v5  = v4  * 1.01f;  float v6  = v5  + 0.01f;
    float v7  = v6  * 1.01f;  float v8  = v7  + 0.01f;
    float v9  = v8  * 1.01f;  float v10 = v9  + 0.01f;
    float v11 = v10 * 1.01f;  float v12 = v11 + 0.01f;
    float v13 = v12 * 1.01f;  float v14 = v13 + 0.01f;
    float v15 = v14 * 1.01f;  float v16 = v15 + 0.01f;
    float v17 = v16 * 1.01f;  float v18 = v17 + 0.01f;
    float v19 = v18 * 1.01f;  float v20 = v19 + 0.01f;
    float v21 = v20 * 1.01f;  float v22 = v21 + 0.01f;
    float v23 = v22 * 1.01f;  float v24 = v23 + 0.01f;
    float v25 = v24 * 1.01f;  float v26 = v25 + 0.01f;
    float v27 = v26 * 1.01f;  float v28 = v27 + 0.01f;
    float v29 = v28 * 1.01f;  float v30 = v29 + 0.01f;
    float v31 = v30 * 1.01f;  float v32 = v31 + 0.01f;

    // [D5-T20] prevent compiler from eliminating (use all variables)
    // [D5-T21] TODO [REQUIRED-5] uncomment #pragma unroll, observe register impact
    float result = v0 + v1 + v2 + v3 + v4 + v5 + v6 + v7
                 + v8 + v9 + v10 + v11 + v12 + v13 + v14 + v15
                 + v16 + v17 + v18 + v19 + v20 + v21 + v22 + v23
                 + v24 + v25 + v26 + v27 + v28 + v29 + v30 + v31 + v32;

    out[tid] = result;
}

// ------------------------------------------------------------
// [D5-T22] Helper: print occupancy info
// ------------------------------------------------------------
template<typename KernelT>
static void print_occupancy(const char* name, KernelT kernel, int block_size, size_t smem = 0)
{
    int active_blocks = 0;
    CUDA_CHECK(cudaOccupancyMaxActiveBlocksPerMultiprocessor(
        &active_blocks, kernel, block_size, smem));

    cudaDeviceProp prop{};
    CUDA_CHECK(cudaGetDeviceProperties(&prop, 0));
    int max_warps    = prop.maxThreadsPerMultiProcessor / prop.warpSize;
    int active_warps = active_blocks * (block_size / prop.warpSize);
    float occ        = (max_warps > 0) ? 100.0f * active_warps / max_warps : 0.0f;

    printf("  %-20s  block=%d  active_blocks/SM=%d  occupancy=%.1f%%\n",
           name, block_size, active_blocks, occ);
}

// ------------------------------------------------------------
// [D5-T23] Main program
// ------------------------------------------------------------
int main()
{
    print_device_info(0);
    NVTX_RANGE("D5/main");

    float *d_in = nullptr, *d_out = nullptr;
    CUDA_CHECK(cudaMalloc(&d_in,  N * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_out, N * sizeof(float)));
    {
        float* h = new float[N];
        for (int i = 0; i < N; ++i) h[i] = static_cast<float>(i);
        CUDA_CHECK(cudaMemcpy(d_in, h, N * sizeof(float), cudaMemcpyHostToDevice));
        delete[] h;
    }

    // ------------------------------------------------------------
    // [D5-T24] TODO [REQUIRED-1] cudaOccupancyMaxPotentialBlockSize: recommended blockDim
    // ------------------------------------------------------------
    {
        int min_grid = 0, opt_block = 0;
        // [D5-T25] TODO [REQUIRED-1]:
        // CUDA_CHECK(cudaOccupancyMaxPotentialBlockSize(
        //     &min_grid, &opt_block, kernel_no_bounds, 0, 0));
        // [D5-T26]
        printf("\n[REQUIRED-1] cudaOccupancyMaxPotentialBlockSize:\n");
        // [D5-T27]
        printf("  minGridSize=%d  optBlockSize=%d  (stub=0, fill in after TODO)\n",
               min_grid, opt_block);
    }

    // ------------------------------------------------------------
    // [D5-T28] TODO [REQUIRED-3] print occupancy of each kernel
    // ------------------------------------------------------------
    // [D5-T29]
    printf("\n[REQUIRED-3] occupancy per kernel (blockDim=%d):\n", BLOCK_SIZE);
    print_occupancy("no_bounds",   kernel_no_bounds, BLOCK_SIZE);
    print_occupancy("lb_256_0",    kernel_lb_256_0,  BLOCK_SIZE);
    print_occupancy("lb_256_2",    kernel_lb_256_2,  BLOCK_SIZE);
    print_occupancy("lb_256_4",    kernel_lb_256_4,  BLOCK_SIZE);
    print_occupancy("heavy_regs",  kernel_heavy_registers, BLOCK_SIZE);

    int grid = (N + BLOCK_SIZE - 1) / BLOCK_SIZE;
    CudaEventTimer timer;
    const int REPS = 5;

    // ------------------------------------------------------------
    // [D5-T30] Performance comparison
    // ------------------------------------------------------------
    // [D5-T31]
    printf("\n[REQUIRED-4] performance (REPS=%d, grid=%d block=%d):\n", REPS, grid, BLOCK_SIZE);

    auto bench = [&](const char* name, auto kernel) {
        NVTX_RANGE(name);
        float total = 0;
        for (int r = 0; r < REPS; ++r) {
            timer.start();
            kernel<<<grid, BLOCK_SIZE>>>(d_in, d_out, N);
            CUDA_CHECK(cudaGetLastError());
            timer.stop();
            total += timer.elapsed_ms();
        }
        printf("  %-20s  avg=%.3f ms\n", name, total / REPS);
    };

    bench("no_bounds",  kernel_no_bounds);
    bench("lb_256_0",   kernel_lb_256_0);
    bench("lb_256_2",   kernel_lb_256_2);
    bench("lb_256_4",   kernel_lb_256_4);

    // [D5-T32] heavy registers kernel (may trigger spill)
    {
        NVTX_RANGE("D5/heavy_regs");
        float total = 0;
        for (int r = 0; r < REPS; ++r) {
            timer.start();
            kernel_heavy_registers<<<grid, BLOCK_SIZE>>>(d_in, d_out, N);
            CUDA_CHECK(cudaGetLastError());
            timer.stop();
            total += timer.elapsed_ms();
        }
        // [D5-T33]
        printf("  %-20s  avg=%.3f ms  (check PTX for st.local/ld.local)\n",
               "heavy_registers", total / REPS);
    }

    CUDA_CHECK(cudaDeviceSynchronize());

    // ------------------------------------------------------------
    // [D5-T34] TODO [REQUIRED-6] register stats: nvcc -Xptxas -v
    // [D5-T35]   format: ptxas info: Used X registers, Y bytes lmem, ...
    // [D5-T36]   lmem > 0 indicates spill
    // [D5-T37] TODO [ADVANCED] sweep register usage, plot occupancy vs registers
    // [D5-T38] TODO [ADVANCED] effect of #pragma unroll on registers
    // ------------------------------------------------------------

    CUDA_CHECK(cudaFree(d_in));
    CUDA_CHECK(cudaFree(d_out));

    // [D5-T39]
    printf("\n[D5] done. Use nvcc -Xptxas -v for register stats, ncu for Achieved Occupancy.\n");
    return 0;
}
