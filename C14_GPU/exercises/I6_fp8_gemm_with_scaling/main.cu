// I6_fp8_gemm_with_scaling/main.cu
// [I6-T01] Exercise I6: FP8 E4M3 GEMM + Per-tensor / Per-block Scaling
//
// [I6-T02] Goals:
//   - FP8 E4M3: 4 exponent bits + 3 mantissa bits, range +/-240
//   - per-tensor scaling: one scale shared by entire matrix (amax scan)
//   - per-block scaling: one scale per 64x64 tile (more accurate)
//   - dequant-fused matmul: dequantize directly in the mainloop, no intermediate matrix
//   - FP8 vs FP16 baseline numerical drift analysis
//
// [I6-T03] Note: FP8 requires sm_89+ (Ada Lovelace) or sm_90a (Hopper).
//                On sm_80/sm_86 the program will skip FP8 tests at runtime.
//
// Build: cmake --build build --target I6_fp8_gemm_with_scaling
// Run:   ./I6_fp8_gemm_with_scaling

#include "common/cuda_check.cuh"
#include "common/timer.cuh"
#include "common/device_info.cuh"
#include "common/nvtx_range.cuh"

#include <cuda_runtime.h>
#include <cuda_fp16.h>

// [I6-T04] FP8 datatype header (CUDA 11.8+)
// [I6-T05] Available when compiling for sm_89+; protected here by preprocessor guards
#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ >= 890
#  include <cuda_fp8.h>
#  define HAVE_FP8 1
#else
// [I6-T06] host side: try include (CUDA 11.8+ toolkit provides it)
#  if __has_include(<cuda_fp8.h>)
#    include <cuda_fp8.h>
#    define HAVE_FP8 1
#  else
#    define HAVE_FP8 0
#  endif
#endif

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cfloat>
#include <cassert>
#include <vector>
#include <random>
#include <algorithm>

// ------------------------------------------------------------
// [I6-T07] Problem size
// ------------------------------------------------------------
constexpr int M_DIM   = 4096;
constexpr int N_DIM   = 4096;
constexpr int K_DIM   = 4096;
constexpr int TILE_SZ = 64;    // [I6-T08] per-block scaling tile size
constexpr float FP8_MAX = 240.0f; // [I6-T09] FP8 E4M3 max absolute value

// ------------------------------------------------------------
// [I6-T10] FP8 E4M3 type alias (guard-protected)
// ------------------------------------------------------------
#if HAVE_FP8
using fp8_e4m3_t = __nv_fp8_e4m3;
#else
// [I6-T11] non-FP8 build: use uint8_t placeholder, skip at runtime
using fp8_e4m3_t = uint8_t;
#endif

// ------------------------------------------------------------
// [I6-T12] Kernel 0: FP16 GEMM baseline (control, fully implemented)
// [I6-T13] C = A * B, FP16 inputs, FP32 accumulator, FP16 output
// ------------------------------------------------------------
__global__ void gemm_fp16_baseline(const __half* __restrict__ A,  // [M, K]
                                    const __half* __restrict__ B,  // [K, N]
                                    __half*       __restrict__ C,  // [M, N]
                                    int M, int N, int K)
{
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;
    if (row >= M || col >= N) return;

    float acc = 0.0f;
    for (int k = 0; k < K; ++k)
        acc += __half2float(A[row * K + k]) * __half2float(B[k * N + col]);
    C[row * N + col] = __float2half(acc);
}

// ------------------------------------------------------------
// [I6-T14] Kernel 1: per-tensor FP8 quantization (amax scan + quantized write)
//
// [I6-T15] TODO [REQUIRED] step 2: per-tensor quantization
//   scale = amax / FP8_MAX (FP8 E4M3 max = 240)
//   A_fp8[i] = clamp(A[i] / scale, -FP8_MAX, FP8_MAX)
// ------------------------------------------------------------
__global__ void quantize_fp8_per_tensor(const float*    __restrict__ in,    // [M, K] FP32
                                         fp8_e4m3_t*     __restrict__ out,   // [M, K] FP8
                                         float                        scale, // = amax / FP8_MAX
                                         int n)
{
#if HAVE_FP8
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= n) return;

    // [I6-T16] TODO [REQUIRED] step 2: FP32 -> FP8 quantization
    // float q = fmaxf(-FP8_MAX, fminf(FP8_MAX, in[i] / scale));
    // out[i] = (__nv_fp8_e4m3)q;

    out[i] = (__nv_fp8_e4m3)0.0f; // stub
#endif
}

// ------------------------------------------------------------
// [I6-T17] Kernel 2: per-block FP8 quantization (independent scale per TILE_SZ x TILE_SZ block)
//
// [I6-T18] TODO [REQUIRED] step 3: per-block quantization
//   for each [row_tile, col_tile] block:
//     block_amax = max(abs(A[r:r+TILE, c:c+TILE]))
//     scale[r_tile, c_tile] = block_amax / FP8_MAX
// ------------------------------------------------------------
__global__ void quantize_fp8_per_block(const float*    __restrict__ in,    // [M, K] FP32
                                        fp8_e4m3_t*     __restrict__ out,   // [M, K] FP8
                                        float*          __restrict__ scales, // [M/T, K/T]
                                        int M, int K, int tile)
{
#if HAVE_FP8
    // [I6-T19] each block handles one tile
    int tile_row = blockIdx.y;
    int tile_col = blockIdx.x;
    int r_start  = tile_row * tile;
    int c_start  = tile_col * tile;
    int tid      = threadIdx.x;

    // [I6-T20] TODO [REQUIRED] step 3: compute block amax + quantize
    // 1. scan tile to find block_amax (smem reduce)
    // 2. scale = block_amax / FP8_MAX
    // 3. write scales[tile_row * (K/tile) + tile_col] = scale
    // 4. quantize all elements in the block

    // [I6-T21] stub: skip
    (void)in; (void)out; (void)scales;
    (void)r_start; (void)c_start; (void)tid;
#endif
}

// ------------------------------------------------------------
// [I6-T22] Kernel 3: dequant-fused GEMM (per-tensor scaling)
//
// [I6-T23] C = A_fp8 * B_fp8 * scale_A * scale_B (dequantize directly in the mainloop)
// [I6-T24] Avoids writing the intermediate dequantized matrix
//
// [I6-T25] TODO [REQUIRED] step 4: dequant-fused GEMM
//   In the mainloop: read FP8 -> cast to FP32 -> multiply scale -> FP32 accumulate
//   Final write FP32 C (or cast to FP16)
// ------------------------------------------------------------
__global__ void gemm_fp8_dequant_fused(const fp8_e4m3_t* __restrict__ A_fp8,  // [M, K]
                                        const fp8_e4m3_t* __restrict__ B_fp8,  // [K, N]
                                        float*            __restrict__ C,      // [M, N] FP32
                                        float scale_A, float scale_B,
                                        int M, int N, int K)
{
#if HAVE_FP8
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;
    if (row >= M || col >= N) return;

    // [I6-T26] TODO [REQUIRED] step 4: dequant-fused mainloop
    // float acc = 0.0f;
    // for (int k = 0; k < K; ++k) {
    //     float a = (float)A_fp8[row * K + k] * scale_A;  // dequantize
    //     float b = (float)B_fp8[k  * N + col] * scale_B; // dequantize
    //     acc += a * b;
    // }
    // C[row * N + col] = acc;

    C[row * N + col] = 0.0f; // stub
#else
    // [I6-T27] non-FP8 build: output 0
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;
    if (row >= M || col >= N) return;
    C[row * N + col] = 0.0f;
#endif
}

// ------------------------------------------------------------
// [I6-T28] TODO [ADVANCED] delayed scaling (TransformerEngine style)
// ------------------------------------------------------------

// ------------------------------------------------------------
// [I6-T29] TODO [ADVANCED] per-block dequant-fused GEMM (read scale metadata per tile)
// ------------------------------------------------------------

// ------------------------------------------------------------
// [I6-T30] TODO [ADVANCED] asymmetric quantization (with zero point)
// ------------------------------------------------------------

// ------------------------------------------------------------
// [I6-T31] CPU reference: FP32 GEMM (full implementation, used for small-scale validation)
// ------------------------------------------------------------
static void cpu_gemm_fp32_ref(const std::vector<float>& A,
                               const std::vector<float>& B,
                               std::vector<float>&       C,
                               int M, int N, int K)
{
    for (int i = 0; i < M; ++i)
        for (int j = 0; j < N; ++j) {
            float acc = 0.0f;
            for (int k = 0; k < K; ++k)
                acc += A[i * K + k] * B[k * N + j];
            C[i * N + j] = acc;
        }
}

static float compute_amax(const float* data, int n)
{
    float mx = 0.0f;
    for (int i = 0; i < n; ++i) mx = std::max(mx, std::abs(data[i]));
    return mx;
}

static float rel_err(const float* a, const float* b, int n)
{
    float num = 0.0f, den = 0.0f;
    for (int i = 0; i < n; ++i) {
        num += (a[i] - b[i]) * (a[i] - b[i]);
        den += b[i] * b[i];
    }
    return (den > 0.0f) ? std::sqrt(num / den) : 0.0f;
}

// ------------------------------------------------------------
// [I6-T32] main
// ------------------------------------------------------------
int main()
{
    std::puts("[I6_fp8_gemm_with_scaling]");
    print_device_info(0);
    NVTX_RANGE("I6_fp8_gemm_with_scaling/main");

    // [I6-T33] -- runtime FP8 capability check (sm_89+ required) --
    {
        cudaDeviceProp prop{};
        CUDA_CHECK(cudaGetDeviceProperties(&prop, 0));
        int cc = prop.major * 10 + prop.minor;
        // [I6-T34]
        printf("Device Compute Capability: %d.%d (cc=%d)\n", prop.major, prop.minor, cc);

        if (cc < 89) {
            // [I6-T35]
            printf("\n[skip] FP8 E4M3 requires sm_89+ (Ada Lovelace or Hopper).\n");
            // [I6-T36]
            printf("Current device CC=%d does not satisfy this; program exits gracefully.\n", cc);
            // [I6-T37]
            printf("FP16 baseline still runs to validate the framework.\n\n");
            // [I6-T38] continue with FP16 baseline; FP8 sections are skipped
        } else {
            // [I6-T39]
            printf("FP8 support: confirmed (cc=%d >= 89)\n\n", cc);
        }
    }

    constexpr int M = M_DIM, N = N_DIM, K = K_DIM;
    // [I6-T40]
    printf("Problem size: M=%d  N=%d  K=%d\n", M, N, K);
    // [I6-T41]
    printf("Per-block tile size: %dx%d\n\n", TILE_SZ, TILE_SZ);

    // [I6-T42] -- TODO [REQUIRED] step 1: build FP32 matrices A, B (Gaussian) --
    // [I6-T43] Note: M*N*K = 4096^3 is huge; CPU ref uses small matrices
    // [I6-T44] Use small (64x64) matrices for CPU correctness, large for perf
    constexpr int Vs = 64;  // [I6-T45] verification matrix size
    std::mt19937 rng(42);
    std::normal_distribution<float> ndist(0.0f, 1.0f);

    std::vector<float> h_A_small(Vs * Vs), h_B_small(Vs * Vs), h_C_ref(Vs * Vs);
    for (auto& v : h_A_small) v = ndist(rng);
    for (auto& v : h_B_small) v = ndist(rng);
    cpu_gemm_fp32_ref(h_A_small, h_B_small, h_C_ref, Vs, Vs, Vs);

    // [I6-T46]
    printf("Small matrix validation (%dx%d): CPU FP32 GEMM reference computed.\n\n", Vs, Vs);

    // [I6-T47] -- per-tensor scale (FP32 -> FP8) --
    float amax_A = compute_amax(h_A_small.data(), Vs * Vs);
    float amax_B = compute_amax(h_B_small.data(), Vs * Vs);
    float scale_A = (amax_A > 0.0f) ? (amax_A / FP8_MAX) : 1.0f;
    float scale_B = (amax_B > 0.0f) ? (amax_B / FP8_MAX) : 1.0f;
    // [I6-T48]
    printf("Per-tensor scale: scale_A=%.4f  scale_B=%.4f\n",  scale_A, scale_B);
    // [I6-T49]
    printf("  amax_A=%.4f  amax_B=%.4f  FP8_MAX=%.1f\n\n", amax_A, amax_B, FP8_MAX);

    // [I6-T50] GPU allocation (large matrix M=N=K=4096 perf test)
    // [I6-T51] Note: 4096^2 * sizeof(fp16) = 32 MB per matrix; FP8 halves that
    float       *d_A_f32 = nullptr, *d_B_f32 = nullptr, *d_C_fp8 = nullptr;
    __half      *d_A_fp16 = nullptr, *d_B_fp16 = nullptr, *d_C_fp16 = nullptr;
    fp8_e4m3_t  *d_A_fp8 = nullptr, *d_B_fp8 = nullptr;
    float       *d_scales_A = nullptr;  // [I6-T52] per-block scales for A

    size_t mn_bytes   = (size_t)M * N;
    size_t mk_bytes   = (size_t)M * K;
    size_t kn_bytes   = (size_t)K * N;
    size_t tiles_A    = ((M + TILE_SZ - 1) / TILE_SZ) * ((K + TILE_SZ - 1) / TILE_SZ);

    CUDA_CHECK(cudaMalloc(&d_A_fp16,   mk_bytes   * sizeof(__half)));
    CUDA_CHECK(cudaMalloc(&d_B_fp16,   kn_bytes   * sizeof(__half)));
    CUDA_CHECK(cudaMalloc(&d_C_fp16,   mn_bytes   * sizeof(__half)));
    CUDA_CHECK(cudaMalloc(&d_A_fp8,    mk_bytes   * sizeof(fp8_e4m3_t)));
    CUDA_CHECK(cudaMalloc(&d_B_fp8,    kn_bytes   * sizeof(fp8_e4m3_t)));
    CUDA_CHECK(cudaMalloc(&d_C_fp8,    mn_bytes   * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_scales_A, tiles_A    * sizeof(float)));

    // [I6-T53] init FP16 data (random)
    std::vector<__half> h_A_fp16(mk_bytes), h_B_fp16(kn_bytes);
    for (auto& v : h_A_fp16) v = __float2half(ndist(rng));
    for (auto& v : h_B_fp16) v = __float2half(ndist(rng));
    CUDA_CHECK(cudaMemcpy(d_A_fp16, h_A_fp16.data(), mk_bytes * sizeof(__half), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_B_fp16, h_B_fp16.data(), kn_bytes * sizeof(__half), cudaMemcpyHostToDevice));

    // [I6-T54] launch config (16x16 thread tile)
    constexpr int BLK = 16;
    dim3 blk(BLK, BLK), grd((N + BLK - 1) / BLK, (M + BLK - 1) / BLK);
    // [I6-T55]
    printf("Launch config (FP16/FP8 GEMM): grid=(%d,%d,1)  block=(%d,%d,1)\n\n",
           grd.x, grd.y, blk.x, blk.y);

    CudaEventTimer timer;

    // ------------------------------------------------------------
    // [I6-T56] FP16 GEMM baseline
    // ------------------------------------------------------------
    {
        NVTX_RANGE("I6/fp16_baseline");
        CUDA_CHECK(cudaMemset(d_C_fp16, 0, mn_bytes * sizeof(__half)));
        timer.start();
        gemm_fp16_baseline<<<grd, blk>>>(d_A_fp16, d_B_fp16, d_C_fp16, M, N, K);
        CUDA_CHECK(cudaGetLastError());
        timer.stop();
        float ms = timer.elapsed_ms();
        float tflops = 2.0f * M * N * K / (ms * 1e-3f) / 1e12f;
        // [I6-T57]
        printf("[fp16_baseline]  %.3f ms  %.2f TFLOPS\n", ms, tflops);
    }

    // ------------------------------------------------------------
    // [I6-T58] FP8 dequant-fused GEMM (per-tensor scaling)
    // ------------------------------------------------------------
    {
        NVTX_RANGE("I6/fp8_dequant_fused");
        CUDA_CHECK(cudaMemset(d_C_fp8, 0, mn_bytes * sizeof(float)));
        timer.start();
        gemm_fp8_dequant_fused<<<grd, blk>>>(d_A_fp8, d_B_fp8, d_C_fp8,
                                              scale_A, scale_B, M, N, K);
        CUDA_CHECK(cudaGetLastError());
        timer.stop();
        float ms = timer.elapsed_ms();
        float tflops = 2.0f * M * N * K / (ms * 1e-3f) / 1e12f;
        // [I6-T59]
        printf("[fp8_dequant]    %.3f ms  %.2f TFLOPS  (stub: outputs all zeros)\n",
               ms, tflops);
    }

    // ------------------------------------------------------------
    // [I6-T60] Small-matrix accuracy analysis (CPU ref vs GPU FP8 stub)
    // ------------------------------------------------------------
    // [I6-T61]
    printf("\nSmall-matrix accuracy analysis (%dx%d, meaningful once stub is implemented):\n", Vs, Vs);
    // [I6-T62]
    printf("  per-tensor FP8 vs FP32 baseline: expect rel_err >= 1e-2 (acceptable range)\n");
    // [I6-T63]
    printf("  per-block  FP8 vs FP32 baseline: expect rel_err < 1e-3 (more accurate)\n");

    // ------------------------------------------------------------
    // [I6-T64] TODO [REQUIRED] step 5: compare per-tensor vs per-block FP8 relative error
    // [I6-T65] TODO [REQUIRED] step 6: Nsight Compute observe dequant-fused register footprint and throughput
    // ------------------------------------------------------------
    // [I6-T66]
    printf("\nAcceptance (after stub is implemented):\n");
    // [I6-T67]
    printf("  per-tensor FP8 rel_err >= 1e-2 (FP8 precision loss is large, expected)\n");
    // [I6-T68]
    printf("  per-block  FP8 rel_err <  1e-3 (clearly better than per-tensor)\n");
    // [I6-T69]
    printf("  dequant-fused throughput: no significant drop vs separated version\n");

    CUDA_CHECK(cudaFree(d_A_fp16));
    CUDA_CHECK(cudaFree(d_B_fp16));
    CUDA_CHECK(cudaFree(d_C_fp16));
    CUDA_CHECK(cudaFree(d_A_fp8));
    CUDA_CHECK(cudaFree(d_B_fp8));
    CUDA_CHECK(cudaFree(d_C_fp8));
    CUDA_CHECK(cudaFree(d_scales_A));

    // [I6-T70]
    printf("\n[I6] done. Use ncu --set full ./I6_fp8_gemm_with_scaling to inspect registers.\n");
    return 0;
}
