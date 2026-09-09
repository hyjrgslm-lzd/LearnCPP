// H3_gemm_pipelined_double_buffer/main.cu
// ============================================================
// [H3-T01] Exercise H3: double-buffered / multi-stage pipelined GEMM
//          (cp.async + cuda::pipeline)
//
// [H3-T02] Goal:
//   On top of the H2 warp-tile version, introduce 2-stage and 3-stage
//   double-buffered shared-memory pipelines. The producer issues
//   cp.async (sm_80+) to load the next K-tile while the consumer warp
//   runs mma.sync on the current tile, hiding global memory latency.
//
// [H3-T03] Build requirements: sm_80+, CUDA 13.x, C++20 device / C++26 host
// ============================================================

#include "common/cuda_check.cuh"
#include "common/device_info.cuh"
#include "common/nvtx_range.cuh"
#include "common/timer.cuh"

#include <cuda_runtime.h>
#include <cuda_fp16.h>
#include <cuda/pipeline>
#include <cuda/barrier>
#include <cooperative_groups/memcpy_async.h>

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <algorithm>

// ------------------------------------------------------------
// [H3-T04] Constants and hyperparameters
// ------------------------------------------------------------
static constexpr int BM           = 32;
static constexpr int BN           = 32;
static constexpr int BK           = 16;
static constexpr int NUM_STAGES   = 2;   // [H3-T05] set to 3 to test the 3-stage pipeline
static constexpr int WARMUP_ITERS = 3;
static constexpr int BENCH_ITERS  = 10;

// ------------------------------------------------------------
// [H3-T06] Version 1 (baseline): tiled GEMM without pipeline (FP16 in, FP32 accum).
// ------------------------------------------------------------
__global__ void gemm_no_pipeline(
    const __half* __restrict__ A,
    const __half* __restrict__ B,
    float*        __restrict__ C,
    int M, int N, int K)
{
    __shared__ __half sA[BM][BK];
    __shared__ __half sB[BK][BN];

    int row = blockIdx.y * BM + threadIdx.y;
    int col = blockIdx.x * BN + threadIdx.x;
    float acc = 0.0f;

    // [H3-T07] TODO [REQUIRED] step 1: copy the H2 tiled logic as the no-pipeline baseline.
    //   sync load -> __syncthreads -> mma / scalar compute -> __syncthreads.

    (void)A; (void)B; (void)sA; (void)sB; (void)K; (void)acc;

    if (row < M && col < N) C[row * N + col] = 0.0f; // [H3-T08] stub
}

// ------------------------------------------------------------
// [H3-T09] Version 2: 2-stage double-buffer pipeline (cuda::pipeline API).
//
// smem layout: sA[NUM_STAGES][BM][BK], sB[NUM_STAGES][BK][BN]
// producer: __pipeline_memcpy_async / cuda::memcpy_async fills the current stage
// consumer: runs mma.sync against the previous stage
// ------------------------------------------------------------
__global__ void gemm_pipeline_2stage(
    const __half* __restrict__ A,
    const __half* __restrict__ B,
    float*        __restrict__ C,
    int M, int N, int K)
{
    // [H3-T10] TODO [REQUIRED] step 1: declare multi-stage smem buffers.
    //   __shared__ __half sA[NUM_STAGES][BM][BK];
    //   __shared__ __half sB[NUM_STAGES][BK][BN];
    __shared__ __half sA[NUM_STAGES][BM][BK];
    __shared__ __half sB[NUM_STAGES][BK][BN];

    int row = blockIdx.y * BM + threadIdx.y;
    int col = blockIdx.x * BN + threadIdx.x;
    float acc = 0.0f;
    int num_tiles = K / BK;

    // [H3-T11] TODO [REQUIRED] step 2: prologue -- load tile 0 into stage 0.
    //   issue an async copy via __pipeline_memcpy_async or cuda::memcpy_async,
    //   then __pipeline_commit().

    // [H3-T12] TODO [REQUIRED] step 3: pipeline main loop.
    //   for (int tile = 1; tile < num_tiles; ++tile) {
    //     int cur_stage  = tile % NUM_STAGES;
    //     int prev_stage = (tile - 1) % NUM_STAGES;
    //
    //     // producer: async load tile into cur_stage
    //     // (cp.async: each thread is responsible for a few elements)
    //     __pipeline_memcpy_async(dst, src, bytes);
    //     __pipeline_commit();
    //
    //     // wait for prev_stage to be ready
    //     __pipeline_wait_prior(1);   // at most 1 pending commit
    //     __syncthreads();
    //
    //     // consumer: run mma.sync (or scalar accumulate) on prev_stage
    //     // (same logic as H2 warp tile, reading sA[prev_stage] / sB[prev_stage])
    //     for (int k = 0; k < BK; ++k)
    //         acc += __half2float(sA[prev_stage][threadIdx.y][k])
    //              * __half2float(sB[prev_stage][k][threadIdx.x]);
    //   }
    //
    //   // tail: drain the last commit and process the final tile
    //   __pipeline_wait_prior(0);
    //   __syncthreads();
    //   int last_stage = (num_tiles - 1) % NUM_STAGES;
    //   for (int k = 0; k < BK; ++k)
    //       acc += __half2float(sA[last_stage][threadIdx.y][k])
    //            * __half2float(sB[last_stage][k][threadIdx.x]);

    (void)A; (void)B; (void)sA; (void)sB; (void)K; (void)num_tiles; (void)acc;

    if (row < M && col < N) C[row * N + col] = 0.0f; // [H3-T13] stub
}

// ------------------------------------------------------------
// [H3-T14] Version 3: 3-stage pipeline (variant after setting NUM_STAGES=3).
//   Same structure as version 2, only the stage count differs;
//   shown separately for an easier comparison.
// ------------------------------------------------------------
__global__ void gemm_pipeline_3stage(
    const __half* __restrict__ A,
    const __half* __restrict__ B,
    float*        __restrict__ C,
    int M, int N, int K)
{
    constexpr int STAGES = 3;
    __shared__ __half sA[STAGES][BM][BK];
    __shared__ __half sB[STAGES][BK][BN];

    int row = blockIdx.y * BM + threadIdx.y;
    int col = blockIdx.x * BN + threadIdx.x;
    float acc = 0.0f;

    // [H3-T15] TODO [REQUIRED] step 4: implement the 3-stage pipeline.
    //   Same logic as version 2, replace NUM_STAGES with 3,
    //   __pipeline_wait_prior(2) allows up to 2 pending commits.
    //   Note: prologue must prefill stage 0 and stage 1 (two tiles).

    (void)A; (void)B; (void)sA; (void)sB; (void)K; (void)acc;

    if (row < M && col < N) C[row * N + col] = 0.0f; // [H3-T16] stub
}

// ------------------------------------------------------------
// [H3-T17] CPU reference.
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
    int mismatch = 0; float max_err = 0.0f;
    for (int i = 0; i < M * N; ++i) {
        float rel = std::fabs(ref[i] - got[i]) / (std::fabs(ref[i]) + 1e-6f);
        max_err   = std::max(max_err, rel);
        if (rel > 1e-2f) ++mismatch;
    }
    bool ok = (mismatch == 0);
    std::fprintf(stdout, "  [%s] %s  max_rel=%.2e  mismatch=%d/%d\n",
                 label, ok ? "PASS" : "FAIL", (double)max_err, mismatch, M * N);
    return ok;
}

static double calc_tflops(int M, int N, int K, float ms) {
    return 2.0 * M * N * K / (ms * 1e-3) / 1e12;
}

// ------------------------------------------------------------
// [H3-T18] main
// ------------------------------------------------------------
int main()
{
    std::puts("[H3_gemm_pipelined_double_buffer]");
    print_device_info();

    NVTX_RANGE("H3_gemm_pipelined_double_buffer/main");

    const int M = 2048, N = 2048, K = 2048;
    // [H3-T19]
    std::fprintf(stdout, "Problem size: M=%d  N=%d  K=%d\n\n", M, N, K);

    std::vector<float>  hA_f32(M * K), hB_f32(K * N), hC_ref(M * N, 0.0f);
    std::vector<float>  hC_np(M * N, 0.0f), hC_p2(M * N, 0.0f), hC_p3(M * N, 0.0f);
    std::vector<__half> hA_f16(M * K), hB_f16(K * N);

    std::srand(42);
    for (int i = 0; i < M * K; ++i) { hA_f32[i] = static_cast<float>(std::rand()) / RAND_MAX - 0.5f; hA_f16[i] = __float2half(hA_f32[i]); }
    for (int i = 0; i < K * N; ++i) { hB_f32[i] = static_cast<float>(std::rand()) / RAND_MAX - 0.5f; hB_f16[i] = __float2half(hB_f32[i]); }

    // [H3-T20]
    std::puts("Computing CPU reference (may take tens of seconds)...");
    gemm_cpu_ref(hA_f32.data(), hB_f32.data(), hC_ref.data(), M, N, K);

    __half *dA, *dB; float *dC;
    CUDA_CHECK(cudaMalloc(&dA, sizeof(__half) * M * K));
    CUDA_CHECK(cudaMalloc(&dB, sizeof(__half) * K * N));
    CUDA_CHECK(cudaMalloc(&dC, sizeof(float)  * M * N));
    CUDA_CHECK(cudaMemcpy(dA, hA_f16.data(), sizeof(__half) * M * K, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(dB, hB_f16.data(), sizeof(__half) * K * N, cudaMemcpyHostToDevice));

    dim3 block(BN, BM);
    dim3 grid((N + BN - 1) / BN, (M + BM - 1) / BM);
    std::fprintf(stdout, "grid=(%u,%u)  block=(%u,%u)\n\n", grid.x, grid.y, block.x, block.y);

    CudaEventTimer timer;

    // ------------------------------------------------------------
    // [H3-T21] No-pipeline baseline.
    // ------------------------------------------------------------
    for (int i = 0; i < WARMUP_ITERS; ++i)
        gemm_no_pipeline<<<grid, block>>>(dA, dB, dC, M, N, K);
    CUDA_CHECK(cudaDeviceSynchronize());
    timer.start();
    for (int i = 0; i < BENCH_ITERS; ++i)
        gemm_no_pipeline<<<grid, block>>>(dA, dB, dC, M, N, K);
    timer.stop(); CUDA_CHECK_LAST();
    float ms_np = timer.elapsed_ms() / BENCH_ITERS;
    CUDA_CHECK(cudaMemcpy(hC_np.data(), dC, sizeof(float) * M * N, cudaMemcpyDeviceToHost));

    // ------------------------------------------------------------
    // [H3-T22] TODO [REQUIRED] step 5: measure the 2-stage pipeline.
    // ------------------------------------------------------------
    for (int i = 0; i < WARMUP_ITERS; ++i)
        gemm_pipeline_2stage<<<grid, block>>>(dA, dB, dC, M, N, K);
    CUDA_CHECK(cudaDeviceSynchronize());
    timer.start();
    for (int i = 0; i < BENCH_ITERS; ++i)
        gemm_pipeline_2stage<<<grid, block>>>(dA, dB, dC, M, N, K);
    timer.stop(); CUDA_CHECK_LAST();
    float ms_p2 = timer.elapsed_ms() / BENCH_ITERS;
    CUDA_CHECK(cudaMemcpy(hC_p2.data(), dC, sizeof(float) * M * N, cudaMemcpyDeviceToHost));

    // ------------------------------------------------------------
    // [H3-T23] TODO [REQUIRED] step 4: measure the 3-stage pipeline.
    // ------------------------------------------------------------
    for (int i = 0; i < WARMUP_ITERS; ++i)
        gemm_pipeline_3stage<<<grid, block>>>(dA, dB, dC, M, N, K);
    CUDA_CHECK(cudaDeviceSynchronize());
    timer.start();
    for (int i = 0; i < BENCH_ITERS; ++i)
        gemm_pipeline_3stage<<<grid, block>>>(dA, dB, dC, M, N, K);
    timer.stop(); CUDA_CHECK_LAST();
    float ms_p3 = timer.elapsed_ms() / BENCH_ITERS;
    CUDA_CHECK(cudaMemcpy(hC_p3.data(), dC, sizeof(float) * M * N, cudaMemcpyDeviceToHost));

    // ------------------------------------------------------------
    // [H3-T24] Correctness check.
    // ------------------------------------------------------------
    // [H3-T25]
    std::puts("\n-- Correctness check (FAIL expected at stub stage) --");
    check_result(hC_ref.data(), hC_np.data(), M, N, "no_pipeline");
    check_result(hC_ref.data(), hC_p2.data(), M, N, "pipeline_2stage");
    check_result(hC_ref.data(), hC_p3.data(), M, N, "pipeline_3stage");

    // ------------------------------------------------------------
    // [H3-T26] Performance summary.
    // ------------------------------------------------------------
    double peak = 1.5;
    // [H3-T27]
    std::puts("\n-- Performance summary --");
    std::fprintf(stdout, "%-22s | %8s | %8s | %10s\n", "variant", "ms", "TFLOPS", "% peak");
    auto print_row = [&](const char* name, float ms) {
        double t = calc_tflops(M, N, K, ms);
        std::fprintf(stdout, "%-22s | %8.3f | %8.4f | %9.2f%%\n",
                     name, (double)ms, t, t / peak * 100.0);
    };
    print_row("no_pipeline",     ms_np);
    print_row("pipeline_2stage", ms_p2);
    print_row("pipeline_3stage", ms_p3);
    // [H3-T28]
    std::fprintf(stdout, "\n2-stage speedup: %.2fx  3-stage speedup: %.2fx\n",
                 (double)ms_np / ms_p2, (double)ms_np / ms_p3);

    // ------------------------------------------------------------
    // [H3-T29] TODO [REQUIRED] step 6: observe "Smem/Dmem Pipeline stall" in Nsight Compute.
    //   ncu --metrics l1tex__t_sectors_pipe_lsu_mem_global_op_ld.sum
    //       ./H3_gemm_pipelined_double_buffer
    // ------------------------------------------------------------

    // ------------------------------------------------------------
    // [H3-T30] TODO [ADVANCED] implement a deeper pipeline and overlap further.
    // ------------------------------------------------------------

    // ------------------------------------------------------------
    // [H3-T31] TODO [ADVANCED] compare cp.async vs cuda::memcpy_async performance.
    // ------------------------------------------------------------

    CUDA_CHECK(cudaFree(dA));
    CUDA_CHECK(cudaFree(dB));
    CUDA_CHECK(cudaFree(dC));

    // [H3-T32]
    std::puts("\n[H3] done.");
    return 0;
}
