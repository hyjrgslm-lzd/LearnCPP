// H2_gemm_warp_tile_and_mma/main.cu
// ============================================================
// [H2-T01] Exercise H2: Warp Tile + mma.sync m16n8k16 Tensor Core GEMM
//
// [H2-T02] Goal:
//   On top of the H1 shared-memory tiled version, introduce a warp tile
//   (each warp owns a 16x16 sub-tile of C). Load fragments from smem
//   via ldmatrix, run mma.sync.aligned.m16n8k16 on the Tensor Cores,
//   and compare TFLOPS against H1.
//
// [H2-T03] Build requirements: sm_80+, CUDA 13.x, C++20 device / C++26 host
// ============================================================

#include "common/cuda_check.cuh"
#include "common/device_info.cuh"
#include "common/nvtx_range.cuh"
#include "common/timer.cuh"

#include <cuda_runtime.h>
#include <cuda_fp16.h>

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <algorithm>

// ------------------------------------------------------------
// [H2-T04] Constants and hyperparameters
// ------------------------------------------------------------
// [H2-T05] block tile: each block owns BMxBN of C
static constexpr int BM           = 32;   // [H2-T06] block M
static constexpr int BN           = 32;   // [H2-T07] block N
static constexpr int BK           = 16;   // [H2-T08] block K (K width loaded from global per step)
// [H2-T09] warp tile: each warp owns WMxWN (aligned with mma shape)
static constexpr int WM           = 16;   // [H2-T10] warp M (mma m dimension)
static constexpr int WN           = 16;   // [H2-T11] warp N (two mmas with n=8 combine to 16)
static constexpr int WARMUP_ITERS = 3;
static constexpr int BENCH_ITERS  = 10;

// ------------------------------------------------------------
// [H2-T12] Version 1 (baseline): tiled GEMM inherited from H1 (FP32 ALU).
// ------------------------------------------------------------
__global__ void gemm_tiled_fp32(
    const float* __restrict__ A,
    const float* __restrict__ B,
    float*       __restrict__ C,
    int M, int N, int K)
{
    __shared__ float sA[BM][BK];
    __shared__ float sB[BK][BN];

    int row = blockIdx.y * BM + threadIdx.y;
    int col = blockIdx.x * BN + threadIdx.x;
    float acc = 0.0f;

    // [H2-T13] TODO [REQUIRED] step 1: copy the H1 tiled GEMM logic as the FP32 baseline.
    //   outer loop over tile_k, load sA/sB, __syncthreads, inner accumulate, __syncthreads.

    (void)A; (void)B; (void)sA; (void)sB; (void)K; (void)acc;

    if (row < M && col < N)
        C[row * N + col] = 0.0f; // [H2-T14] stub
}

// ------------------------------------------------------------
// [H2-T15] Version 2: Warp Tile + mma.sync (FP16 inputs, FP32 accumulator).
//
// block = (32, 8) = 2 warps (each owning a different warp tile of C).
// Each warp processes a WMxWN = 16x16 C tile:
//   - issues two mma.sync.aligned.m16n8k16 (n=8 x 2 = n=16)
//   - A fragment shape [m16][k16], 8 FP16 per thread
//   - B fragment shape [k16][n8], 4 FP16 per thread
//   - C/D fragment shape [m16][n8], 4 FP32 per thread
// ------------------------------------------------------------
__global__ void gemm_warp_mma(
    const __half* __restrict__ A,   // [H2-T16] [M x K] row-major FP16
    const __half* __restrict__ B,   // [H2-T17] [K x N] column-major FP16 (mind the layout!)
    float*        __restrict__ C,   // [H2-T18] [M x N] row-major FP32
    int M, int N, int K)
{
    // [H2-T19] TODO [REQUIRED] step 1: declare smem tiles (FP16, ldmatrix-aligned).
    //   __shared__ __half sA[BM][BK];   // row-major
    //   __shared__ __half sB[BK][BN];   // row-major (ldmatrix on B wants col-major; use a transposed smem)
    __shared__ __half sA[BM][BK];
    __shared__ __half sB[BK][BN];

    // [H2-T20] TODO [REQUIRED] step 2: derive warp id and lane id within the warp.
    //   int warp_id = threadIdx.y;  // 0 or 1 (two warps)
    //   int lane    = threadIdx.x;  // 0-31

    int warp_id = threadIdx.y;
    int lane    = threadIdx.x;

    // [H2-T21] TODO [REQUIRED] step 2 (cont.): row/col origin of each warp's C tile.
    //   int warp_row = blockIdx.y * BM + warp_id * WM;
    //   int warp_col = blockIdx.x * BN;   // both warps share the same column range, different rows

    int warp_row = blockIdx.y * BM + warp_id * WM;
    int warp_col = blockIdx.x * BN;

    // [H2-T22] TODO [REQUIRED] step 3: declare C/D accumulator fragments (FP32, m16n8 x 2).
    //   uint32_t fragC[2][2] = {};  // [two mmas][2 FP32 regs per thread]
    uint32_t fragC[2][2] = {};  // [H2-T23] stub declaration; real shape is 4 FP32/thread/mma

    // [H2-T24] TODO [REQUIRED] step 4: outer K-tile loop.
    //   for (int tile_k = 0; tile_k < K / BK; ++tile_k) {
    //     // 4a. cooperatively load A/B tiles into sA/sB (mind ldmatrix alignment)
    //     __syncthreads();
    //
    //     // 4b. ldmatrix the A fragment from sA (8 FP16 per thread)
    //     //   uint32_t fragA[4];  // m16k16: 128 bits / thread
    //     //   asm volatile("ldmatrix.sync.aligned.x4.m8n8.shared.b16 {%0,%1,%2,%3}, [%4];"
    //     //                : "=r"(fragA[0]), "=r"(fragA[1]), "=r"(fragA[2]), "=r"(fragA[3])
    //     //                : "r"(smem_ptr_A));
    //
    //     // 4c. ldmatrix the B fragment from sB (4 FP16 per thread)
    //     //   uint32_t fragB[2];  // k16n8: 64 bits / thread
    //
    //     // 4d. first mma.sync (left half, n=8):
    //     //   asm volatile("mma.sync.aligned.m16n8k16.row.col.f32.f16.f16.f32
    //     //                 {%0,%1,%2,%3}, {%4,%5,%6,%7}, {%8,%9}, {%0,%1,%2,%3};"
    //     //                : "+r"(fragC[0][0]), "+r"(fragC[0][1]),
    //     //                  "+r"(fragC[0][2]), "+r"(fragC[0][3])
    //     //                : "r"(fragA[0]), "r"(fragA[1]), "r"(fragA[2]), "r"(fragA[3]),
    //     //                  "r"(fragB[0]), "r"(fragB[1]));
    //
    //     // 4e. second mma.sync (right half, n=8, fragB+offset)
    //
    //     __syncthreads();
    //   }

    // [H2-T25] TODO [REQUIRED] step 5: store fragments back to global C
    //          (need to recompute each thread's row/column).

    (void)A; (void)B; (void)sA; (void)sB; (void)K;
    (void)warp_id; (void)lane; (void)warp_row; (void)warp_col; (void)fragC;

    // [H2-T26] stub: zero output
    int row = blockIdx.y * BM + threadIdx.y * (BM / 2) + lane / 4;
    int col = blockIdx.x * BN + (lane % 4) * 2;
    if (row < M && col < N) C[row * N + col] = 0.0f;
}

// ------------------------------------------------------------
// [H2-T27] CPU reference.
// ------------------------------------------------------------
static void gemm_cpu_ref(
    const float* A, const float* B, float* C,
    int M, int N, int K)
{
    for (int m = 0; m < M; ++m)
        for (int n = 0; n < N; ++n) {
            float acc = 0.0f;
            for (int k = 0; k < K; ++k)
                acc += A[m * K + k] * B[k * N + n];
            C[m * N + n] = acc;
        }
}

static bool check_result(
    const float* ref, const float* got,
    int M, int N, const char* label)
{
    int mismatch = 0;
    float max_err = 0.0f;
    for (int i = 0; i < M * N; ++i) {
        float err = std::fabs(ref[i] - got[i]);
        float rel = err / (std::fabs(ref[i]) + 1e-6f);
        max_err   = std::max(max_err, rel);
        if (rel > 1e-2f) ++mismatch; // [H2-T28] FP16 accumulation: looser tolerance
    }
    bool ok = (mismatch == 0);
    // [H2-T29]
    std::fprintf(stdout, "  [%s] %s  max_rel_err %.2e  mismatch %d/%d\n",
                 label, ok ? "PASS" : "FAIL", (double)max_err, mismatch, M * N);
    return ok;
}

static double calc_tflops(int M, int N, int K, float ms) {
    return 2.0 * M * N * K / (ms * 1e-3) / 1e12;
}

// ------------------------------------------------------------
// [H2-T30] main
// ------------------------------------------------------------
int main()
{
    std::puts("[H2_gemm_warp_tile_and_mma]");
    print_device_info();

    NVTX_RANGE("H2_gemm_warp_tile_and_mma/main");

    const int M = 1024, N = 1024, K = 1024;
    // [H2-T31]
    std::fprintf(stdout, "Problem size: M=%d  N=%d  K=%d\n\n", M, N, K);

    // [H2-T32] -- host memory --
    std::vector<float>  hA_f32(M * K), hB_f32(K * N), hC_ref(M * N, 0.0f);
    std::vector<float>  hC_tiled(M * N, 0.0f), hC_mma(M * N, 0.0f);
    std::vector<__half> hA_f16(M * K), hB_f16(K * N);

    std::srand(42);
    for (int i = 0; i < M * K; ++i) {
        hA_f32[i] = static_cast<float>(std::rand()) / RAND_MAX - 0.5f;
        hA_f16[i] = __float2half(hA_f32[i]);
    }
    for (int i = 0; i < K * N; ++i) {
        hB_f32[i] = static_cast<float>(std::rand()) / RAND_MAX - 0.5f;
        hB_f16[i] = __float2half(hB_f32[i]);
    }

    // [H2-T33]
    std::puts("Computing CPU reference (FP32)...");
    gemm_cpu_ref(hA_f32.data(), hB_f32.data(), hC_ref.data(), M, N, K);

    // [H2-T34] -- device memory --
    float  *dA_f32, *dB_f32, *dC;
    __half *dA_f16, *dB_f16;
    CUDA_CHECK(cudaMalloc(&dA_f32, sizeof(float)  * M * K));
    CUDA_CHECK(cudaMalloc(&dB_f32, sizeof(float)  * K * N));
    CUDA_CHECK(cudaMalloc(&dA_f16, sizeof(__half) * M * K));
    CUDA_CHECK(cudaMalloc(&dB_f16, sizeof(__half) * K * N));
    CUDA_CHECK(cudaMalloc(&dC,     sizeof(float)  * M * N));

    CUDA_CHECK(cudaMemcpy(dA_f32, hA_f32.data(), sizeof(float)  * M * K, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(dB_f32, hB_f32.data(), sizeof(float)  * K * N, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(dA_f16, hA_f16.data(), sizeof(__half) * M * K, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(dB_f16, hB_f16.data(), sizeof(__half) * K * N, cudaMemcpyHostToDevice));

    // [H2-T35] -- launch config --
    dim3 block_tiled(BN, BM);   // [H2-T36] (32, 32)
    dim3 block_mma(32, 2);      // [H2-T37] 2 warps per block, 32 threads per warp
    dim3 grid_tiled((N + BN - 1) / BN, (M + BM - 1) / BM);
    dim3 grid_mma  ((N + BN - 1) / BN, (M + BM - 1) / BM);

    std::fprintf(stdout, "Tiled(FP32) grid=(%u,%u) block=(%u,%u)\n",
                 grid_tiled.x, grid_tiled.y, block_tiled.x, block_tiled.y);
    std::fprintf(stdout, "MMA(FP16)   grid=(%u,%u) block=(%u,%u)\n\n",
                 grid_mma.x,   grid_mma.y,   block_mma.x,   block_mma.y);

    CudaEventTimer timer;

    // ------------------------------------------------------------
    // [H2-T38] TODO [REQUIRED] step 1 & 4: measure the FP32 tiled baseline.
    // ------------------------------------------------------------
    for (int i = 0; i < WARMUP_ITERS; ++i)
        gemm_tiled_fp32<<<grid_tiled, block_tiled>>>(dA_f32, dB_f32, dC, M, N, K);
    CUDA_CHECK(cudaDeviceSynchronize());

    timer.start();
    for (int i = 0; i < BENCH_ITERS; ++i)
        gemm_tiled_fp32<<<grid_tiled, block_tiled>>>(dA_f32, dB_f32, dC, M, N, K);
    timer.stop();
    CUDA_CHECK_LAST();
    float ms_tiled = timer.elapsed_ms() / BENCH_ITERS;
    CUDA_CHECK(cudaMemcpy(hC_tiled.data(), dC, sizeof(float) * M * N, cudaMemcpyDeviceToHost));

    // ------------------------------------------------------------
    // [H2-T39] TODO [REQUIRED] step 5: measure the warp MMA version.
    // ------------------------------------------------------------
    for (int i = 0; i < WARMUP_ITERS; ++i)
        gemm_warp_mma<<<grid_mma, block_mma>>>(dA_f16, dB_f16, dC, M, N, K);
    CUDA_CHECK(cudaDeviceSynchronize());

    timer.start();
    for (int i = 0; i < BENCH_ITERS; ++i)
        gemm_warp_mma<<<grid_mma, block_mma>>>(dA_f16, dB_f16, dC, M, N, K);
    timer.stop();
    CUDA_CHECK_LAST();
    float ms_mma = timer.elapsed_ms() / BENCH_ITERS;
    CUDA_CHECK(cudaMemcpy(hC_mma.data(), dC, sizeof(float) * M * N, cudaMemcpyDeviceToHost));

    // ------------------------------------------------------------
    // [H2-T40] Correctness check.
    // ------------------------------------------------------------
    // [H2-T41]
    std::puts("\n-- Correctness check (FAIL expected at stub stage) --");
    check_result(hC_ref.data(), hC_tiled.data(), M, N, "tiled_fp32");
    check_result(hC_ref.data(), hC_mma.data(),   M, N, "warp_mma");

    // ------------------------------------------------------------
    // [H2-T42] Performance summary.
    // ------------------------------------------------------------
    double tflops_tiled = calc_tflops(M, N, K, ms_tiled);
    double tflops_mma   = calc_tflops(M, N, K, ms_mma);
    double peak_tflops  = 1.5; // [H2-T43] FP32 placeholder

    // [H2-T44]
    std::puts("\n-- Performance summary --");
    std::fprintf(stdout, "%-20s | %8s | %8s | %10s\n", "variant", "ms", "TFLOPS", "% peak");
    std::fprintf(stdout, "%-20s | %8.3f | %8.4f | %9.2f%%\n",
                 "tiled_fp32", (double)ms_tiled, tflops_tiled, tflops_tiled / peak_tflops * 100.0);
    std::fprintf(stdout, "%-20s | %8.3f | %8.4f | %9.2f%%\n",
                 "warp_mma_tc", (double)ms_mma, tflops_mma, tflops_mma / peak_tflops * 100.0);
    // [H2-T45]
    std::fprintf(stdout, "\nspeedup (mma / tiled_fp32): %.2fx\n",
                 (double)ms_tiled / ms_mma);

    // ------------------------------------------------------------
    // [H2-T46] TODO [REQUIRED] step 6: check Tensor Core utilization
    //   ncu --metrics sm__pipe_tensor_cycles_active.avg.pct_of_peak_sustained_active
    //       ./H2_gemm_warp_tile_and_mma
    // ------------------------------------------------------------

    // ------------------------------------------------------------
    // [H2-T47] TODO [ADVANCED] try mma.sync.aligned.m16n8k32 (merge two BK=16 tiles).
    // ------------------------------------------------------------

    // ------------------------------------------------------------
    // [H2-T48] TODO [ADVANCED] try warp tile 16x32 (requires four m16n8k16 mmas).
    // ------------------------------------------------------------

    // ------------------------------------------------------------
    // [H2-T49] TODO [ADVANCED] compare hand-written PTX mma.sync vs the wmma wrapper-generated PTX.
    // ------------------------------------------------------------

    CUDA_CHECK(cudaFree(dA_f32));
    CUDA_CHECK(cudaFree(dB_f32));
    CUDA_CHECK(cudaFree(dA_f16));
    CUDA_CHECK(cudaFree(dB_f16));
    CUDA_CHECK(cudaFree(dC));

    // [H2-T50]
    std::puts("\n[H2] done.");
    return 0;
}
