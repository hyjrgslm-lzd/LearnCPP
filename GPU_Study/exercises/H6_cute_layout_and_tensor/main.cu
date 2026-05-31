// H6_cute_layout_and_tensor/main.cu
// ============================================================
// [H6-T01] Exercise H6: cuTe Layout + Tensor + Copy_Atom + MMA_Atom mini-GEMM
//
// [H6-T02] Goal:
//   Learn the four core cuTe concepts (Layout, Tensor, Copy_Atom,
//   MMA_Atom) and hand-write a mini-GEMM kernel without the
//   Collective layer. See how type-level layout abstractions remove
//   the need for manual indexing. Compare the cuTe version against
//   a manual-indexing version in lines of code and readability.
//
// [H6-T03] Build requirements: sm_80+, CUDA 13.x, CUTLASS 3.x headers (cute/ subdir)
// [H6-T04] Dependency: cutlass::headers (provided by CutlassSetup.cmake)
// ============================================================

// ------------------------------------------------------------
// [H6-T05] cuTe core headers
// ------------------------------------------------------------
#include "cute/tensor.hpp"
#include "cute/atom/mma_atom.hpp"
#include "cute/atom/copy_atom.hpp"
#include "cute/layout.hpp"
#include "cute/numeric/integral_constant.hpp"

// ------------------------------------------------------------
// [H6-T06] Exercise common headers
// ------------------------------------------------------------
#include "cuda_check.cuh"
#include "device_info.cuh"
#include "nvtx_range.cuh"
#include "timer.cuh"

#include <cuda_runtime.h>
#include <cuda_fp16.h>

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <algorithm>

// ------------------------------------------------------------
// [H6-T07] Hyperparameters
// ------------------------------------------------------------
static constexpr int BM           = 32;   // [H6-T08] block tile M
static constexpr int BN           = 32;   // [H6-T09] block tile N
static constexpr int BK           = 16;   // [H6-T10] block tile K
static constexpr int WARMUP_ITERS = 3;
static constexpr int BENCH_ITERS  = 10;

// ------------------------------------------------------------
// [H6-T11] Version 1: manual-index GEMM (baseline, no cuTe).
//          Used for line-count and readability comparison.
// ------------------------------------------------------------
__global__ void gemm_manual_index(
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

    // [H6-T12] TODO [REQUIRED] step 6 (baseline): manual-index tiled GEMM.
    //   Outer tile_k loop, manually compute:
    //     sA[ty][tx] = A[(blockIdx.y*BM + ty) * K + (tile_k*BK + tx)]
    //     sB[ty][tx] = B[(tile_k*BK + ty) * N + (blockIdx.x*BN + tx)]
    //   __syncthreads(), inner accumulate, __syncthreads().

    (void)A; (void)B; (void)sA; (void)sB; (void)K; (void)acc;

    if (row < M && col < N) C[row * N + col] = 0.0f; // [H6-T13] stub
}

// ------------------------------------------------------------
// [H6-T14] Version 2: cuTe mini-GEMM (the core learning goal).
//
// Key cuTe concepts:
//   cute::Layout  = Shape x Stride, mapping multidim indices to a linear address
//   cute::Tensor  = pointer + Layout, wrapping arbitrary-rank tensor access
//   cute::Copy_Atom    = smallest hardware copy unit (cp.async / ldmatrix / etc.)
//   cute::MMA_Atom     = smallest Tensor Core op unit (m16n8k16 / etc.)
//
// kernel structure:
//   1. wrap A/B/C global pointers with make_tensor
//   2. carve out the current block's tile via local_tile
//   3. create A/B smem tensors
//   4. partition smem tile down to thread/warp granularity via local_partition
//   5. Copy_Atom moves global -> smem (swappable for cp.async)
//   6. MMA_Atom executes Tensor Core ops
//   7. write the result back to global C
// ------------------------------------------------------------
__global__ void gemm_cute(
    const __half* __restrict__ gA_ptr,  // [H6-T15] [M x K] row-major
    const __half* __restrict__ gB_ptr,  // [H6-T16] [K x N] row-major
    float*        __restrict__ gC_ptr,  // [H6-T17] [M x N] row-major
    int M, int N, int K)
{
    using namespace cute;

    // [H6-T18] -- TODO [REQUIRED] step 1: create global tensors --
    //   make_layout(Shape<int,int>{M, K}, Stride<int,int>{K, 1}) describes a row-major MxK matrix
    //
    //   auto gA = make_tensor(make_gmem_ptr(gA_ptr),
    //                         make_layout(make_shape(M, K), make_stride(K, 1)));
    //   auto gB = make_tensor(make_gmem_ptr(gB_ptr),
    //                         make_layout(make_shape(K, N), make_stride(N, 1)));
    //   auto gC = make_tensor(make_gmem_ptr(gC_ptr),
    //                         make_layout(make_shape(M, N), make_stride(N, 1)));
    //
    // Note: make_shape / make_stride accept either runtime ints or compile-time Int<N>{}.
    //       For smem tiles (fixed size) prefer compile-time shapes for better optimization.

    // [H6-T19] -- TODO [REQUIRED] step 1 (cont.): carve out the current block's tile --
    //   auto blkA = local_tile(gA, make_shape(Int<BM>{}, Int<BK>{}),
    //                          make_coord(blockIdx.y, _));   // first dim = block row
    //   auto blkB = local_tile(gB, make_shape(Int<BK>{}, Int<BN>{}),
    //                          make_coord(_, blockIdx.x));
    //   auto blkC = local_tile(gC, make_shape(Int<BM>{}, Int<BN>{}),
    //                          make_coord(blockIdx.y, blockIdx.x));

    // [H6-T20] -- TODO [REQUIRED] step 1 (cont.): shared memory tensors --
    //   __shared__ __half smem_A[BM * BK];
    //   __shared__ __half smem_B[BK * BN];
    //   auto sA = make_tensor(make_smem_ptr(smem_A),
    //                         make_layout(make_shape(Int<BM>{}, Int<BK>{}),
    //                                     make_stride(Int<BK>{}, Int<1>{})));
    //   auto sB = make_tensor(make_smem_ptr(smem_B),
    //                         make_layout(make_shape(Int<BK>{}, Int<BN>{}),
    //                                     make_stride(Int<BN>{}, Int<1>{})));

    __shared__ __half smem_A[BM * BK];
    __shared__ __half smem_B[BK * BN];

    // [H6-T21] -- TODO [REQUIRED] step 2: tensor access demo (debug) --
    //   if thread 0 in block 0:
    //     auto val = gA(make_coord(threadIdx.y, threadIdx.x));  // access (ty, tx)
    //     printf("gA(0,0) = %f\n", float(val));
    //
    // Note: Tensor::operator() turns multi-dim coords into a linear address;
    //       it is equivalent to gA_ptr[ty * K + tx] with zero runtime overhead.

    // [H6-T22] -- TODO [REQUIRED] step 3: mini-GEMM kernel --
    //
    // // thread partitioning (chop the smem tile by thread):
    // auto thr_copy = ...;  // Copy_Atom partition
    // auto thr_mma  = ...;  // MMA_Atom  partition
    //
    // float acc[...] = {};
    //
    // // K tile loop
    // for (int k_tile = 0; k_tile < K / BK; ++k_tile) {
    //   // -- data movement: global -> smem --
    //   //   using Copy_Atom:
    //   //   auto tAgA = thr_copy.partition_S(blkA(_, _, k_tile));  // src
    //   //   auto tAsA = thr_copy.partition_D(sA);                   // dst
    //   //   copy(thr_copy, tAgA, tAsA);
    //   //
    //   //   without Copy_Atom you can fall back to a simple loop:
    //   //   for (int i = threadIdx.x; i < BM * BK; i += blockDim.x)
    //   //       smem_A[i] = gA_ptr[...];
    //   __syncthreads();
    //
    //   // -- compute: MMA_Atom --
    //   //   auto tCsA = thr_mma.partition_A(sA);
    //   //   auto tCsB = thr_mma.partition_B(sB);
    //   //   auto tCrC = thr_mma.partition_C(blkC);
    //   //   gemm(thr_mma, tCsA, tCsB, tCrC);   // cuTe built-in gemm dispatches to MMA_Atom
    //   __syncthreads();
    // }
    //
    // // -- write back to global C --
    // auto tCgC = thr_mma.partition_C(blkC);
    // copy(tCrC, tCgC);

    // [H6-T23] stub: produce zero output
    int row = blockIdx.y * BM + threadIdx.y;
    int col = blockIdx.x * BN + threadIdx.x;
    if (row < M && col < N)
        gC_ptr[row * N + col] = 0.0f;

    (void)gA_ptr; (void)gB_ptr; (void)K; (void)smem_A; (void)smem_B;

    // [H6-T24] -- TODO [REQUIRED] step 4: rewrite data loads with Copy_Atom --
    //   e.g. SM80_CP_ASYNC_CACHEGLOBAL (cp.async.ca.global.shared.b128)
    //   using CopyAtom = Copy_Atom<SM80_CP_ASYNC_CACHEGLOBAL<sizeof(__half)>, __half>;
    //   TiledCopy tiled_copy = make_tiled_copy(CopyAtom{},
    //       Layout<Shape<_16,_2>, Stride<_2,_1>>{},   // thread layout
    //       Layout<Shape<_1,_8>>{});                   // value layout (8 FP16 per move)
    //
    //   // use tiled_copy in the global->smem transfer inside the K tile loop above

    // [H6-T25] -- TODO [REQUIRED] step 5: rewrite compute with MMA_Atom --
    //   sm_80 FP16 Tensor Core:
    //   using MmaAtom = MMA_Atom<SM80_16x8x16_F32F16F16F32_TN>;
    //   TiledMma tiled_mma = make_tiled_mma(MmaAtom{},
    //       Layout<Shape<_1,_1,_1>>{},    // warp layout
    //       Tile<_16, _16, _16>{});        // tile size
    //
    //   sm_90a wgmma (Hopper):
    //   using MmaAtom = MMA_Atom<SM90_64x64x16_F32F16F16_SS<GMMA::Major::K>>;
}

// ------------------------------------------------------------
// [H6-T26] CPU reference.
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
// [H6-T27] cuTe Layout demo (host side, compile time)
//
// Shows make_layout / print_layout to help understand Shape x Stride.
// ------------------------------------------------------------
static void demo_cute_layouts()
{
    using namespace cute;

    // [H6-T28]
    std::puts("\n-- cuTe Layout demo (host) --");

    // [H6-T29] TODO [REQUIRED] step 1 (cont.): uncomment below to inspect printed layouts
    //
    // // 16x32 row-major matrix: stride = (32, 1)
    // auto layout_rm = make_layout(make_shape(16, 32), make_stride(32, 1));
    // std::printf("row-major 16x32 layout:\n"); print_layout(layout_rm); std::printf("\n");
    //
    // // 16x32 column-major matrix: stride = (1, 16)
    // auto layout_cm = make_layout(make_shape(16, 32), make_stride(1, 16));
    // std::printf("col-major 16x32 layout:\n"); print_layout(layout_cm); std::printf("\n");
    //
    // // smem layout with swizzle (eliminates bank conflicts):
    // // Swizzle<B, M, S>: B=bits, M=shift, S=mix bits
    // auto layout_sw = composition(
    //     Swizzle<3, 3, 3>{},
    //     make_layout(make_shape(16, 16), make_stride(16, 1)));
    // std::printf("Swizzle<3,3,3> 16x16:\n"); print_layout(layout_sw); std::printf("\n");

    // [H6-T30]
    std::puts("  (uncomment the body of demo_cute_layouts to see printed layouts)");
}

// ------------------------------------------------------------
// [H6-T31] main
// ------------------------------------------------------------
int main()
{
    std::puts("[H6_cute_layout_and_tensor]");
    print_device_info();

    NVTX_RANGE("H6_cute_layout_and_tensor/main");

    // [H6-T32] Layout demo (host only)
    demo_cute_layouts();

    const int M = 1024, N = 1024, K = 1024;
    // [H6-T33]
    std::fprintf(stdout, "\nProblem size: M=%d  N=%d  K=%d\n\n", M, N, K);

    // [H6-T34] -- host memory --
    std::vector<float>  hA_f32(M * K), hB_f32(K * N), hC_ref(M * N, 0.0f);
    std::vector<float>  hC_manual(M * N, 0.0f), hC_cute(M * N, 0.0f);
    std::vector<__half> hA_f16(M * K), hB_f16(K * N);

    std::srand(42);
    for (int i = 0; i < M * K; ++i) { hA_f32[i] = static_cast<float>(std::rand()) / RAND_MAX - 0.5f; hA_f16[i] = __float2half(hA_f32[i]); }
    for (int i = 0; i < K * N; ++i) { hB_f32[i] = static_cast<float>(std::rand()) / RAND_MAX - 0.5f; hB_f16[i] = __float2half(hB_f32[i]); }

    // [H6-T35]
    std::puts("Computing CPU reference...");
    gemm_cpu_ref(hA_f32.data(), hB_f32.data(), hC_ref.data(), M, N, K);

    // [H6-T36] -- device memory --
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
    // [H6-T37] Manual-index baseline.
    // ------------------------------------------------------------
    for (int i = 0; i < WARMUP_ITERS; ++i)
        gemm_manual_index<<<grid, block>>>(dA, dB, dC, M, N, K);
    CUDA_CHECK(cudaDeviceSynchronize());
    timer.start();
    for (int i = 0; i < BENCH_ITERS; ++i)
        gemm_manual_index<<<grid, block>>>(dA, dB, dC, M, N, K);
    timer.stop(); CUDA_CHECK_LAST();
    float ms_manual = timer.elapsed_ms() / BENCH_ITERS;
    CUDA_CHECK(cudaMemcpy(hC_manual.data(), dC, sizeof(float) * M * N, cudaMemcpyDeviceToHost));

    // ------------------------------------------------------------
    // [H6-T38] cuTe version.
    // ------------------------------------------------------------
    for (int i = 0; i < WARMUP_ITERS; ++i)
        gemm_cute<<<grid, block>>>(dA, dB, dC, M, N, K);
    CUDA_CHECK(cudaDeviceSynchronize());
    timer.start();
    for (int i = 0; i < BENCH_ITERS; ++i)
        gemm_cute<<<grid, block>>>(dA, dB, dC, M, N, K);
    timer.stop(); CUDA_CHECK_LAST();
    float ms_cute = timer.elapsed_ms() / BENCH_ITERS;
    CUDA_CHECK(cudaMemcpy(hC_cute.data(), dC, sizeof(float) * M * N, cudaMemcpyDeviceToHost));

    // ------------------------------------------------------------
    // [H6-T39] Correctness check.
    // ------------------------------------------------------------
    // [H6-T40]
    std::puts("\n-- Correctness check (FAIL expected at stub stage) --");
    check_result(hC_ref.data(), hC_manual.data(), M, N, "manual_index");
    check_result(hC_ref.data(), hC_cute.data(),   M, N, "cute_gemm");

    // ------------------------------------------------------------
    // [H6-T41] Performance summary.
    // ------------------------------------------------------------
    double peak = 1.5;
    // [H6-T42]
    std::puts("\n-- Performance summary --");
    std::fprintf(stdout, "%-22s | %8s | %8s | %10s\n", "variant", "ms", "TFLOPS", "% peak");
    auto print_row = [&](const char* n, float ms) {
        double t = calc_tflops(M, N, K, ms);
        std::fprintf(stdout, "%-22s | %8.3f | %8.4f | %9.2f%%\n",
                     n, (double)ms, t, t / peak * 100.0);
    };
    print_row("manual_index", ms_manual);
    print_row("cute_gemm",    ms_cute);

    // ------------------------------------------------------------
    // [H6-T43] TODO [REQUIRED] step 6: code-size comparison.
    //   manual-index version (lines) vs cuTe version (lines, indexing region).
    //   Goal: cuTe should be 30-50% smaller.
    // ------------------------------------------------------------

    // ------------------------------------------------------------
    // [H6-T44] TODO [ADVANCED] implement split-K GEMM with cuTe.
    // ------------------------------------------------------------

    // ------------------------------------------------------------
    // [H6-T45] TODO [ADVANCED] compare cuTe hand-written vs CUTLASS Collective auto-generated PTX/SASS.
    // ------------------------------------------------------------

    CUDA_CHECK(cudaFree(dA));
    CUDA_CHECK(cudaFree(dB));
    CUDA_CHECK(cudaFree(dC));

    // [H6-T46]
    std::puts("\n[H6] done.");
    return 0;
}
