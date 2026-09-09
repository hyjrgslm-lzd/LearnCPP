// G1_wmma_fp16_basic/main.cu
// [G1-T01] Exercise G1: wmma_fp16_basic - Volta+ nvcuda::wmma API basics
//
// [G1-T02] Learning goals:
//   - Declare and initialize fragment<matrix_a/b/accumulator, M,N,K>
//   - load_matrix_sync from global memory (16-byte aligned)
//   - mma_sync executes 16x16x16 FP16->FP32 Tensor Core multiply-accumulate
//   - store_matrix_sync writes back result, validate against CPU reference
//   - Nsight Compute measures Tensor Core Utilization
//
// [G1-T03] Build: cmake --build build --target G1_wmma_fp16_basic
// [G1-T04] Run:   ./G1_wmma_fp16_basic
// [G1-T05] HW:    sm_70+ (Volta / Turing / Ampere / Ada / Hopper)

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cuda_runtime.h>
#include <cuda_fp16.h>
#include <mma.h>

#include "common/cuda_check.cuh"
#include "common/timer.cuh"
#include "common/device_info.cuh"
#include "common/nvtx_range.cuh"

using namespace nvcuda;

// ------------------------------------------------------------
// [G1-T06] Constants
// ------------------------------------------------------------
constexpr int M_TILE = 16;
constexpr int N_TILE = 16;
constexpr int K_TILE = 16;

// [G1-T07] Matrix size 128x128x128, decomposed into 8x8x8 of 16x16x16 tiles
constexpr int M = 128;
constexpr int N = 128;
constexpr int K = 128;

// ------------------------------------------------------------
// [G1-T08] CPU reference (FP16 inputs -> FP32 accumulation)
// ------------------------------------------------------------
static void cpu_gemm_ref(
    const half* A, const half* B, float* C,
    int m, int n, int k)
{
    // [G1-T09] C[i,j] = sum_l A[i,l] * B[l,j]
    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < n; ++j) {
            float acc = 0.0f;
            for (int l = 0; l < k; ++l) {
                acc += __half2float(A[i * k + l]) * __half2float(B[l * n + j]);
            }
            C[i * n + j] = acc;
        }
    }
}

// ------------------------------------------------------------
// [G1-T10] Verify: per-element compare (relative tolerance 1e-2)
// ------------------------------------------------------------
static bool verify_result(
    const float* gpu, const float* cpu, int total,
    float rel_tol = 1e-2f)
{
    int errors = 0;
    for (int i = 0; i < total; ++i) {
        float diff = std::fabsf(gpu[i] - cpu[i]);
        float ref  = std::fabsf(cpu[i]) + 1e-6f;
        if (diff / ref > rel_tol) {
            if (errors < 5) {
                std::fprintf(stderr,
                    "  [%d] gpu=%.4f  cpu=%.4f  rel_err=%.4f\n",
                    i, gpu[i], cpu[i], diff / ref);
            }
            ++errors;
        }
    }
    return errors == 0;
}

// ------------------------------------------------------------
// [G1-T11] Kernel 1: single-warp wmma GEMM (each warp handles one 16x16x16 tile)
//
// gridDim  = (N/N_TILE, M/M_TILE)
// blockDim = (32, 1, 1)   // single warp
//
// [G1-T12] TODO [REQUIRED] step 1: declare three fragments
// [G1-T13] TODO [REQUIRED] step 3: use load_matrix_sync to load fA / fB
// [G1-T14] TODO [REQUIRED] step 4: mma_sync executes multiply-accumulate
// [G1-T15] TODO [REQUIRED] step 5: store_matrix_sync writes back result
// ------------------------------------------------------------
__global__ void wmma_gemm_kernel(
    const half* __restrict__ A,   // [M, K]  row-major
    const half* __restrict__ B,   // [K, N]  row-major
    float*      __restrict__ C,   // [M, N]  row-major
    int m, int n, int k)
{
    // [G1-T16] Each block (= 1 warp) is responsible for output tile (tile_row, tile_col)
    int tile_row = blockIdx.y;
    int tile_col = blockIdx.x;

    // ------------------------------------------------------------
    // [G1-T17] TODO [REQUIRED] step 1: declare fragments
    //   fragment<matrix_a,    16, 16, 16, half, row_major> fA;
    //   fragment<matrix_b,    16, 16, 16, half, row_major> fB;
    //   fragment<accumulator, 16, 16, 16, float>           fC;
    //   fill_fragment(fC, 0.0f);
    // ------------------------------------------------------------
    wmma::fragment<wmma::matrix_a,    M_TILE, N_TILE, K_TILE, half, wmma::row_major> fA;
    wmma::fragment<wmma::matrix_b,    M_TILE, N_TILE, K_TILE, half, wmma::row_major> fB;
    wmma::fragment<wmma::accumulator, M_TILE, N_TILE, K_TILE, float>                 fC;
    wmma::fill_fragment(fC, 0.0f); // [G1-T18] zero accumulator

    // ------------------------------------------------------------
    // [G1-T19] TODO [REQUIRED] step 3: per K-tile loop load + mma
    //   for each k_tile:
    //     load_matrix_sync(fA, A_ptr, k)   // leading-dim = k (row-major)
    //     load_matrix_sync(fB, B_ptr, n)
    //     mma_sync(fC, fA, fB, fC)
    // ------------------------------------------------------------
    for (int k_tile = 0; k_tile < k / K_TILE; ++k_tile) {
        // [G1-T20] Compute starting pointers for A, B tiles
        const half* A_ptr = A + tile_row * K_TILE * k + k_tile * K_TILE;
        const half* B_ptr = B + k_tile  * K_TILE * n + tile_col * N_TILE;

        // [G1-T21] TODO [REQUIRED] step 3: load_matrix_sync (mind alignment)
        wmma::load_matrix_sync(fA, A_ptr, k);  // leading dim = k (row-major A)
        wmma::load_matrix_sync(fB, B_ptr, n);  // leading dim = n (row-major B)

        // [G1-T22] TODO [REQUIRED] step 4: mma_sync
        wmma::mma_sync(fC, fA, fB, fC);
    }

    // ------------------------------------------------------------
    // [G1-T23] TODO [REQUIRED] step 5: store_matrix_sync write-back
    //   float* C_ptr = C + tile_row * M_TILE * n + tile_col * N_TILE;
    //   store_matrix_sync(C_ptr, fC, n, mem_row_major);
    // ------------------------------------------------------------
    float* C_ptr = C + tile_row * M_TILE * n + tile_col * N_TILE;
    wmma::store_matrix_sync(C_ptr, fC, n, wmma::mem_row_major);
}

// ------------------------------------------------------------
// [G1-T24] Kernel 2 (advanced): tiled GEMM, multi-warp per block
// Each block (WARPS_M x WARPS_N x 1 warps) handles (WARPS_M x M_TILE) x (WARPS_N x N_TILE)
// ------------------------------------------------------------
constexpr int WARPS_M = 2;
constexpr int WARPS_N = 2;

__global__ void wmma_gemm_tiled_kernel(
    const half* __restrict__ A,
    const half* __restrict__ B,
    float*      __restrict__ C,
    int m, int n, int k)
{
    // [G1-T25] warp position within block
    int warp_id  = threadIdx.x / 32;
    int warp_row = warp_id / WARPS_N;           // 0..WARPS_M-1
    int warp_col = warp_id % WARPS_N;           // 0..WARPS_N-1

    // [G1-T26] Global tile this warp is responsible for
    int global_tile_row = blockIdx.y * WARPS_M + warp_row;
    int global_tile_col = blockIdx.x * WARPS_N + warp_col;

    if (global_tile_row * M_TILE >= m || global_tile_col * N_TILE >= n) return;

    // [G1-T27] TODO [REQUIRED] step 1: declare fragments (same as Kernel 1)
    wmma::fragment<wmma::matrix_a,    M_TILE, N_TILE, K_TILE, half, wmma::row_major> fA;
    wmma::fragment<wmma::matrix_b,    M_TILE, N_TILE, K_TILE, half, wmma::row_major> fB;
    wmma::fragment<wmma::accumulator, M_TILE, N_TILE, K_TILE, float>                 fC;
    wmma::fill_fragment(fC, 0.0f);

    // [G1-T28] TODO [REQUIRED] steps 3-4: K loop
    for (int k_tile = 0; k_tile < k / K_TILE; ++k_tile) {
        const half* A_ptr = A + global_tile_row * M_TILE * k + k_tile * K_TILE;
        const half* B_ptr = B + k_tile * K_TILE * n + global_tile_col * N_TILE;

        wmma::load_matrix_sync(fA, A_ptr, k);
        wmma::load_matrix_sync(fB, B_ptr, n);
        wmma::mma_sync(fC, fA, fB, fC);
    }

    // [G1-T29] TODO [REQUIRED] step 5: write back
    float* C_ptr = C + global_tile_row * M_TILE * n + global_tile_col * N_TILE;
    wmma::store_matrix_sync(C_ptr, fC, n, wmma::mem_row_major);
}

// ------------------------------------------------------------
// [G1-T30] main
// ------------------------------------------------------------
int main()
{
    std::puts("[G1_wmma_fp16_basic]");
    print_device_info(0);
    NVTX_RANGE("G1_wmma_fp16_basic/main");

    // ------------------------------------------------------------
    // [G1-T31] TODO [REQUIRED] step 2: build FP16 matrices on host
    //   Allocate M*K (A), K*N (B) half arrays, fill with random values
    //   Copy to device global memory
    // ------------------------------------------------------------
    const size_t sizeA = M * K * sizeof(half);
    const size_t sizeB = K * N * sizeof(half);
    const size_t sizeC = M * N * sizeof(float);

    half*  h_A   = static_cast<half* >(std::malloc(sizeA));
    half*  h_B   = static_cast<half* >(std::malloc(sizeB));
    float* h_C   = static_cast<float*>(std::malloc(sizeC));
    float* h_ref = static_cast<float*>(std::malloc(sizeC));

    // [G1-T32] Fill with small random values to avoid FP16 overflow
    for (int i = 0; i < M * K; ++i)
        h_A[i] = __float2half(static_cast<float>(rand() % 8) / 8.0f - 0.5f);
    for (int i = 0; i < K * N; ++i)
        h_B[i] = __float2half(static_cast<float>(rand() % 8) / 8.0f - 0.5f);

    // [G1-T33] CPU reference (compute before copy)
    cpu_gemm_ref(h_A, h_B, h_ref, M, N, K);

    half*  d_A = nullptr;
    half*  d_B = nullptr;
    float* d_C = nullptr;
    CUDA_CHECK(cudaMalloc(&d_A, sizeA));
    CUDA_CHECK(cudaMalloc(&d_B, sizeB));
    CUDA_CHECK(cudaMalloc(&d_C, sizeC));
    CUDA_CHECK(cudaMemcpy(d_A, h_A, sizeA, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_B, h_B, sizeB, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemset(d_C, 0, sizeC));

    // ------------------------------------------------------------
    // [G1-T34] Kernel 1 launch: each block = 1 warp, grid = (N/N_TILE, M/M_TILE)
    // ------------------------------------------------------------
    {
        NVTX_RANGE("G1/wmma_gemm_single_warp");
        dim3 grid(N / N_TILE, M / M_TILE);
        dim3 block(32, 1, 1);
        // [G1-T35]
        std::printf("launch wmma_gemm_kernel: grid=(%d,%d,1)  block=(32,1,1)\n",
                    grid.x, grid.y);

        CudaEventTimer timer;
        timer.start();
        wmma_gemm_kernel<<<grid, block>>>(d_A, d_B, d_C, M, N, K);
        CUDA_CHECK(cudaGetLastError());
        CUDA_CHECK(cudaDeviceSynchronize());
        timer.stop();

        CUDA_CHECK(cudaMemcpy(h_C, d_C, sizeC, cudaMemcpyDeviceToHost));
        bool ok = verify_result(h_C, h_ref, M * N);
        // [G1-T36]
        std::printf("[wmma_gemm_kernel] %.3f ms  verify: %s\n\n",
                    timer.elapsed_ms(), ok ? "PASS" : "FAIL");
    }

    // ------------------------------------------------------------
    // [G1-T37] Kernel 2 launch: tiled, block = WARPS_M*WARPS_N warps
    // ------------------------------------------------------------
    {
        NVTX_RANGE("G1/wmma_gemm_tiled");
        CUDA_CHECK(cudaMemset(d_C, 0, sizeC));
        dim3 grid(N / (N_TILE * WARPS_N), M / (M_TILE * WARPS_M));
        dim3 block(32 * WARPS_M * WARPS_N, 1, 1);
        // [G1-T38]
        std::printf("launch wmma_gemm_tiled_kernel: grid=(%d,%d,1)  block=(%d,1,1)\n",
                    grid.x, grid.y, block.x);

        CudaEventTimer timer;
        timer.start();
        wmma_gemm_tiled_kernel<<<grid, block>>>(d_A, d_B, d_C, M, N, K);
        CUDA_CHECK(cudaGetLastError());
        CUDA_CHECK(cudaDeviceSynchronize());
        timer.stop();

        CUDA_CHECK(cudaMemcpy(h_C, d_C, sizeC, cudaMemcpyDeviceToHost));
        bool ok = verify_result(h_C, h_ref, M * N);
        // [G1-T39]
        std::printf("[wmma_gemm_tiled_kernel] %.3f ms  verify: %s\n\n",
                    timer.elapsed_ms(), ok ? "PASS" : "FAIL");
    }

    // ------------------------------------------------------------
    // [G1-T40] TODO [REQUIRED] step 6: Nsight Compute performance measurement
    //   ncu --metrics sm__pipe_tensor_cycles_active.avg.pct_of_peak_sustained_active
    //       --target-processes all ./G1_wmma_fp16_basic
    //   Check whether Tensor Core Utilization >= 80%
    // ------------------------------------------------------------

    // [G1-T41] TODO [ADVANCED] try M16N16K32 FP16->FP32 (K=32 version), watch ldmatrix load count change
    // [G1-T42] TODO [ADVANCED] compare sm_70 Volta vs sm_90a Hopper SASS (Nsight PTX view)
    // [G1-T43] TODO [ADVANCED] use half as accumulator (fragment<accumulator, 16,16,16, half>), watch precision and throughput

    CUDA_CHECK(cudaFree(d_A));
    CUDA_CHECK(cudaFree(d_B));
    CUDA_CHECK(cudaFree(d_C));
    std::free(h_A);
    std::free(h_B);
    std::free(h_C);
    std::free(h_ref);

    // [G1-T44]
    std::puts("\n[G1] done. Use ncu --set full ./G1_wmma_fp16_basic to inspect Tensor Core utilization.");
    return 0;
}
