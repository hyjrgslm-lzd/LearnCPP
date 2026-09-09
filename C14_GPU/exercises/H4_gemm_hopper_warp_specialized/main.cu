// H4_gemm_hopper_warp_specialized/main.cu
// ============================================================
// [H4-T01] Exercise H4: Hopper Warp-Specialized GEMM
//   producer warp (TMA) + consumer warps (wgmma) + cluster launch
//
// [H4-T02] Goal:
//   Integrate the module-G techniques (wgmma, TMA, mbarrier, warp
//   specialization) into a complete GEMM kernel. The producer warp
//   moves A/B tiles via TMA bulk copy and consumer warps run wgmma;
//   __cluster_dims__(2,1,1) launch is optional. Aim for near-library
//   performance on sm_90a (target 70-85% of Hopper FP32 peak).
//
// [H4-T03] Build requirements: sm_90a, CUDA 13.x, C++20 device / C++26 host
// ============================================================

#include "common/cuda_check.cuh"
#include "common/device_info.cuh"
#include "common/nvtx_range.cuh"
#include "common/timer.cuh"

#include <cuda_runtime.h>
#include <cuda_fp16.h>
#include <cuda/barrier>

// [H4-T04] cuda/ptx provides wgmma / cp.async.bulk and other Hopper PTX bindings.
#if defined(__CUDACC__) && (__CUDA_ARCH__ >= 900 || !defined(__CUDA_ARCH__))
#  include <cuda/ptx>
#endif

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <algorithm>

// ------------------------------------------------------------
// [H4-T05] Hopper runtime detection macro
//   Skip kernel launch on non-sm_90a devices (avoid illegal-instruction crash).
// ------------------------------------------------------------
#define HOPPER_SKIP_IF_UNSUPPORTED(device)                        \
    do {                                                          \
        if (!has_hopper_features(device)) {                       \
            std::puts("[H4] Current GPU lacks Hopper (sm_90a) features; skipping kernel."); \
            return 0;                                             \
        }                                                         \
    } while (0)

// ------------------------------------------------------------
// [H4-T06] Hyperparameters
// ------------------------------------------------------------
static constexpr int BM           = 64;   // [H4-T07] block tile M
static constexpr int BN           = 64;   // [H4-T08] block tile N
static constexpr int BK           = 16;   // [H4-T09] block tile K
static constexpr int NUM_STAGES   = 2;    // [H4-T10] pipeline stage count
static constexpr int WARMUP_ITERS = 3;
static constexpr int BENCH_ITERS  = 10;

// ------------------------------------------------------------
// [H4-T11] Hopper warp-specialized GEMM kernel
//
// blockDim = (256, 1, 1):
//   warp 0          (thread 0-31)  : producer, owns TMA copies
//   warp 1-7        (thread 32-255): consumer, owns wgmma
//
// cluster: __cluster_dims__(2,1,1) optional;
//          launch via cudaLaunchKernelEx if used.
//
// smem layout:
//   sA[NUM_STAGES][BM][BK] (FP16, TMA-swizzled)
//   sB[NUM_STAGES][BK][BN] (FP16, TMA-swizzled)
//   mbarrier[NUM_STAGES] (producer-consumer handshake)
// ------------------------------------------------------------

// [H4-T12] NOTE: wgmma / TMA instructions are only compiled on the sm_90a
//          device path. Host code has no __CUDA_ARCH__, so the guard
//          prevents the compiler from expanding the PTX paths.
#if defined(__CUDA_ARCH__) && (__CUDA_ARCH__ >= 900)

__global__
__launch_bounds__(256)
void gemm_hopper_warp_specialized(
    const __half* __restrict__  A,          // [H4-T13] [M x K] row-major
    const __half* __restrict__  B,          // [H4-T14] [K x N] row-major
    float*        __restrict__  C,          // [H4-T15] [M x N] row-major
    const void*   __grid_constant__ descA,  // [H4-T16] TMA tensor descriptor (cuTensorMap)
    const void*   __grid_constant__ descB,
    int M, int N, int K)
{
    // [H4-T17] -- Shared memory layout --
    __shared__ __align__(128) __half sA[NUM_STAGES][BM][BK];
    __shared__ __align__(128) __half sB[NUM_STAGES][BK][BN];

    // [H4-T18] mbarrier: one per stage, used for producer-consumer handshake.
    __shared__ uint64_t mbar[NUM_STAGES];

    int warp_id = threadIdx.x / 32;
    int lane    = threadIdx.x % 32;

    // [H4-T19] -- mbarrier init (only thread 0) --
    if (threadIdx.x == 0) {
        for (int s = 0; s < NUM_STAGES; ++s) {
            // [H4-T20] TODO [REQUIRED] step 1 (cont.): init mbarrier with expect
            //          count = 1 (producer arrives after TMA completes).
            //   cuda::barrier<cuda::thread_scope_block> bar;
            //   init(&bar, 1);
            //   mbar[s] = ...; // really uses __cuda_ptx_mbarrier_init_b64
            mbar[s] = 0; // [H4-T21] stub
        }
    }
    __syncthreads();

    // [H4-T22] -- starting coordinates of the C tile owned by this block --
    int tile_row = blockIdx.y * BM;
    int tile_col = blockIdx.x * BN;
    int num_k_tiles = K / BK;

    // [H4-T23] -- accumulator registers (consumer warps) --
    // wgmma m64n64k16 output: 64x64 FP32 held by a warp group (4 warps);
    // 64 FP32 regs per thread for m64n64k16 -> 32 accum/thread.
    float fragC[32] = {};  // [H4-T24] stub size, real size depends on wgmma shape

    // [H4-T25] TODO [REQUIRED] step 2: producer logic (warp 0).
    //   if (warp_id == 0) {
    //     // prefill stage 0
    //     // cp.async.bulk.tensor (TMA): move BMxBK tile from global to sA[0]
    //     //   asm volatile("cp.async.bulk.tensor.2d.shared::cluster.global.mbarrier::complete_tx::bytes"
    //     //                " [%0], [%1, {%2, %3}], [%4];"
    //     //                : : "r"(smem_ptr_sA0), "l"(descA), "r"(tile_row), "r"(0), "r"(mbar_ptr) : "memory");
    //     // similar for B
    //     //
    //     // main loop:
    //     //   for (int k = 0; k < num_k_tiles; ++k) {
    //     //     int s = k % NUM_STAGES;
    //     //     // wait for the consumer to release this stage (mbarrier wait)
    //     //     // issue the next TMA (k+1 tile, if in range)
    //     //     // arrive mbarrier[s] (notify consumer that data is ready)
    //     //   }
    //   }

    // [H4-T26] TODO [REQUIRED] step 3: consumer logic (warps 1-7).
    //   if (warp_id >= 1) {
    //     for (int k = 0; k < num_k_tiles; ++k) {
    //       int s = k % NUM_STAGES;
    //       // wait for producer to finish this stage (mbarrier::arrive_and_wait)
    //       //   asm volatile("mbarrier.arrive.expect_tx.shared.b64 ...")
    //       //
    //       // run wgmma (warp group matmul accumulate):
    //       //   warpgroup.mma.sync.aligned.m64n64k16.f32.f16.f16 {frag_d...}, [sA[s]], [sB[s]];
    //       //
    //       // signal producer that the stage can be reused
    //     }
    //   }

    // [H4-T27] TODO [REQUIRED] step 4: epilogue -- write fragC back to global C.
    //   Need to compute each thread's row/column inside the BMxBN tile and
    //   add tile_row/tile_col offsets.

    // [H4-T28] stub: zero the output
    int row = tile_row + (threadIdx.x / (BN / 1));
    int col = tile_col + (threadIdx.x % (BN / 1));
    if (row < M && col < N && threadIdx.x < BM)
        C[row * N + col] = 0.0f;

    (void)A; (void)B; (void)descA; (void)descB; (void)K;
    (void)warp_id; (void)lane; (void)tile_row; (void)tile_col;
    (void)num_k_tiles; (void)fragC; (void)mbar;
}

#else // [H4-T29] non-sm_90a path: empty kernel placeholder to keep linking.

__global__ void gemm_hopper_warp_specialized(
    const __half*, const __half*, float*,
    const void*, const void*, int, int, int) {}

#endif // __CUDA_ARCH__ >= 900

// ------------------------------------------------------------
// [H4-T30] CPU reference.
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
// [H4-T31] main
// ------------------------------------------------------------
int main()
{
    std::puts("[H4_gemm_hopper_warp_specialized]");
    print_device_info();

    NVTX_RANGE("H4_gemm_hopper_warp_specialized/main");

    // [H4-T32] -- Hopper runtime check --
    HOPPER_SKIP_IF_UNSUPPORTED(0);

    const int M = 2048, N = 2048, K = 2048;
    // [H4-T33]
    std::fprintf(stdout, "Problem size: M=%d  N=%d  K=%d\n\n", M, N, K);

    // [H4-T34] -- host memory --
    std::vector<float>  hA_f32(M * K), hB_f32(K * N), hC_ref(M * N, 0.0f);
    std::vector<float>  hC_out(M * N, 0.0f);
    std::vector<__half> hA_f16(M * K), hB_f16(K * N);

    std::srand(42);
    for (int i = 0; i < M * K; ++i) { hA_f32[i] = static_cast<float>(std::rand()) / RAND_MAX - 0.5f; hA_f16[i] = __float2half(hA_f32[i]); }
    for (int i = 0; i < K * N; ++i) { hB_f32[i] = static_cast<float>(std::rand()) / RAND_MAX - 0.5f; hB_f16[i] = __float2half(hB_f32[i]); }

    // [H4-T35]
    std::puts("Computing CPU reference (may take tens of seconds)...");
    gemm_cpu_ref(hA_f32.data(), hB_f32.data(), hC_ref.data(), M, N, K);

    // [H4-T36] -- device memory --
    __half *dA, *dB; float *dC;
    CUDA_CHECK(cudaMalloc(&dA, sizeof(__half) * M * K));
    CUDA_CHECK(cudaMalloc(&dB, sizeof(__half) * K * N));
    CUDA_CHECK(cudaMalloc(&dC, sizeof(float)  * M * N));
    CUDA_CHECK(cudaMemcpy(dA, hA_f16.data(), sizeof(__half) * M * K, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(dB, hB_f16.data(), sizeof(__half) * K * N, cudaMemcpyHostToDevice));

    // [H4-T37] -- TODO [REQUIRED] step 1: build a TMA cuTensorMap (CUtensorMap) --
    //   CUtensorMap tmaA, tmaB;
    //   cuTensorMapEncodeTiled(&tmaA, CU_TENSOR_MAP_DATA_TYPE_FLOAT16,
    //       2, dA, {uint64_t(M), uint64_t(K)}, {uint64_t(M), uint64_t(K)},
    //       {uint32_t(BM), uint32_t(BK)},
    //       CU_TENSOR_MAP_INTERLEAVE_NONE, CU_TENSOR_MAP_SWIZZLE_128B,
    //       CU_TENSOR_MAP_L2_PROMOTION_NONE, CU_TENSOR_MAP_FLOAT_OOB_FILL_NONE);
    //   build tmaB the same way

    // [H4-T38] stub: pass nullptr; the kernel does not use it (compiles fine).
    const void* tmaA = nullptr;
    const void* tmaB = nullptr;

    // [H4-T39] -- launch config --
    dim3 block(256, 1, 1);
    dim3 grid((N + BN - 1) / BN, (M + BM - 1) / BM, 1);
    std::fprintf(stdout, "grid=(%u,%u,%u)  block=(%u,%u,%u)\n\n",
                 grid.x, grid.y, grid.z, block.x, block.y, block.z);

    // [H4-T40] -- TODO [REQUIRED] step 6: launch with cudaLaunchKernelEx and a cluster size --
    //   cudaLaunchConfig_t cfg{};
    //   cfg.gridDim  = grid;
    //   cfg.blockDim = block;
    //   cudaLaunchAttribute attrs[1];
    //   attrs[0].id = cudaLaunchAttributeClusterDimension;
    //   attrs[0].val.clusterDim = {2, 1, 1};
    //   cfg.attrs    = attrs;
    //   cfg.numAttrs = 1;
    //   CUDA_CHECK(cudaLaunchKernelEx(&cfg, gemm_hopper_warp_specialized,
    //                                 dA, dB, dC, tmaA, tmaB, M, N, K));

    CudaEventTimer timer;

    // [H4-T41] warmup
    for (int i = 0; i < WARMUP_ITERS; ++i)
        gemm_hopper_warp_specialized<<<grid, block>>>(dA, dB, dC, tmaA, tmaB, M, N, K);
    CUDA_CHECK(cudaDeviceSynchronize());

    // [H4-T42] timed loop
    timer.start();
    for (int i = 0; i < BENCH_ITERS; ++i)
        gemm_hopper_warp_specialized<<<grid, block>>>(dA, dB, dC, tmaA, tmaB, M, N, K);
    timer.stop();
    CUDA_CHECK_LAST();
    float ms = timer.elapsed_ms() / BENCH_ITERS;

    CUDA_CHECK(cudaMemcpy(hC_out.data(), dC, sizeof(float) * M * N, cudaMemcpyDeviceToHost));

    // ------------------------------------------------------------
    // [H4-T43] Correctness check.
    // ------------------------------------------------------------
    // [H4-T44]
    std::puts("\n-- Correctness check (FAIL expected at stub stage) --");
    check_result(hC_ref.data(), hC_out.data(), M, N, "hopper_warp_spec");

    // ------------------------------------------------------------
    // [H4-T45] Performance summary.
    // ------------------------------------------------------------
    double tflops = calc_tflops(M, N, K, ms);
    // [H4-T46] Hopper H100 SXM FP32 theoretical peak ~67 TFLOPS (TF32 ~989 TOPS).
    //          FP16 Tensor Core peak ~1979 TOPS; the measurement here is FP32-equivalent.
    double peak = 1.5; // [H4-T47] placeholder; should derive from cudaDeviceGetAttribute
    // [H4-T48]
    std::puts("\n-- Performance summary --");
    std::fprintf(stdout, "%-28s | %8s | %8s | %10s\n", "variant", "ms", "TFLOPS", "% peak");
    std::fprintf(stdout, "%-28s | %8.3f | %8.4f | %9.2f%%\n",
                 "hopper_warp_specialized", (double)ms, tflops, tflops / peak * 100.0);

    // ------------------------------------------------------------
    // [H4-T49] TODO [REQUIRED] step 7: validate cluster + warp specialization in Nsight Compute.
    //   ncu --set full --target-processes all ./H4_gemm_hopper_warp_specialized
    //   Inspect "SM Active Cycles", "Warp Occupancy", and TMA-related counters.
    // ------------------------------------------------------------

    // ------------------------------------------------------------
    // [H4-T50] TODO [ADVANCED] integrate split-K (multi-block K split + reduce).
    // ------------------------------------------------------------

    // ------------------------------------------------------------
    // [H4-T51] TODO [ADVANCED] sweep tile sizes (32x32, 64x64, 128x128).
    // ------------------------------------------------------------

    // ------------------------------------------------------------
    // [H4-T52] TODO [ADVANCED] Blackwell sm_100a comparison run.
    // ------------------------------------------------------------

    CUDA_CHECK(cudaFree(dA));
    CUDA_CHECK(cudaFree(dB));
    CUDA_CHECK(cudaFree(dC));

    // [H4-T53]
    std::puts("\n[H4] done.");
    return 0;
}
