// [C3-T01] C3_cooperative_groups/main.cu
// [C3-T02] Exercise C3: Cooperative Groups - this_thread_block + tiled_partition
//
// [C3-T03] Goals:
//   - Use cooperative_groups::this_thread_block() instead of __syncthreads()
//   - Use tiled_partition<32> / tiled_partition<16> to form sub-groups
//   - Compare readability and performance of raw-sync vs CG versions
//
// [C3-T04] Build: cmake --build build --target C3_cooperative_groups
// Run:   ./C3_cooperative_groups

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cuda_runtime.h>
#include <cooperative_groups.h>

#include "common/cuda_check.cuh"
#include "common/timer.cuh"
#include "common/device_info.cuh"
#include "common/nvtx_range.cuh"

namespace cg = cooperative_groups;

// ------------------------------------------------------------
// [C3-T05] Constants
// ------------------------------------------------------------
constexpr int BLOCK_SIZE = 256;
constexpr int N          = BLOCK_SIZE;

// ------------------------------------------------------------
// [C3-T06] Kernel 1: reduction with raw __syncthreads (reference baseline)
// [C3-T07] TODO [REQUIRED-4] control group
// ------------------------------------------------------------
__global__ void reduce_raw_sync(const int* in, int* out)
{
    __shared__ int smem[BLOCK_SIZE];
    int tid = threadIdx.x;

    smem[tid] = (tid < N) ? in[tid] : 0;
    __syncthreads();

    for (int s = BLOCK_SIZE / 2; s > 0; s >>= 1) {
        if (tid < s) {
            smem[tid] += smem[tid + s];
        }
        __syncthreads();
    }

    if (tid == 0) out[0] = smem[0];
}

// ------------------------------------------------------------
// [C3-T08] Kernel 2: cooperative_groups::this_thread_block() instead of __syncthreads
// [C3-T09] TODO [REQUIRED-1]
// ------------------------------------------------------------
__global__ void reduce_cg_block(const int* in, int* out)
{
    __shared__ int smem[BLOCK_SIZE];

    // [C3-T10] TODO [REQUIRED-1] obtain the block-level cooperative group
    auto block = cg::this_thread_block();
    int tid = block.thread_rank();

    smem[tid] = (tid < N) ? in[tid] : 0;

    // [C3-T11] TODO [REQUIRED-1] use block.sync() instead of __syncthreads()
    block.sync();

    for (int s = BLOCK_SIZE / 2; s > 0; s >>= 1) {
        if (tid < s) {
            // TODO: smem[tid] += smem[tid + s];
        }
        // [C3-T12] TODO [REQUIRED-1] block.sync() rather than __syncthreads()
        block.sync();
    }

    if (tid == 0) out[0] = 0; // [C3-T13] stub: after the fix, store smem[0]
}

// ------------------------------------------------------------
// [C3-T14] Device function: warp tile reduce (used by the two-level reduce)
// [C3-T15] TODO [REQUIRED-3]
// ------------------------------------------------------------
__device__ int warp_reduce_tile(cg::thread_block_tile<32> tile, int val)
{
    // [C3-T16] TODO [REQUIRED-3] use tile.shfl_down for warp reduce
    for (int offset = 16; offset > 0; offset >>= 1) {
        // val += tile.shfl_down(val, offset); // TODO: uncomment
    }
    return val; // [C3-T17] stub: after the fix, lane 0 holds the correct warp sum
}

// ------------------------------------------------------------
// [C3-T18] Kernel 3: two-level reduce - tiled_partition<32> + this_thread_block
// [C3-T19] TODO [REQUIRED-3]
// ------------------------------------------------------------
__global__ void reduce_cg_tile(const int* in, int* out)
{
    __shared__ int warp_sums[BLOCK_SIZE / 32]; // [C3-T20] each warp writes one result

    auto block = cg::this_thread_block();
    // [C3-T21] TODO [REQUIRED-2] create tiled_partition<32>
    auto warp  = cg::tiled_partition<32>(block);

    int tid    = block.thread_rank();
    int warp_id = tid / 32;
    int lane_id = tid % 32;

    int val = (tid < N) ? in[tid] : 0;

    // [C3-T22] First level: in-warp reduce
    val = warp_reduce_tile(warp, val);

    // [C3-T23] lane 0 writes to shared memory
    if (lane_id == 0) {
        warp_sums[warp_id] = val; // TODO: val is currently a stub (0)
    }
    block.sync();

    // [C3-T24] Second level: warp 0 collects results from all warps
    if (warp_id == 0) {
        val = (lane_id < BLOCK_SIZE / 32) ? warp_sums[lane_id] : 0;
        val = warp_reduce_tile(warp, val);
    }

    if (tid == 0) out[0] = 0; // [C3-T25] stub: after the fix, store val
}

// ------------------------------------------------------------
// [C3-T26] Kernel 4: tiled_partition<16> demo
// [C3-T27] TODO [REQUIRED-5]
// ------------------------------------------------------------
__global__ void demo_tile16(const int* in, int* out)
{
    auto block = cg::this_thread_block();
    // [C3-T28] TODO [REQUIRED-5] create tiled_partition<16>
    auto tile16 = cg::tiled_partition<16>(block);

    int tid    = block.thread_rank();
    int val    = (tid < N) ? in[tid] : 0;

    // [C3-T29] TODO [REQUIRED-5] reduce within tile16 (demo only, no merge to block level)
    for (int offset = 8; offset > 0; offset >>= 1) {
        // val += tile16.shfl_down(val, offset); // TODO
    }

    // [C3-T30] each tile<16>'s rank 0 writes the output
    if (tile16.thread_rank() == 0) {
        // TODO: out[tid / 16] = val;
    }
    if (tid == 0) out[0] = 0; // [C3-T31] stub
}

// ------------------------------------------------------------
// [C3-T32] CPU reference
// ------------------------------------------------------------
static int cpu_sum(const int* data, int n)
{
    int s = 0;
    for (int i = 0; i < n; ++i) s += data[i];
    return s;
}

// ------------------------------------------------------------
// [C3-T33] Main program
// ------------------------------------------------------------
int main()
{
    print_device_info(0);
    NVTX_RANGE("C3/main");

    int h_in[N];
    for (int i = 0; i < N; ++i) h_in[i] = i + 1;
    int cpu_ref = cpu_sum(h_in, N);
    // [C3-T34]
    printf("CPU reference result = %d\n\n", cpu_ref);

    int *d_in = nullptr, *d_out = nullptr;
    CUDA_CHECK(cudaMalloc(&d_in,  N * sizeof(int)));
    CUDA_CHECK(cudaMalloc(&d_out, sizeof(int)));
    CUDA_CHECK(cudaMemcpy(d_in, h_in, N * sizeof(int), cudaMemcpyHostToDevice));

    CudaEventTimer timer;
    int h_result;

    // ------------------------------------------------------------
    // [C3-T35] Test raw-sync baseline
    // ------------------------------------------------------------
    {
        NVTX_RANGE("C3/raw_sync");
        CUDA_CHECK(cudaMemset(d_out, 0, sizeof(int)));
        timer.start();
        reduce_raw_sync<<<1, BLOCK_SIZE>>>(d_in, d_out);
        CUDA_CHECK(cudaGetLastError());
        timer.stop();
        CUDA_CHECK(cudaMemcpy(&h_result, d_out, sizeof(int), cudaMemcpyDeviceToHost));
        // [C3-T36]
        printf("[raw_sync]   result=%d (expected %d)  %.3f ms\n",
               h_result, cpu_ref, timer.elapsed_ms());
    }

    // ------------------------------------------------------------
    // [C3-T37] Test CG block
    // ------------------------------------------------------------
    {
        NVTX_RANGE("C3/cg_block");
        CUDA_CHECK(cudaMemset(d_out, 0, sizeof(int)));
        timer.start();
        reduce_cg_block<<<1, BLOCK_SIZE>>>(d_in, d_out);
        CUDA_CHECK(cudaGetLastError());
        timer.stop();
        CUDA_CHECK(cudaMemcpy(&h_result, d_out, sizeof(int), cudaMemcpyDeviceToHost));
        // [C3-T38]
        printf("[cg_block]   result=%d (expected %d, stub=0)  %.3f ms\n",
               h_result, cpu_ref, timer.elapsed_ms());
    }

    // ------------------------------------------------------------
    // [C3-T39] Test two-level reduce (warp tile)
    // ------------------------------------------------------------
    {
        NVTX_RANGE("C3/cg_tile");
        CUDA_CHECK(cudaMemset(d_out, 0, sizeof(int)));
        timer.start();
        reduce_cg_tile<<<1, BLOCK_SIZE>>>(d_in, d_out);
        CUDA_CHECK(cudaGetLastError());
        timer.stop();
        CUDA_CHECK(cudaMemcpy(&h_result, d_out, sizeof(int), cudaMemcpyDeviceToHost));
        // [C3-T40]
        printf("[cg_tile32]  result=%d (expected %d, stub=0)  %.3f ms\n",
               h_result, cpu_ref, timer.elapsed_ms());
    }

    // ------------------------------------------------------------
    // [C3-T41] Test tile<16>
    // ------------------------------------------------------------
    {
        NVTX_RANGE("C3/tile16");
        CUDA_CHECK(cudaMemset(d_out, 0, sizeof(int)));
        demo_tile16<<<1, BLOCK_SIZE>>>(d_in, d_out);
        CUDA_CHECK(cudaGetLastError());
        CUDA_CHECK(cudaDeviceSynchronize());
        // [C3-T42]
        printf("[tile16]     (demo only, no merge, stub=0)\n");
    }

    // ------------------------------------------------------------
    // [C3-T43] TODO [REQUIRED-6] compare performance of two versions in Nsight Compute
    // [C3-T44] TODO [ADVANCED] implement inclusive/exclusive scan with cooperative_groups
    // [C3-T45] TODO [ADVANCED] use cooperative_groups::partition for dynamic grouping
    // ------------------------------------------------------------

    CUDA_CHECK(cudaFree(d_in));
    CUDA_CHECK(cudaFree(d_out));

    // [C3-T46]
    printf("\n[C3] done. After completing TODOs, both versions should match the CPU reference.\n");
    return 0;
}
