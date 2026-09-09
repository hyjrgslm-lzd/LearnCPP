// I1_softmax_online/main.cu
// [I1-T01] Exercise I1: Online Softmax (Milakov & Gimelshein 2018)
//
// [I1-T02] Goals:
//   - one-pass online softmax: running max + running denominator
//   - warp-level shuffle reduce (__shfl_down_sync)
//   - block-level cross-warp merge (shared memory two-stage reduce)
//   - compare with two-pass safe softmax (precision and performance)
//
// Build: cmake --build build --target I1_softmax_online
// Run:   ./I1_softmax_online

#include "common/cuda_check.cuh"
#include "common/timer.cuh"
#include "common/device_info.cuh"
#include "common/nvtx_range.cuh"

#include <cuda_runtime.h>
#include <cuda_fp16.h>

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cfloat>
#include <cassert>
#include <vector>
#include <random>
#include <algorithm>

// ------------------------------------------------------------
// [I1-T03] Problem size
// ------------------------------------------------------------
constexpr int B          = 32;     // [I1-T04] batch (number of rows)
constexpr int N          = 4096;   // [I1-T05] length per row
constexpr int BLOCK_ROWS = 4;      // [I1-T06] rows per block
constexpr int WARP_SIZE  = 32;

// ------------------------------------------------------------
// [I1-T07] Device helper: warp reduce max
// ------------------------------------------------------------
__device__ __forceinline__ float warp_reduce_max(float val)
{
    // [I1-T08] tree-style max reduce inside a warp via __shfl_down_sync
    for (int offset = 16; offset > 0; offset >>= 1) {
        float other = __shfl_down_sync(0xffffffff, val, offset);
        val = fmaxf(val, other);
    }
    return val;
}

// ------------------------------------------------------------
// [I1-T09] Device helper: warp reduce sum
// ------------------------------------------------------------
__device__ __forceinline__ float warp_reduce_sum(float val)
{
    for (int offset = 16; offset > 0; offset >>= 1) {
        val += __shfl_down_sync(0xffffffff, val, offset);
    }
    return val;
}

// ------------------------------------------------------------
// [I1-T10] Kernel 1: Two-pass safe softmax (baseline / CPU verification)
// [I1-T11] Pass 1: find row max; Pass 2: compute exp(x - max) / sum
// ------------------------------------------------------------
__global__ void softmax_two_pass(const float* __restrict__ in,
                                  float*       __restrict__ out,
                                  int rows, int cols)
{
    // [I1-T12] one warp per row; blockDim.x = WARP_SIZE * BLOCK_ROWS
    int warp_id = (blockIdx.x * blockDim.y + threadIdx.y); // global row index
    if (warp_id >= rows) return;

    int lane    = threadIdx.x; // 0..31
    const float* row_in  = in  + warp_id * cols;
    float*       row_out = out + warp_id * cols;

    // [I1-T13] Pass 1: find max (tile loop, WARP_SIZE elements per step)
    float row_max = -FLT_MAX;
    for (int i = lane; i < cols; i += WARP_SIZE) {
        row_max = fmaxf(row_max, row_in[i]);
    }
    row_max = warp_reduce_max(row_max);
    // [I1-T14] lane 0 broadcasts row_max (already visible to all lanes via shfl)
    row_max = __shfl_sync(0xffffffff, row_max, 0);

    // [I1-T15] Pass 2: compute exp(x - max) and accumulate denominator
    float row_sum = 0.0f;
    for (int i = lane; i < cols; i += WARP_SIZE) {
        row_sum += expf(row_in[i] - row_max);
    }
    row_sum = warp_reduce_sum(row_sum);
    row_sum = __shfl_sync(0xffffffff, row_sum, 0);

    // [I1-T16] write outputs
    for (int i = lane; i < cols; i += WARP_SIZE) {
        row_out[i] = expf(row_in[i] - row_max) / row_sum;
    }
}

// ------------------------------------------------------------
// [I1-T17] Kernel 2: Online softmax (one pass, Milakov 2018)
// [I1-T18] One warp per row, single scan maintains running_max and running_sum
//
// [I1-T19] TODO [REQUIRED] step 2: warp-level online softmax (simplified d <= 32)
// [I1-T20] TODO [REQUIRED] step 3: block-level extension, support arbitrary row length
//          (warp loop + cross-warp merge)
// [I1-T21] TODO [REQUIRED] step 4: block-level reduce: smem combines running_max
//          across multiple warps
// ------------------------------------------------------------
__global__ void softmax_online(const float* __restrict__ in,
                                float*       __restrict__ out,
                                int rows, int cols)
{
    // [I1-T22] one warp per row
    int warp_id = blockIdx.x * blockDim.y + threadIdx.y;
    if (warp_id >= rows) return;

    int lane    = threadIdx.x;
    const float* row_in  = in  + warp_id * cols;
    float*       row_out = out + warp_id * cols;

    // [I1-T23] -- TODO [REQUIRED] step 2/3: one-pass online softmax --
    // [I1-T24] Initialize running state
    // float running_max = -INFINITY;
    // float running_sum = 0.0f;
    //
    // Single-pass scan (WARP_SIZE elements per step):
    // for (int i = lane; i < cols; i += WARP_SIZE) {
    //     float x = row_in[i];
    //     float new_max = fmaxf(running_max, x);
    //     // correction factor: exp(old_max - new_max)
    //     running_sum = running_sum * expf(running_max - new_max) + expf(x - new_max);
    //     running_max = new_max;
    // }
    //
    // [I1-T25] -- TODO [REQUIRED] step 4: warp reduce to merge running_max and running_sum --
    // Note: when merging across lanes, correction factor must be handled correctly
    // running_max_final = warp_reduce_max(running_max);
    // // each lane corrects its own running_sum via exp(lane_max - global_max)
    // running_sum = running_sum * expf(running_max - running_max_final);
    // running_sum_final = warp_reduce_sum(running_sum);
    //
    // [I1-T26] write output (stub: outputs 0; correct after student finishes TODOs)
    for (int i = lane; i < cols; i += WARP_SIZE) {
        row_out[i] = 0.0f; // stub
    }
    // [I1-T27] -- replace with: row_out[i] = expf(row_in[i] - running_max_final) / running_sum_final;
}

// ------------------------------------------------------------
// [I1-T28] TODO [ADVANCED] FP16 input version (FP32 accumulator)
// [I1-T29] __global__ void softmax_online_fp16(...) { ... }
// ------------------------------------------------------------

// ------------------------------------------------------------
// [I1-T30] TODO [ADVANCED] block-sparse mask softmax (some blocks skip exp)
// ------------------------------------------------------------

// ------------------------------------------------------------
// [I1-T31] TODO [ADVANCED] FP8 running max + sum (with rescaling)
// ------------------------------------------------------------

// ------------------------------------------------------------
// [I1-T32] CPU reference: two-pass safe softmax (full implementation, for verification)
// ------------------------------------------------------------
static void cpu_softmax_ref(const float* in, float* out, int rows, int cols)
{
    for (int r = 0; r < rows; ++r) {
        const float* row_in  = in  + r * cols;
        float*       row_out = out + r * cols;

        // [I1-T33] Pass 1: find max
        float row_max = -FLT_MAX;
        for (int c = 0; c < cols; ++c) row_max = std::max(row_max, row_in[c]);

        // [I1-T34] Pass 2: exp(x - max) / sum
        float row_sum = 0.0f;
        for (int c = 0; c < cols; ++c) row_sum += std::exp(row_in[c] - row_max);
        for (int c = 0; c < cols; ++c)
            row_out[c] = std::exp(row_in[c] - row_max) / row_sum;
    }
}

// ------------------------------------------------------------
// [I1-T35] Verification: max abs diff between two arrays
// ------------------------------------------------------------
static float max_abs_diff(const float* a, const float* b, int n)
{
    float diff = 0.0f;
    for (int i = 0; i < n; ++i)
        diff = std::max(diff, std::abs(a[i] - b[i]));
    return diff;
}

// ------------------------------------------------------------
// [I1-T36] main
// ------------------------------------------------------------
int main()
{
    std::puts("[I1_softmax_online]");
    print_device_info(0);
    NVTX_RANGE("I1_softmax_online/main");

    // [I1-T37] -- TODO [REQUIRED] step 1: build row-major matrix, random init --
    // [I1-T38] init already provided so students can observe a correct input distribution
    constexpr int total = B * N;
    std::vector<float> h_in(total), h_ref(total), h_two_pass(total), h_online(total);

    std::mt19937 rng(42);
    // [I1-T39] dynamic range from 1e-3 to 1e3 (uniform on [-6, 6] simulates this)
    std::uniform_real_distribution<float> dist(-6.0f, 6.0f);
    for (auto& v : h_in) v = dist(rng);

    // [I1-T40]
    printf("Problem size: B=%d  N=%d  total_floats=%d\n\n", B, N, total);

    // [I1-T41] CPU reference
    cpu_softmax_ref(h_in.data(), h_ref.data(), B, N);

    // [I1-T42] GPU allocation
    float *d_in = nullptr, *d_out_tp = nullptr, *d_out_ol = nullptr;
    CUDA_CHECK(cudaMalloc(&d_in,     total * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_out_tp, total * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_out_ol, total * sizeof(float)));
    CUDA_CHECK(cudaMemcpy(d_in, h_in.data(), total * sizeof(float), cudaMemcpyHostToDevice));

    // [I1-T43] Launch config: BLOCK_ROWS warps per block, one warp per row
    dim3 block(WARP_SIZE, BLOCK_ROWS);
    dim3 grid((B + BLOCK_ROWS - 1) / BLOCK_ROWS);
    // [I1-T44]
    printf("Launch config: grid=(%d,1,1)  block=(%d,%d,1)  rows/block=%d\n\n",
           grid.x, block.x, block.y, BLOCK_ROWS);

    CudaEventTimer timer;

    // ------------------------------------------------------------
    // [I1-T45] Two-pass softmax (baseline)
    // ------------------------------------------------------------
    {
        NVTX_RANGE("I1/two_pass");
        CUDA_CHECK(cudaMemset(d_out_tp, 0, total * sizeof(float)));
        timer.start();
        softmax_two_pass<<<grid, block>>>(d_in, d_out_tp, B, N);
        CUDA_CHECK(cudaGetLastError());
        timer.stop();
        float ms = timer.elapsed_ms();

        CUDA_CHECK(cudaMemcpy(h_two_pass.data(), d_out_tp,
                              total * sizeof(float), cudaMemcpyDeviceToHost));

        float diff = max_abs_diff(h_ref.data(), h_two_pass.data(), total);
        // [I1-T46] effective bandwidth: read B*N float + write B*N float (2 passes => 2x2xB*N bytes)
        float bw_gbps = 2.f * 2.f * total * sizeof(float) / (ms * 1e-3f) / 1e9f;
        // [I1-T47]
        printf("[two_pass]  %.3f ms  max_err=%.2e  effective_bw=%.1f GB/s\n",
               ms, diff, bw_gbps);
    }

    // ------------------------------------------------------------
    // [I1-T48] Online softmax (to be implemented)
    // ------------------------------------------------------------
    {
        NVTX_RANGE("I1/online");
        CUDA_CHECK(cudaMemset(d_out_ol, 0, total * sizeof(float)));
        timer.start();
        softmax_online<<<grid, block>>>(d_in, d_out_ol, B, N);
        CUDA_CHECK(cudaGetLastError());
        timer.stop();
        float ms = timer.elapsed_ms();

        CUDA_CHECK(cudaMemcpy(h_online.data(), d_out_ol,
                              total * sizeof(float), cudaMemcpyDeviceToHost));

        float diff = max_abs_diff(h_ref.data(), h_online.data(), total);
        // [I1-T49] effective bandwidth: ideally one pass reads/writes once (1x2xB*N bytes)
        float bw_gbps = 1.f * 2.f * total * sizeof(float) / (ms * 1e-3f) / 1e9f;
        // [I1-T50]
        printf("[online]    %.3f ms  max_err=%.2e  effective_bw=%.1f GB/s  "
               "(stub: expect err < 1e-5 once implemented)\n",
               ms, diff, bw_gbps);
    }

    // ------------------------------------------------------------
    // [I1-T51] TODO [REQUIRED] step 5: online vs two-pass diff should be < 1e-5 once implemented
    // [I1-T52] TODO [REQUIRED] step 6: use Nsight Compute to measure block-level reduce throughput
    // ------------------------------------------------------------
    // [I1-T53]
    printf("\nAcceptance: online softmax max_err vs CPU ref < 1.0e-05 (FP32)\n");
    // [I1-T54]
    printf("Current stub outputs all zeros; CPU check will report mismatch (expected).\n");

    CUDA_CHECK(cudaFree(d_in));
    CUDA_CHECK(cudaFree(d_out_tp));
    CUDA_CHECK(cudaFree(d_out_ol));

    // [I1-T55]
    printf("\n[I1] done. Use ncu --set full ./I1_softmax_online to inspect registers and bandwidth.\n");
    return 0;
}
