// D3_warp_reduce_scan/main.cu
// [D3-T01] Exercise D3: Block-level Reduce & Scan
// [D3-T02] Two-tier structure (warp shuffle + smem)
//
// [D3-T03] Learning goals:
// [D3-T04]   - two-tier reduce: warp shuffle locally, smem across warps
// [D3-T05]   - two-tier scan: warp Kogge-Stone + inter-warp offset
// [D3-T06]   - compare full-smem / shuffle+atomic / two-tier hybrid
// [D3-T07]   - Nsight Compute bottleneck classification (memory vs compute)
//
// Build: cmake --build build --target D3_warp_reduce_scan
// Run:   ./D3_warp_reduce_scan

#include <cstdio>
#include <cstdlib>
#include <cuda_runtime.h>

#include "common/cuda_check.cuh"
#include "common/timer.cuh"
#include "common/device_info.cuh"
#include "common/nvtx_range.cuh"

// ------------------------------------------------------------
// [D3-T08] Constants
// ------------------------------------------------------------
constexpr int BLOCK_SIZE = 256;
constexpr int WARP_SIZE  = 32;
constexpr int NUM_WARPS  = BLOCK_SIZE / WARP_SIZE; // 8
constexpr int N          = BLOCK_SIZE;

// ------------------------------------------------------------
// [D3-T09] Device function: warp reduce (core primitive from D1)
// ------------------------------------------------------------
__device__ int warp_reduce(int val)
{
    for (int offset = 16; offset > 0; offset >>= 1) {
        // TODO: val += __shfl_down_sync(0xffffffff, val, offset);
    }
    return val; // stub
}

// ------------------------------------------------------------
// [D3-T10] Device function: warp inclusive scan (Kogge-Stone)
// ------------------------------------------------------------
__device__ int warp_scan_inclusive(int val, int lane_id)
{
    for (int offset = 1; offset < WARP_SIZE; offset <<= 1) {
        // int prev = __shfl_up_sync(0xffffffff, val, offset); // TODO
        // if (lane_id >= offset) val += prev;                  // TODO
    }
    return val; // stub
}

// ------------------------------------------------------------
// [D3-T11] Kernel 1: full shared memory reduce (baseline)
// [D3-T12] TODO [REQUIRED-1 control]
// ------------------------------------------------------------
__global__ void reduce_smem_only(const int* in, int* out)
{
    __shared__ int smem[BLOCK_SIZE];
    int tid = threadIdx.x;
    smem[tid] = (tid < N) ? in[tid] : 0;
    __syncthreads();

    for (int s = BLOCK_SIZE / 2; s > 0; s >>= 1) {
        if (tid < s) smem[tid] += smem[tid + s];
        __syncthreads();
    }
    if (tid == 0) out[0] = smem[0];
}

// ------------------------------------------------------------
// [D3-T13] Kernel 2: two-tier hybrid reduce (warp shuffle + smem second level)
// [D3-T14] TODO [REQUIRED-1]
// ------------------------------------------------------------
__global__ void reduce_two_level(const int* in, int* out)
{
    __shared__ int warp_sums[NUM_WARPS];

    int tid     = threadIdx.x;
    int lane_id = tid % WARP_SIZE;
    int warp_id = tid / WARP_SIZE;

    int val = (tid < N) ? in[tid] : 0;

    // [D3-T15] First tier: warp reduce
    val = warp_reduce(val);

    // [D3-T16] lane 0 writes warp result to smem
    if (lane_id == 0) warp_sums[warp_id] = val; // TODO: stub val
    __syncthreads();

    // [D3-T17] Second tier: warp 0 sums all warp results
    if (warp_id == 0) {
        val = (lane_id < NUM_WARPS) ? warp_sums[lane_id] : 0;
        val = warp_reduce(val);
    }

    if (tid == 0) out[0] = 0; // [D3-T18] stub: replace with val after fix
}

// ------------------------------------------------------------
// [D3-T19] Kernel 3: full shuffle + atomicAdd (atomic across warps)
// [D3-T20] TODO [REQUIRED-3 comparison]
// ------------------------------------------------------------
__global__ void reduce_shfl_atomic(const int* in, int* out)
{
    int tid     = threadIdx.x;
    int lane_id = tid % WARP_SIZE;

    int val = (tid < N) ? in[tid] : 0;
    val = warp_reduce(val);

    if (lane_id == 0) {
        // TODO: atomicAdd(out, val);
    }
    if (tid == 0 && threadIdx.x == 0) out[0] = 0; // [D3-T21] stub (initialization)
}

// ------------------------------------------------------------
// [D3-T22] Kernel 4: two-tier block inclusive scan
// [D3-T23] TODO [REQUIRED-2]
// ------------------------------------------------------------
__global__ void scan_two_level(const int* in, int* out_inc, int* out_exc)
{
    __shared__ int warp_totals[NUM_WARPS]; // [D3-T24] inclusive scan tail of each warp
    __shared__ int warp_offsets[NUM_WARPS]; // [D3-T25] inter-warp prefix offset

    int tid     = threadIdx.x;
    int lane_id = tid % WARP_SIZE;
    int warp_id = tid / WARP_SIZE;

    int val = (tid < N) ? in[tid] : 0;

    // [D3-T26] First tier: Kogge-Stone inclusive scan within warp
    int inc = warp_scan_inclusive(val, lane_id);

    // [D3-T27] lane 31 holds total of this warp
    if (lane_id == WARP_SIZE - 1) warp_totals[warp_id] = inc;
    __syncthreads();

    // [D3-T28] warp 0 scans warp_totals to obtain inter-warp offset
    if (warp_id == 0) {
        int w_val = (lane_id < NUM_WARPS) ? warp_totals[lane_id] : 0;
        int w_inc = warp_scan_inclusive(w_val, lane_id);
        // TODO: warp_offsets[lane_id] = w_inc - w_val; // exclusive offset
        if (lane_id < NUM_WARPS) warp_offsets[lane_id] = 0; // stub
    }
    __syncthreads();

    // [D3-T29] add inter-warp offset
    int offset = (warp_id > 0) ? warp_offsets[warp_id - 1] : 0;
    int inclusive = inc + offset; // [D3-T30] TODO: correct after fix (stub = inc + 0)

    if (tid < N) {
        out_inc[tid] = 0; // [D3-T31] stub: replace with inclusive after fix
        out_exc[tid] = 0; // [D3-T32] stub: replace with inclusive - val after fix
    }
}

// ------------------------------------------------------------
// [D3-T33] CPU reference
// ------------------------------------------------------------
static int cpu_reduce_ref(const int* data, int n)
{
    int s = 0;
    for (int i = 0; i < n; ++i) s += data[i];
    return s;
}

static void cpu_scan_ref(const int* data, int n, int* inc_out, int* exc_out)
{
    inc_out[0] = data[0];
    exc_out[0] = 0;
    for (int i = 1; i < n; ++i) {
        inc_out[i] = inc_out[i-1] + data[i];
        exc_out[i] = inc_out[i-1];
    }
}

// ------------------------------------------------------------
// [D3-T34] Main program
// ------------------------------------------------------------
int main()
{
    print_device_info(0);
    NVTX_RANGE("D3/main");

    int h_in[N];
    for (int i = 0; i < N; ++i) h_in[i] = i + 1;

    int cpu_total = cpu_reduce_ref(h_in, N);
    int cpu_inc[N], cpu_exc[N];
    cpu_scan_ref(h_in, N, cpu_inc, cpu_exc);
    printf("CPU total=%d  inc[255]=%d  exc[1]=%d\n\n",
           cpu_total, cpu_inc[N-1], cpu_exc[1]);

    int *d_in = nullptr, *d_out = nullptr, *d_out2 = nullptr;
    CUDA_CHECK(cudaMalloc(&d_in,   N * sizeof(int)));
    CUDA_CHECK(cudaMalloc(&d_out,  N * sizeof(int)));
    CUDA_CHECK(cudaMalloc(&d_out2, N * sizeof(int)));
    CUDA_CHECK(cudaMemcpy(d_in, h_in, N * sizeof(int), cudaMemcpyHostToDevice));

    CudaEventTimer timer;
    int h_result;

    // ------------------------------------------------------------
    // [D3-T35] Reduce comparison
    // ------------------------------------------------------------
    {
        NVTX_RANGE("D3/reduce_smem");
        CUDA_CHECK(cudaMemset(d_out, 0, sizeof(int)));
        timer.start();
        reduce_smem_only<<<1, BLOCK_SIZE>>>(d_in, d_out);
        CUDA_CHECK(cudaGetLastError());
        timer.stop();
        CUDA_CHECK(cudaMemcpy(&h_result, d_out, sizeof(int), cudaMemcpyDeviceToHost));
        // [D3-T36]
        printf("[reduce_smem]     %d (expected %d)  %.3f ms\n",
               h_result, cpu_total, timer.elapsed_ms());
    }
    {
        NVTX_RANGE("D3/reduce_two_level");
        CUDA_CHECK(cudaMemset(d_out, 0, sizeof(int)));
        timer.start();
        reduce_two_level<<<1, BLOCK_SIZE>>>(d_in, d_out);
        CUDA_CHECK(cudaGetLastError());
        timer.stop();
        CUDA_CHECK(cudaMemcpy(&h_result, d_out, sizeof(int), cudaMemcpyDeviceToHost));
        // [D3-T37]
        printf("[reduce_2level]   %d (expected %d, stub=0)  %.3f ms\n",
               h_result, cpu_total, timer.elapsed_ms());
    }
    {
        NVTX_RANGE("D3/reduce_atomic");
        CUDA_CHECK(cudaMemset(d_out, 0, sizeof(int)));
        timer.start();
        reduce_shfl_atomic<<<1, BLOCK_SIZE>>>(d_in, d_out);
        CUDA_CHECK(cudaGetLastError());
        timer.stop();
        CUDA_CHECK(cudaMemcpy(&h_result, d_out, sizeof(int), cudaMemcpyDeviceToHost));
        // [D3-T38]
        printf("[reduce_atomic]   %d (expected %d, stub=0)  %.3f ms\n",
               h_result, cpu_total, timer.elapsed_ms());
    }

    // ------------------------------------------------------------
    // [D3-T39] Scan test
    // ------------------------------------------------------------
    {
        NVTX_RANGE("D3/scan_two_level");
        CUDA_CHECK(cudaMemset(d_out,  0, N * sizeof(int)));
        CUDA_CHECK(cudaMemset(d_out2, 0, N * sizeof(int)));
        timer.start();
        scan_two_level<<<1, BLOCK_SIZE>>>(d_in, d_out, d_out2);
        CUDA_CHECK(cudaGetLastError());
        timer.stop();
        int h_inc[N], h_exc[N];
        CUDA_CHECK(cudaMemcpy(h_inc, d_out,  N * sizeof(int), cudaMemcpyDeviceToHost));
        CUDA_CHECK(cudaMemcpy(h_exc, d_out2, N * sizeof(int), cudaMemcpyDeviceToHost));
        // [D3-T40]
        printf("[scan_2level] inc[255]=%d (expected %d, stub=0)  exc[1]=%d (expected %d)  %.3f ms\n",
               h_inc[N-1], cpu_inc[N-1], h_exc[1], cpu_exc[1], timer.elapsed_ms());
    }

    // ------------------------------------------------------------
    // [D3-T41] TODO [REQUIRED-5] throughput comparison across blockDim (128/256/512)
    // [D3-T42] TODO [REQUIRED-6] Nsight Compute bottleneck classification
    // [D3-T43] TODO [ADVANCED] block-level histogram (multi-bin ballot)
    // [D3-T44] TODO [ADVANCED] split-K reduce (multi-block + global atomic)
    // ------------------------------------------------------------

    CUDA_CHECK(cudaFree(d_in));
    CUDA_CHECK(cudaFree(d_out));
    CUDA_CHECK(cudaFree(d_out2));

    // [D3-T45]
    printf("\n[D3] done. Use Nsight Compute --set full to view bottleneck classification.\n");
    return 0;
}
