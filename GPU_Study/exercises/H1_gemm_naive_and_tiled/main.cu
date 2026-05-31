// H1_gemm_naive_and_tiled/main.cu
// ============================================================
// [H1-T01] Exercise H1: GEMM Naive vs Shared-Memory Tiled
//
// [H1-T02] Goal:
//   Start from the simplest naive GEMM (each thread computes one C
//   element) and optimize to a 32x32 shared-memory tiled version.
//   Compare bandwidth and TFLOPS.
//
// [H1-T03] Build requirements: sm_70+, CUDA 13.x, C++20 device / C++26 host
// ============================================================

#include "cuda_check.cuh"
#include "device_info.cuh"
#include "nvtx_range.cuh"
#include "timer.cuh"

#include <cuda_runtime.h>
#include <cuda_fp16.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cfloat>
#include <vector>
#include <algorithm>

// ------------------------------------------------------------
// [H1-T04] Constants and hyperparameters
// ------------------------------------------------------------
static constexpr int BLOCK_SIZE   = 32;   // [H1-T05] blockDim.x = blockDim.y
static constexpr int TILE_SIZE    = 32;   // [H1-T06] smem tile side length
static constexpr int TILE_K_SIZE  = 32;   // [H1-T07] K-dimension tile width
static constexpr int WARMUP_ITERS = 3;    // [H1-T08] warmup count (discarded)
static constexpr int BENCH_ITERS  = 10;   // [H1-T09] measured iteration count

// ------------------------------------------------------------
// [H1-T10] Version 1: Naive GEMM
//   Each thread (tx, ty) computes one element of C[row][col].
//   Each thread loops over a full row of A and a full column of B,
//   relying entirely on global memory.
// ------------------------------------------------------------
__global__ void gemm_naive(
    const float* __restrict__ A,   // [H1-T11] [M x K], row-major
    const float* __restrict__ B,   // [H1-T12] [K x N], row-major
    float*       __restrict__ C,   // [H1-T13] [M x N], row-major
    int M, int N, int K)
{
    // [H1-T14] TODO [REQUIRED] step 1: implement the naive GEMM.
    //   gridDim  = (ceil(N/BLOCK_SIZE), ceil(M/BLOCK_SIZE))
    //   blockDim = (BLOCK_SIZE, BLOCK_SIZE)
    //   each thread computes C[blockIdx.y*BLOCK_SIZE + ty][blockIdx.x*BLOCK_SIZE + tx]
    //   inner loop: for k in 0..K, accumulate A[row][k] * B[k][col]
    //
    // [H1-T15] Hint: compute row / col first, return early on out-of-bounds.

    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;

    if (row >= M || col >= N) return;

    // [H1-T16] TODO [REQUIRED] step 1 (cont.): complete the inner accumulation
    //          loop and write the result into C[row * N + col].
    (void)A; (void)B; (void)C; (void)K;
    // [H1-T17] stub: produce zero output so the CPU reference flags a mismatch.
    C[row * N + col] = 0.0f;
}

// ------------------------------------------------------------
// [H1-T18] Version 2: Shared-Memory Tiled GEMM
//   The block owns a TILE_SIZE x TILE_SIZE tile of C.
//   Loop K/TILE_K_SIZE times; each iteration loads a small A/B tile
//   into smem and lets the whole block contribute to the C tile.
// ------------------------------------------------------------
__global__ void gemm_tiled(
    const float* __restrict__ A,
    const float* __restrict__ B,
    float*       __restrict__ C,
    int M, int N, int K)
{
    // [H1-T19] TODO [REQUIRED] step 3: declare shared-memory tiles.
    //   __shared__ float sA[TILE_SIZE][TILE_K_SIZE];
    //   __shared__ float sB[TILE_K_SIZE][TILE_SIZE];
    __shared__ float sA[TILE_SIZE][TILE_K_SIZE];
    __shared__ float sB[TILE_K_SIZE][TILE_SIZE];

    int row = blockIdx.y * TILE_SIZE + threadIdx.y;
    int col = blockIdx.x * TILE_SIZE + threadIdx.x;

    float acc = 0.0f;

    // [H1-T20] TODO [REQUIRED] step 3 (cont.): outer loop tile_k = 0 .. K/TILE_K_SIZE
    //   1. cooperatively load A tile: sA[threadIdx.y][threadIdx.x] = A[row][tile_k*TILE_K_SIZE + threadIdx.x]
    //   2. cooperatively load B tile: sB[threadIdx.y][threadIdx.x] = B[(tile_k*TILE_K_SIZE + threadIdx.y)][col]
    //   3. __syncthreads() so the tile is in place
    //   4. inner compute: acc += sA[threadIdx.y][k] * sB[k][threadIdx.x]
    //   5. __syncthreads() before loading the next tile
    //
    // [H1-T21] Note: bounds checks (row < M, col < N, tile coord < K).

    (void)A; (void)B; (void)sA; (void)sB; (void)K; (void)acc;

    // [H1-T22] stub: produce zero output.
    if (row < M && col < N)
        C[row * N + col] = 0.0f;
}

// ------------------------------------------------------------
// [H1-T23] CPU reference (triple loop, used for correctness check).
// ------------------------------------------------------------
static void gemm_cpu_ref(
    const float* A, const float* B, float* C,
    int M, int N, int K)
{
    for (int m = 0; m < M; ++m) {
        for (int n = 0; n < N; ++n) {
            float acc = 0.0f;
            for (int k = 0; k < K; ++k)
                acc += A[m * K + k] * B[k * N + n];
            C[m * N + n] = acc;
        }
    }
}

// ------------------------------------------------------------
// [H1-T24] Correctness check (element-wise, 1e-3 relative tolerance).
// ------------------------------------------------------------
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
        if (rel > 1e-3f) ++mismatch;
    }
    bool ok = (mismatch == 0);
    // [H1-T25]
    std::fprintf(stdout, "  [%s] %s  max_rel_err %.2e  mismatch %d/%d\n",
                 label, ok ? "PASS" : "FAIL", (double)max_err, mismatch, M * N);
    return ok;
}

// ------------------------------------------------------------
// [H1-T26] TFLOPS calculation.
// ------------------------------------------------------------
static double calc_tflops(int M, int N, int K, float ms) {
    return 2.0 * M * N * K / (ms * 1e-3) / 1e12;
}

// ------------------------------------------------------------
// [H1-T27] main
// ------------------------------------------------------------
int main()
{
    std::puts("[H1_gemm_naive_and_tiled]");
    print_device_info();

    NVTX_RANGE("H1_gemm_naive_and_tiled/main");

    // [H1-T28] -- problem size --
    const int M = 1024, N = 1024, K = 1024;
    std::fprintf(stdout, "Problem size: M=%d  N=%d  K=%d\n\n", M, N, K);

    // [H1-T29] -- allocate host memory --
    std::vector<float> hA(M * K), hB(K * N), hC_ref(M * N, 0.0f);
    std::vector<float> hC_naive(M * N, 0.0f), hC_tiled(M * N, 0.0f);

    // [H1-T30] random init (fixed seed for reproducibility)
    std::srand(42);
    for (auto& v : hA) v = static_cast<float>(std::rand()) / RAND_MAX - 0.5f;
    for (auto& v : hB) v = static_cast<float>(std::rand()) / RAND_MAX - 0.5f;

    // [H1-T31] -- CPU reference --
    std::puts("Computing CPU reference (may take several seconds)...");
    gemm_cpu_ref(hA.data(), hB.data(), hC_ref.data(), M, N, K);

    // [H1-T32] -- allocate device memory --
    float *dA, *dB, *dC;
    CUDA_CHECK(cudaMalloc(&dA, sizeof(float) * M * K));
    CUDA_CHECK(cudaMalloc(&dB, sizeof(float) * K * N));
    CUDA_CHECK(cudaMalloc(&dC, sizeof(float) * M * N));

    CUDA_CHECK(cudaMemcpy(dA, hA.data(), sizeof(float) * M * K, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(dB, hB.data(), sizeof(float) * K * N, cudaMemcpyHostToDevice));

    // [H1-T33] -- launch config --
    dim3 block(BLOCK_SIZE, BLOCK_SIZE);
    dim3 grid_naive((N + BLOCK_SIZE - 1) / BLOCK_SIZE,
                    (M + BLOCK_SIZE - 1) / BLOCK_SIZE);
    dim3 grid_tiled((N + TILE_SIZE  - 1) / TILE_SIZE,
                    (M + TILE_SIZE  - 1) / TILE_SIZE);

    std::fprintf(stdout, "Naive  grid=(%u,%u)  block=(%u,%u)\n",
                 grid_naive.x, grid_naive.y, block.x, block.y);
    std::fprintf(stdout, "Tiled  grid=(%u,%u)  block=(%u,%u)\n\n",
                 grid_tiled.x, grid_tiled.y, block.x, block.y);

    CudaEventTimer timer;

    // ------------------------------------------------------------
    // [H1-T34] TODO [REQUIRED] step 2: measure the naive version.
    // ------------------------------------------------------------
    // [H1-T35] warmup
    for (int i = 0; i < WARMUP_ITERS; ++i) {
        gemm_naive<<<grid_naive, block>>>(dA, dB, dC, M, N, K);
    }
    CUDA_CHECK(cudaDeviceSynchronize());

    // [H1-T36] timed loop
    timer.start();
    for (int i = 0; i < BENCH_ITERS; ++i) {
        gemm_naive<<<grid_naive, block>>>(dA, dB, dC, M, N, K);
    }
    timer.stop();
    CUDA_CHECK_LAST();
    float ms_naive = timer.elapsed_ms() / BENCH_ITERS;

    CUDA_CHECK(cudaMemcpy(hC_naive.data(), dC, sizeof(float) * M * N, cudaMemcpyDeviceToHost));

    // ------------------------------------------------------------
    // [H1-T37] TODO [REQUIRED] step 4: measure the tiled version.
    // ------------------------------------------------------------
    for (int i = 0; i < WARMUP_ITERS; ++i) {
        gemm_tiled<<<grid_tiled, block>>>(dA, dB, dC, M, N, K);
    }
    CUDA_CHECK(cudaDeviceSynchronize());

    timer.start();
    for (int i = 0; i < BENCH_ITERS; ++i) {
        gemm_tiled<<<grid_tiled, block>>>(dA, dB, dC, M, N, K);
    }
    timer.stop();
    CUDA_CHECK_LAST();
    float ms_tiled = timer.elapsed_ms() / BENCH_ITERS;

    CUDA_CHECK(cudaMemcpy(hC_tiled.data(), dC, sizeof(float) * M * N, cudaMemcpyDeviceToHost));

    // ------------------------------------------------------------
    // [H1-T38] Correctness check (stub kernels emit zero, CPU ref non-zero
    //          -> FAIL is expected at the stub stage).
    // ------------------------------------------------------------
    // [H1-T39]
    std::puts("\n-- Correctness check (FAIL expected at stub stage) --");
    check_result(hC_ref.data(), hC_naive.data(), M, N, "naive");
    check_result(hC_ref.data(), hC_tiled.data(), M, N, "tiled");

    // ------------------------------------------------------------
    // [H1-T40] TODO [REQUIRED] step 5: performance comparison table.
    // ------------------------------------------------------------
    double tflops_naive = calc_tflops(M, N, K, ms_naive);
    double tflops_tiled = calc_tflops(M, N, K, ms_tiled);

    // [H1-T41] Theoretical peak (FP32; query device props; placeholder 1.5 TFLOPS).
    // [H1-T42] TODO [REQUIRED] step 5 (cont.): use cudaDeviceGetAttribute to
    //          query the actual FP32 peak instead of hard-coding it.
    double peak_tflops = 1.5; // [H1-T43] Hopper H100 SXM FP32 single-card peak (estimate)

    // [H1-T44]
    std::puts("\n-- Performance summary --");
    std::fprintf(stdout, "%-20s | %8s | %8s | %10s\n",
                 "variant", "ms", "TFLOPS", "% peak");
    std::fprintf(stdout, "%-20s | %8.3f | %8.4f | %9.2f%%\n",
                 "naive",
                 (double)ms_naive, tflops_naive,
                 tflops_naive / peak_tflops * 100.0);
    std::fprintf(stdout, "%-20s | %8.3f | %8.4f | %9.2f%%\n",
                 "tiled (smem 32x32)",
                 (double)ms_tiled, tflops_tiled,
                 tflops_tiled / peak_tflops * 100.0);

    // ------------------------------------------------------------
    // [H1-T45] Rough memory-bandwidth utilization estimate.
    //   naive reads A(M*K) + B(K*N) bytes per GEMM (theoretical lower bound).
    // ------------------------------------------------------------
    double bytes_naive = sizeof(float) * (static_cast<double>(M) * K + static_cast<double>(K) * N);
    double bw_naive_GBs = bytes_naive / (ms_naive * 1e-3) / 1e9;
    // [H1-T46]
    std::fprintf(stdout, "\nNaive estimated read BW (lower bound): %.2f GB/s\n", bw_naive_GBs);

    // ------------------------------------------------------------
    // [H1-T47] TODO [ADVANCED] eliminate bank conflicts via padding on the smem version.
    // ------------------------------------------------------------

    // ------------------------------------------------------------
    // [H1-T48] TODO [ADVANCED] sweep BLOCK_SIZE / TILE_K_SIZE for the optimum.
    // ------------------------------------------------------------

    // ------------------------------------------------------------
    // [H1-T49] TODO [ADVANCED] manually unroll the innermost K loop (#pragma unroll).
    // ------------------------------------------------------------

    // ------------------------------------------------------------
    // [H1-T50] TODO [REQUIRED] step 6: profile both versions with
    //          Nsight Compute --set full. Record "Memory Bound" vs
    //          "Compute Bound", Achieved Occupancy, L1/L2 Cache Hit
    //          Rate, and draw the roofline diagram.
    // ------------------------------------------------------------

    // [H1-T51] -- cleanup --
    CUDA_CHECK(cudaFree(dA));
    CUDA_CHECK(cudaFree(dB));
    CUDA_CHECK(cudaFree(dC));

    // [H1-T52]
    std::puts("\n[H1] done.");
    return 0;
}
