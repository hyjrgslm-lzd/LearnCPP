// D1_warp_shuffle_basics/main.cu
// [D1-T01] Exercise D1: Warp Shuffle - five primitives
// [D1-T02] broadcast / butterfly / shift / reduce / scan
//
// [D1-T03] Learning goals:
// [D1-T04]   - __shfl_sync: broadcast (lane 0 broadcasts to whole warp)
// [D1-T05]   - __shfl_xor_sync: butterfly exchange
// [D1-T06]   - __shfl_up_sync: shift toward higher lanes (Kogge-Stone scan)
// [D1-T07]   - __shfl_down_sync: shift toward lower lanes (warp reduce)
// [D1-T08]   - implement warp-32 reduce and inclusive scan
//
// Build: cmake --build build --target D1_warp_shuffle_basics
// Run:   ./D1_warp_shuffle_basics

#include <cstdio>
#include <cstdlib>
#include <cuda_runtime.h>

#include "common/cuda_check.cuh"
#include "common/timer.cuh"
#include "common/device_info.cuh"
#include "common/nvtx_range.cuh"

// ------------------------------------------------------------
// [D1-T09] Constants
// ------------------------------------------------------------
constexpr int WARP_SIZE  = 32;
constexpr int BLOCK_SIZE = WARP_SIZE; // [D1-T10] single-warp demo

// ------------------------------------------------------------
// [D1-T11] Kernel 1: warp broadcast - lane 0 broadcasts to all lanes
// [D1-T12] TODO [REQUIRED-1]
// ------------------------------------------------------------
__global__ void warp_broadcast_kernel(const int* in, int* out)
{
    int lane_id = threadIdx.x % WARP_SIZE;
    int val     = in[lane_id];

    // [D1-T13] TODO [REQUIRED-1] use __shfl_sync to broadcast lane 0 value to all lanes
    // int broadcast_val = __shfl_sync(0xffffffff, val, 0);
    // out[lane_id] = broadcast_val;

    out[lane_id] = 0; // [D1-T14] stub: after fix all lanes should write in[0]
}

// ------------------------------------------------------------
// [D1-T15] Kernel 2: butterfly exchange - lane i swaps with lane (i^1)
// [D1-T16] TODO [REQUIRED-2]
// ------------------------------------------------------------
__global__ void warp_butterfly_kernel(const int* in, int* out)
{
    int lane_id = threadIdx.x % WARP_SIZE;
    int val     = in[lane_id];

    // [D1-T17] TODO [REQUIRED-2] __shfl_xor_sync(mask, val, 1) swaps neighbor lanes
    // int partner_val = __shfl_xor_sync(0xffffffff, val, 1);
    // out[lane_id] = partner_val;

    out[lane_id] = 0; // [D1-T18] stub: after fix lane i writes in[i^1]
}

// ------------------------------------------------------------
// [D1-T19] Kernel 3: shift - each lane reads value of lane offset=1 above
// [D1-T20] TODO [REQUIRED-3]
// ------------------------------------------------------------
__global__ void warp_shift_kernel(const int* in, int* out)
{
    int lane_id = threadIdx.x % WARP_SIZE;
    int val     = in[lane_id];

    // [D1-T21] TODO [REQUIRED-3] __shfl_down_sync: each lane reads value of lane+1
    // int shifted = __shfl_down_sync(0xffffffff, val, 1);
    // out[lane_id] = (lane_id < WARP_SIZE - 1) ? shifted : val; // [D1-T22] lane 31 keeps own value

    out[lane_id] = 0; // stub
}

// ------------------------------------------------------------
// [D1-T23] Device function: warp reduce (sum)
// [D1-T24] TODO [REQUIRED-4]
// ------------------------------------------------------------
__device__ int warp_reduce_sum(int val)
{
    // [D1-T25] TODO [REQUIRED-4] log2(32) = 5 rounds, halve offset each round
    for (int offset = 16; offset > 0; offset >>= 1) {
        // val += __shfl_down_sync(0xffffffff, val, offset); // TODO
    }
    return val; // [D1-T26] stub: lane 0 should hold the warp-wide sum
}

// ------------------------------------------------------------
// [D1-T27] Kernel 4: warp reduce kernel
// [D1-T28] TODO [REQUIRED-4]
// ------------------------------------------------------------
__global__ void warp_reduce_kernel(const int* in, int* out)
{
    int lane_id = threadIdx.x % WARP_SIZE;
    int val     = in[lane_id];

    val = warp_reduce_sum(val);

    if (lane_id == 0) out[0] = 0; // [D1-T29] stub: replace with val after fix
}

// ------------------------------------------------------------
// [D1-T30] Device function: Kogge-Stone inclusive scan
// [D1-T31] TODO [REQUIRED-5]
// ------------------------------------------------------------
__device__ int warp_scan_inclusive(int val)
{
    int lane_id = threadIdx.x % WARP_SIZE;

    // [D1-T32] Kogge-Stone: log2(32) = 5 rounds
    for (int offset = 1; offset < WARP_SIZE; offset <<= 1) {
        // int prev = __shfl_up_sync(0xffffffff, val, offset); // TODO
        // if (lane_id >= offset) val += prev;                  // TODO
    }
    return val; // [D1-T33] stub: after fix lane i holds prefix sum of in[0..i]
}

// ------------------------------------------------------------
// [D1-T34] Kernel 5: warp inclusive scan kernel
// [D1-T35] TODO [REQUIRED-5]
// ------------------------------------------------------------
__global__ void warp_scan_kernel(const int* in, int* out_inclusive, int* out_exclusive)
{
    int lane_id = threadIdx.x % WARP_SIZE;
    int val     = in[lane_id];

    int inc = warp_scan_inclusive(val);

    out_inclusive[lane_id] = 0; // [D1-T36] stub: replace with inc after fix
    // [D1-T37] exclusive = inclusive - own value
    // TODO: out_exclusive[lane_id] = inc - val;
    out_exclusive[lane_id] = 0; // stub
}

// ------------------------------------------------------------
// [D1-T38] Kernel 6: smem-based reduce (for comparison)
// [D1-T39] TODO [REQUIRED-6]
// ------------------------------------------------------------
__global__ void smem_reduce_kernel(const int* in, int* out)
{
    __shared__ int smem[WARP_SIZE];
    int tid = threadIdx.x;
    smem[tid] = in[tid];
    __syncthreads();

    for (int s = WARP_SIZE / 2; s > 0; s >>= 1) {
        if (tid < s) smem[tid] += smem[tid + s];
        __syncthreads();
    }
    if (tid == 0) out[0] = smem[0];
}

// ------------------------------------------------------------
// [D1-T40] CPU reference
// ------------------------------------------------------------
static void cpu_reference(const int* in, int* prefix_out, int& total)
{
    total = 0;
    prefix_out[0] = in[0];
    for (int i = 1; i < WARP_SIZE; ++i) {
        prefix_out[i] = prefix_out[i-1] + in[i];
        total += in[i-1];
    }
    total += in[WARP_SIZE-1];
}

// ------------------------------------------------------------
// [D1-T41] Main program
// ------------------------------------------------------------
int main()
{
    print_device_info(0);
    NVTX_RANGE("D1/main");

    // [D1-T42] input: lane ID + 1 (1..32)
    int h_in[WARP_SIZE];
    for (int i = 0; i < WARP_SIZE; ++i) h_in[i] = i + 1;

    int cpu_prefix[WARP_SIZE], cpu_total;
    cpu_reference(h_in, cpu_prefix, cpu_total);
    printf("CPU total=%d  cpu_prefix[31]=%d\n\n", cpu_total, cpu_prefix[31]);

    int *d_in = nullptr, *d_out = nullptr, *d_out2 = nullptr;
    CUDA_CHECK(cudaMalloc(&d_in,   WARP_SIZE * sizeof(int)));
    CUDA_CHECK(cudaMalloc(&d_out,  WARP_SIZE * sizeof(int)));
    CUDA_CHECK(cudaMalloc(&d_out2, WARP_SIZE * sizeof(int)));
    CUDA_CHECK(cudaMemcpy(d_in, h_in, WARP_SIZE * sizeof(int), cudaMemcpyHostToDevice));

    CudaEventTimer timer;
    int h_out[WARP_SIZE], h_out2[WARP_SIZE];

    // ------------------------------------------------------------
    // [D1-T43] Test 1: broadcast
    // ------------------------------------------------------------
    {
        NVTX_RANGE("D1/broadcast");
        CUDA_CHECK(cudaMemset(d_out, 0, WARP_SIZE * sizeof(int)));
        warp_broadcast_kernel<<<1, BLOCK_SIZE>>>(d_in, d_out);
        CUDA_CHECK(cudaGetLastError());
        CUDA_CHECK(cudaDeviceSynchronize());
        CUDA_CHECK(cudaMemcpy(h_out, d_out, WARP_SIZE * sizeof(int), cudaMemcpyDeviceToHost));
        // [D1-T44]
        printf("[broadcast] out[0]=%d out[31]=%d  (expected both=%d, stub=0)\n",
               h_out[0], h_out[31], h_in[0]);
    }

    // ------------------------------------------------------------
    // [D1-T45] Test 2: butterfly
    // ------------------------------------------------------------
    {
        NVTX_RANGE("D1/butterfly");
        CUDA_CHECK(cudaMemset(d_out, 0, WARP_SIZE * sizeof(int)));
        warp_butterfly_kernel<<<1, BLOCK_SIZE>>>(d_in, d_out);
        CUDA_CHECK(cudaGetLastError());
        CUDA_CHECK(cudaDeviceSynchronize());
        CUDA_CHECK(cudaMemcpy(h_out, d_out, WARP_SIZE * sizeof(int), cudaMemcpyDeviceToHost));
        // [D1-T46]
        printf("[butterfly] out[0]=%d out[1]=%d  (expected %d/%d, stub=0)\n",
               h_out[0], h_out[1], h_in[1], h_in[0]);
    }

    // ------------------------------------------------------------
    // [D1-T47] Test 3: shift
    // ------------------------------------------------------------
    {
        NVTX_RANGE("D1/shift");
        CUDA_CHECK(cudaMemset(d_out, 0, WARP_SIZE * sizeof(int)));
        warp_shift_kernel<<<1, BLOCK_SIZE>>>(d_in, d_out);
        CUDA_CHECK(cudaGetLastError());
        CUDA_CHECK(cudaDeviceSynchronize());
        CUDA_CHECK(cudaMemcpy(h_out, d_out, WARP_SIZE * sizeof(int), cudaMemcpyDeviceToHost));
        // [D1-T48]
        printf("[shift]     out[0]=%d  (expected %d, stub=0)\n", h_out[0], h_in[1]);
    }

    // ------------------------------------------------------------
    // [D1-T49] Test 4: warp reduce (shuffle) vs smem reduce
    // ------------------------------------------------------------
    {
        NVTX_RANGE("D1/reduce");
        CUDA_CHECK(cudaMemset(d_out, 0, sizeof(int)));
        timer.start();
        warp_reduce_kernel<<<1, BLOCK_SIZE>>>(d_in, d_out);
        CUDA_CHECK(cudaGetLastError());
        timer.stop();
        CUDA_CHECK(cudaMemcpy(h_out, d_out, sizeof(int), cudaMemcpyDeviceToHost));
        float shfl_ms = timer.elapsed_ms();

        CUDA_CHECK(cudaMemset(d_out2, 0, sizeof(int)));
        timer.start();
        smem_reduce_kernel<<<1, BLOCK_SIZE>>>(d_in, d_out2);
        CUDA_CHECK(cudaGetLastError());
        timer.stop();
        CUDA_CHECK(cudaMemcpy(h_out2, d_out2, sizeof(int), cudaMemcpyDeviceToHost));
        float smem_ms = timer.elapsed_ms();

        // [D1-T50]
        printf("[reduce]  shuffle=%d (expected %d, stub=0) %.3f ms\n",
               h_out[0], cpu_total, shfl_ms);
        // [D1-T51]
        printf("[reduce]  smem   =%d (expected %d)         %.3f ms\n",
               h_out2[0], cpu_total, smem_ms);
    }

    // ------------------------------------------------------------
    // [D1-T52] Test 5: Kogge-Stone inclusive scan
    // ------------------------------------------------------------
    {
        NVTX_RANGE("D1/scan");
        CUDA_CHECK(cudaMemset(d_out,  0, WARP_SIZE * sizeof(int)));
        CUDA_CHECK(cudaMemset(d_out2, 0, WARP_SIZE * sizeof(int)));
        warp_scan_kernel<<<1, BLOCK_SIZE>>>(d_in, d_out, d_out2);
        CUDA_CHECK(cudaGetLastError());
        CUDA_CHECK(cudaDeviceSynchronize());
        CUDA_CHECK(cudaMemcpy(h_out,  d_out,  WARP_SIZE * sizeof(int), cudaMemcpyDeviceToHost));
        CUDA_CHECK(cudaMemcpy(h_out2, d_out2, WARP_SIZE * sizeof(int), cudaMemcpyDeviceToHost));
        // [D1-T53]
        printf("[scan] inclusive[31]=%d (expected %d, stub=0)\n",
               h_out[31], cpu_prefix[31]);
        // [D1-T54]
        printf("[scan] exclusive[1] =%d (expected %d, stub=0)\n",
               h_out2[1], h_in[0]);
    }

    // ------------------------------------------------------------
    // [D1-T55] TODO [ADVANCED] Brent-Kung scan implementation
    // [D1-T56] TODO [ADVANCED] warp-16 partial reduce (mask=0x0000ffff)
    // ------------------------------------------------------------

    CUDA_CHECK(cudaFree(d_in));
    CUDA_CHECK(cudaFree(d_out));
    CUDA_CHECK(cudaFree(d_out2));

    // [D1-T57]
    printf("\n[D1] done. Use Nsight Compute to compare shuffle vs smem latency.\n");
    return 0;
}
