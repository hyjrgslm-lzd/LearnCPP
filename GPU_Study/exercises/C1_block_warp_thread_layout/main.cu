// [C1-T01] C1_block_warp_thread_layout/main.cu
// [C1-T02] Exercise C1: block / warp / thread three-level layout and occupancy
//
// [C1-T03] Goals:
//   - Master indexing of blockIdx / threadIdx
//   - Use threadIdx.x/32 for warp ID, threadIdx.x%32 for lane ID
//   - Compute theoretical occupancy for various blockDim values, and compare
//     with cudaOccupancyMaxActiveBlocksPerMultiprocessor
//
// [C1-T04] Build (from project root):
//   cmake --build build --target C1_block_warp_thread_layout
// Run:
//   ./C1_block_warp_thread_layout
// Nsight Compute:
//   ncu --set full -o c1_profile ./C1_block_warp_thread_layout

#include <cstdio>
#include <cstdlib>
#include <cuda_runtime.h>

#include "common/cuda_check.cuh"
#include "common/timer.cuh"
#include "common/device_info.cuh"
#include "common/nvtx_range.cuh"

// ------------------------------------------------------------
// [C1-T05] Data structure: each thread records its identity info
// ------------------------------------------------------------
struct ThreadInfo {
    int block_idx;   // [C1-T06] blockIdx.x
    int thread_idx;  // [C1-T07] threadIdx.x (after linearization)
    int warp_id;     // [C1-T08] threadIdx_linear / 32
    int lane_id;     // [C1-T09] threadIdx_linear % 32
};

// ------------------------------------------------------------
// [C1-T10] Kernel: record each thread's block/warp/lane info
// ------------------------------------------------------------
__global__ void layout_info_kernel(ThreadInfo* out, int n)
{
    // [C1-T11] TODO [REQUIRED-2] compute linear threadIdx (supports 3D blockDim)
    int linear_tid = threadIdx.z * blockDim.x * blockDim.y
                   + threadIdx.y * blockDim.x
                   + threadIdx.x;

    int global_tid = blockIdx.x * (blockDim.x * blockDim.y * blockDim.z) + linear_tid;
    if (global_tid >= n) return;

    // [C1-T12] TODO [REQUIRED-2] fill in warp ID and lane ID
    out[global_tid].block_idx  = blockIdx.x;
    out[global_tid].thread_idx = linear_tid;
    out[global_tid].warp_id    = 0; // TODO: linear_tid / 32
    out[global_tid].lane_id    = 0; // TODO: linear_tid % 32
}

// ------------------------------------------------------------
// [C1-T13] Kernel: only let lane 0 print (to reduce output volume)
// ------------------------------------------------------------
__global__ void print_lane0_kernel(int total_threads)
{
    int linear_tid = threadIdx.z * blockDim.x * blockDim.y
                   + threadIdx.y * blockDim.x
                   + threadIdx.x;
    int global_tid = blockIdx.x * (blockDim.x * blockDim.y * blockDim.z) + linear_tid;
    if (global_tid >= total_threads) return;

    // [C1-T14] TODO [REQUIRED-2] only let lane 0 (linear_tid % 32 == 0) print
    int warp_id = linear_tid / 32;  // TODO: students replace with actual computation
    int lane_id = linear_tid % 32;  // TODO: students replace with actual computation
    (void)warp_id; (void)lane_id;

    // [C1-T15] TODO [REQUIRED-2] uncomment; should print info for first lane of each warp
    // if (lane_id == 0) {
    //     printf("block=%d warp=%d lane=%d global_tid=%d\n",
    //            blockIdx.x, warp_id, lane_id, global_tid);
    // }
}

// ------------------------------------------------------------
// [C1-T16] Helper: query occupancy for the given blockDim
// ------------------------------------------------------------
static void query_occupancy(int block_dim, int smem_per_block_bytes)
{
    int active_blocks = 0;
    // [C1-T17] TODO [REQUIRED-5] call cudaOccupancyMaxActiveBlocksPerMultiprocessor
    // cudaOccupancyMaxActiveBlocksPerMultiprocessor(
    //     &active_blocks, layout_info_kernel, block_dim, smem_per_block_bytes);

    cudaDeviceProp prop{};
    CUDA_CHECK(cudaGetDeviceProperties(&prop, 0));

    int warps_per_block    = (block_dim + 31) / 32;
    int active_warps       = active_blocks * warps_per_block;
    int max_warps_per_sm   = prop.maxThreadsPerMultiProcessor / prop.warpSize;
    float occupancy        = (max_warps_per_sm > 0)
                             ? static_cast<float>(active_warps) / max_warps_per_sm
                             : 0.0f;

    printf("  blockDim=%4d  warps/block=%2d  active_blocks/SM=%d"
           "  active_warps/SM=%2d  max_warps/SM=%2d  occupancy=%.2f\n",
           block_dim, warps_per_block, active_blocks,
           active_warps, max_warps_per_sm, occupancy);
}

// ------------------------------------------------------------
// [C1-T18] Main program
// ------------------------------------------------------------
int main()
{
    // [C1-T19] print basic device information
    print_device_info(0);

    NVTX_RANGE("C1/main");

    constexpr int N             = 256; // [C1-T20] for demo, no need to be large
    constexpr int SMEM_PER_BLOCK = 8 * 1024; // [C1-T21] assume each block uses 8 KB smem

    // ------------------------------------------------------------
    // [C1-T22] Allocate device memory
    // ------------------------------------------------------------
    ThreadInfo* d_info = nullptr;
    CUDA_CHECK(cudaMalloc(&d_info, N * sizeof(ThreadInfo)));

    // ------------------------------------------------------------
    // [C1-T23] TODO [REQUIRED-1/3/4] try multiple blockDim values, observe warp distribution
    // ------------------------------------------------------------
    int block_dim_list[] = {32, 64, 128, 256, 512, 1024};

    for (int bd : block_dim_list)
    {
        // [C1-T24] TODO [REQUIRED-3] compute the warp count for this blockDim
        int warps = (bd + 31) / 32;
        // [C1-T25]
        printf("\n[blockDim=%d] warp count = %d\n", bd, warps);

        // [C1-T26] only launch when N >= bd (demo case where blockDim <= N)
        if (bd > N) {
            printf("  (skip: blockDim > N=%d)\n", N);
            continue;
        }

        int grid_dim = (N + bd - 1) / bd;
        // [C1-T27]
        printf("  launch config: grid=%d block=%d\n", grid_dim, bd);

        CUDA_CHECK(cudaMemset(d_info, 0, N * sizeof(ThreadInfo)));

        layout_info_kernel<<<grid_dim, bd>>>(d_info, N);
        CUDA_CHECK(cudaGetLastError());
        CUDA_CHECK(cudaDeviceSynchronize());

        print_lane0_kernel<<<grid_dim, bd>>>(N);
        CUDA_CHECK(cudaGetLastError());
        CUDA_CHECK(cudaDeviceSynchronize());

        // [C1-T28] TODO [REQUIRED-5/6] query occupancy
        query_occupancy(bd, SMEM_PER_BLOCK);
    }

    // ------------------------------------------------------------
    // [C1-T29] TODO [REQUIRED-4] 2D blockDim demo
    // [C1-T30] Three blockDim configurations to compare: (256,1,1) / (128,2,1) / (64,4,1)
    // ------------------------------------------------------------
    // [C1-T31]
    printf("\n--- 2D blockDim comparison ---\n");
    {
        // [C1-T32] Config A: (256,1,1) - typical 1D layout
        dim3 bd_a(256, 1, 1);
        dim3 gd_a((N + bd_a.x - 1) / bd_a.x);
        // [C1-T33]
        printf("Config A (256,1,1): grid=(%d,1,1)\n", gd_a.x);
        // [C1-T34] TODO: suitable for 1D vector operations

        // [C1-T35] Config B: (128,2,1) - suitable for 2D tiling
        dim3 bd_b(128, 2, 1);
        dim3 gd_b((N + bd_b.x - 1) / bd_b.x, 1);
        // [C1-T36]
        printf("Config B (128,2,1): grid=(%d,%d,1)\n", gd_b.x, gd_b.y);
        // [C1-T37] TODO: suitable for row/column tiling of 2D data

        // [C1-T38] Config C: (64,4,1) - more parallelism along y
        dim3 bd_c(64, 4, 1);
        dim3 gd_c((N + bd_c.x - 1) / bd_c.x, 1);
        // [C1-T39]
        printf("Config C (64,4,1):  grid=(%d,%d,1)\n", gd_c.x, gd_c.y);
        // [C1-T40] TODO: suitable for warp-tiling, y dimension maps to rows
    }

    // ------------------------------------------------------------
    // [C1-T41] TODO [ADVANCED] 2D addressing with blockDim=(32,32,1)
    // [C1-T42] TODO [ADVANCED] use cudaOccupancyMaxPotentialBlockSize to query optimal blockDim
    // ------------------------------------------------------------

    CUDA_CHECK(cudaFree(d_info));

    // [C1-T43]
    printf("\n[C1] done. Use Nsight Compute --set full to verify occupancy numbers.\n");
    return 0;
}
