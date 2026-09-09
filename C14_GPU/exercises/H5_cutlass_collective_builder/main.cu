// H5_cutlass_collective_builder/main.cu
// ============================================================
// [H5-T01] Exercise H5: CUTLASS 3.x CollectiveBuilder + Device API
//
// [H5-T02] Goal:
//   Use the CUTLASS 3.x Device API and CollectiveBuilder to auto-
//   generate a Hopper warp-specialized GEMM kernel. Understand the
//   five-layer hierarchy: Device / Kernel / Collective / Tiled
//   MMA+Copy / Atom. Compare TFLOPS against the H4 hand-written version.
//
// [H5-T03] Build requirements: sm_90a (Hopper CollectiveBuilder),
//          CUDA 13.x, CUTLASS 3.x headers
// [H5-T04] Dependency: cutlass::headers (provided by CutlassSetup.cmake)
// ============================================================

// ------------------------------------------------------------
// [H5-T05] CUTLASS core headers
// ------------------------------------------------------------
#include "cutlass/cutlass.h"
#include "cutlass/gemm/device/gemm_universal_adapter.h"
#include "cutlass/gemm/kernel/gemm_universal.hpp"
#include "cutlass/gemm/collective/collective_builder.hpp"
#include "cutlass/epilogue/collective/default_epilogue.hpp"
#include "cutlass/epilogue/collective/collective_builder.hpp"
#include "cutlass/gemm/dispatch_policy.hpp"
#include "cutlass/tensor_ref.h"
#include "cutlass/util/host_tensor.h"
#include "cutlass/util/tensor_view_io.h"
#include "cutlass/util/reference/host/tensor_fill.h"
#include "cutlass/util/reference/host/tensor_compare.h"
#include "cutlass/util/reference/host/gemm.h"

// ------------------------------------------------------------
// [H5-T06] Exercise common headers
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
// [H5-T07] Hopper runtime detection
// ------------------------------------------------------------
#define HOPPER_SKIP_IF_UNSUPPORTED(device)                         \
    do {                                                           \
        if (!has_hopper_features(device)) {                        \
            std::puts("[H5] Current GPU lacks Hopper (sm_90a); skipping."); \
            return 0;                                              \
        }                                                          \
    } while (0)

// ------------------------------------------------------------
// [H5-T08] Hyperparameters
// ------------------------------------------------------------
static constexpr int WARMUP_ITERS = 3;
static constexpr int BENCH_ITERS  = 10;

// ------------------------------------------------------------
// [H5-T09] CUTLASS type aliases
//
// [H5-T10] TODO [REQUIRED] step 3: produce CollectiveMainloop via CollectiveBuilder.
//   See CUTLASS examples/48_hopper_warp_specialized_gemm/.
//
// The aliases below illustrate how the five-layer hierarchy is composed;
// the student fills in the CollectiveBuilder template arguments.
// ------------------------------------------------------------

// [H5-T11] data types
using ElementA     = cutlass::half_t;        // [H5-T12] FP16
using ElementB     = cutlass::half_t;        // [H5-T13] FP16
using ElementC     = float;                  // [H5-T14] FP32 output
using ElementAccum = float;                  // [H5-T15] FP32 accumulator

// [H5-T16] layouts (A row-major, B column-major: CUTLASS NT GEMM convention)
using LayoutA = cutlass::layout::RowMajor;
using LayoutB = cutlass::layout::ColumnMajor;
using LayoutC = cutlass::layout::RowMajor;

// [H5-T17] alignment (FP16: 128-bit = 8 elements)
static constexpr int AlignA = 8;
static constexpr int AlignB = 8;
static constexpr int AlignC = 4;

// ------------------------------------------------------------
// [H5-T18] TODO [REQUIRED] step 3: CollectiveMainloop
//
// using CollectiveMainloop =
//   typename cutlass::gemm::collective::CollectiveBuilder<
//     cutlass::arch::Sm90,                          // target arch
//     cutlass::arch::OpClassTensorOp,               // Tensor Core
//     ElementA, LayoutA, AlignA,
//     ElementB, LayoutB, AlignB,
//     ElementAccum,
//     cutlass::gemm::collective::StageCountAuto,    // auto pipeline stage count
//     cutlass::gemm::collective::KernelScheduleAuto // auto schedule policy
//   >::CollectiveOp;
//
// Replace KernelScheduleAuto with:
//   KernelTmaWarpSpecializedCooperative  -> cooperative warp specialization
//   KernelTmaWarpSpecializedPingpong     -> ping-pong (producer-consumer alternation)
// ------------------------------------------------------------

// [H5-T19] stub: placeholder so the file compiles; the student replaces it
//          with a real CollectiveBuilder. (If CUTLASS headers are unavailable
//          the whole file fails to compile -- expected behavior.)
//
// For now a minimal sm_80-compatible config could serve as a stub to let
// the file build on any GPU; the student installs the Hopper-specific
// configuration when implementing the task.

// [H5-T20] TODO [REQUIRED] step 3 (student region):
//   Uncomment the real CollectiveBuilder below and remove the stub.
//
// using CollectiveMainloop = typename cutlass::gemm::collective::CollectiveBuilder<
//     cutlass::arch::Sm90,
//     cutlass::arch::OpClassTensorOp,
//     ElementA, LayoutA, AlignA,
//     ElementB, LayoutB, AlignB,
//     ElementAccum,
//     cutlass::gemm::Shape<_128, _128, _64>,
//     cutlass::gemm::Shape<_1, _2, _1>,
//     cutlass::gemm::collective::StageCountAuto,
//     cutlass::gemm::collective::KernelScheduleAuto
// >::CollectiveOp;

// ------------------------------------------------------------
// [H5-T21] TODO [REQUIRED] step 4: KernelSchedule + CollectiveEpilogue
//
// using CollectiveEpilogue = typename cutlass::epilogue::collective::CollectiveBuilder<
//     cutlass::arch::Sm90,
//     cutlass::arch::OpClassTensorOp,
//     cutlass::gemm::Shape<_128, _128, _64>,
//     cutlass::gemm::Shape<_1, _2, _1>,
//     cutlass::epilogue::collective::EpilogueTileAuto,
//     ElementAccum, ElementAccum,
//     ElementC, LayoutC, AlignC,
//     ElementC, LayoutC, AlignC,
//     cutlass::epilogue::collective::EpilogueScheduleAuto
// >::CollectiveOp;
// ------------------------------------------------------------

// ------------------------------------------------------------
// [H5-T22] TODO [REQUIRED] step 5: GemmKernel + GemmUniversalAdapter
//
// using GemmKernel = cutlass::gemm::kernel::GemmUniversal<
//     cutlass::gemm::Shape<int, int, int, int>,
//     CollectiveMainloop,
//     CollectiveEpilogue
// >;
// using Gemm = cutlass::gemm::device::GemmUniversalAdapter<GemmKernel>;
// ------------------------------------------------------------

// ------------------------------------------------------------
// [H5-T23] CPU reference.
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
// [H5-T24] main
// ------------------------------------------------------------
int main()
{
    std::puts("[H5_cutlass_collective_builder]");
    print_device_info();

    NVTX_RANGE("H5_cutlass_collective_builder/main");

    // [H5-T25] -- Hopper runtime check --
    HOPPER_SKIP_IF_UNSUPPORTED(0);

    // ------------------------------------------------------------
    // [H5-T26] TODO [REQUIRED] step 2: problem size
    // ------------------------------------------------------------
    const int M = 2048, N = 2048, K = 2048;
    // [H5-T27]
    std::fprintf(stdout, "Problem size: M=%d  N=%d  K=%d\n\n", M, N, K);

    // [H5-T28] -- host memory (FP32 for CPU ref, FP16 for CUTLASS) --
    std::vector<float>  hA_f32(M * K), hB_f32(K * N), hC_ref(M * N, 0.0f);
    std::vector<float>  hC_out(M * N, 0.0f);
    std::vector<cutlass::half_t> hA_f16(M * K), hB_f16(K * N);

    std::srand(42);
    for (int i = 0; i < M * K; ++i) {
        hA_f32[i] = static_cast<float>(std::rand()) / RAND_MAX - 0.5f;
        hA_f16[i] = cutlass::half_t(hA_f32[i]);
    }
    for (int i = 0; i < K * N; ++i) {
        hB_f32[i] = static_cast<float>(std::rand()) / RAND_MAX - 0.5f;
        hB_f16[i] = cutlass::half_t(hB_f32[i]);
    }

    // [H5-T29]
    std::puts("Computing CPU reference...");
    gemm_cpu_ref(hA_f32.data(), hB_f32.data(), hC_ref.data(), M, N, K);

    // [H5-T30] -- device memory --
    cutlass::half_t *dA, *dB;
    float           *dC;
    CUDA_CHECK(cudaMalloc(&dA, sizeof(cutlass::half_t) * M * K));
    CUDA_CHECK(cudaMalloc(&dB, sizeof(cutlass::half_t) * K * N));
    CUDA_CHECK(cudaMalloc(&dC, sizeof(float)           * M * N));
    CUDA_CHECK(cudaMemcpy(dA, hA_f16.data(), sizeof(cutlass::half_t) * M * K, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(dB, hB_f16.data(), sizeof(cutlass::half_t) * K * N, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemset(dC, 0, sizeof(float) * M * N));

    // ------------------------------------------------------------
    // [H5-T31] TODO [REQUIRED] step 5: build CUTLASS Gemm arguments and run.
    //
    // typename Gemm::Arguments args{
    //     cutlass::gemm::GemmUniversalMode::kGemm,
    //     {M, N, K},
    //     // A/B/C/D pointers and strides
    //     {dA, K, dB, K, dC, N, dC, N},
    //     // alpha / beta
    //     {1.0f, 0.0f}
    // };
    //
    // Gemm gemm_op;
    // size_t workspace_size = Gemm::get_workspace_size(args);
    // void* workspace = nullptr;
    // if (workspace_size > 0)
    //     CUDA_CHECK(cudaMalloc(&workspace, workspace_size));
    //
    // cutlass::Status status = gemm_op.initialize(args, workspace);
    // if (status != cutlass::Status::kSuccess) {
    //     std::fprintf(stderr, "CUTLASS initialize failed: %s\n",
    //                  cutlassGetStatusString(status));
    //     return 1;
    // }
    //
    // // warmup
    // for (int i = 0; i < WARMUP_ITERS; ++i) gemm_op.run();
    // CUDA_CHECK(cudaDeviceSynchronize());
    //
    // // timed loop
    // CudaEventTimer timer;
    // timer.start();
    // for (int i = 0; i < BENCH_ITERS; ++i) gemm_op.run();
    // timer.stop();
    // float ms = timer.elapsed_ms() / BENCH_ITERS;
    //
    // if (workspace) cudaFree(workspace);
    // ------------------------------------------------------------

    // [H5-T32] stub: leave dC zero via cudaMemset (kernel not implemented)
    float ms_stub = 1.0f; // [H5-T33] placeholder
    // [H5-T34]
    std::puts("  [stub] CUTLASS kernel not implemented; output is zero (CPU check FAIL is expected).");

    CUDA_CHECK(cudaMemcpy(hC_out.data(), dC, sizeof(float) * M * N, cudaMemcpyDeviceToHost));

    // ------------------------------------------------------------
    // [H5-T35] Correctness check.
    // ------------------------------------------------------------
    // [H5-T36]
    std::puts("\n-- Correctness check (FAIL expected at stub stage) --");
    check_result(hC_ref.data(), hC_out.data(), M, N, "cutlass_collective");

    // ------------------------------------------------------------
    // [H5-T37] Performance summary.
    // ------------------------------------------------------------
    double tflops = calc_tflops(M, N, K, ms_stub);
    double peak   = 1.5;
    // [H5-T38]
    std::puts("\n-- Performance summary --");
    std::fprintf(stdout, "%-28s | %8s | %8s | %10s\n", "variant", "ms", "TFLOPS", "% peak");
    std::fprintf(stdout, "%-28s | %8.3f | %8.4f | %9.2f%%\n",
                 "cutlass_collective_builder", (double)ms_stub, tflops, tflops / peak * 100.0);
    // [H5-T39]
    std::puts("  (numbers above are stub placeholders, not real performance)");

    // ------------------------------------------------------------
    // [H5-T40] TODO [REQUIRED] step 6: measure throughput and compare against the H4 hand-written version.
    // ------------------------------------------------------------

    // ------------------------------------------------------------
    // [H5-T41] TODO [ADVANCED] swap CollectiveBuilder configurations (tile size / schedule).
    // ------------------------------------------------------------

    // ------------------------------------------------------------
    // [H5-T42] TODO [ADVANCED] run on sm_80 and sm_90a separately and inspect the PTX differences.
    // ------------------------------------------------------------

    // ------------------------------------------------------------
    // [H5-T43] TODO [ADVANCED] try split-K mode (GemmUniversalMode::kGemmSplitKParallel).
    // ------------------------------------------------------------

    CUDA_CHECK(cudaFree(dA));
    CUDA_CHECK(cudaFree(dB));
    CUDA_CHECK(cudaFree(dC));

    // [H5-T44]
    std::puts("\n[H5] done.");
    (void)WARMUP_ITERS; (void)BENCH_ITERS;
    return 0;
}
