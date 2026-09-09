// I3_gelu_silu_fused/main.cu
// [I3-T01] Exercise I3: GELU / SiLU activations and epilogue fusion
//
// [I3-T02] Goals:
//   - tanh-approx GELU (matches PyTorch approximate='tanh')
//   - exact GELU (erff implementation)
//   - SiLU/Swish (x * sigmoid(x))
//   - throughput comparison: three independent kernels vs fused epilogue
//   - register pressure vs occupancy trade-off
//
// Build: cmake --build build --target I3_gelu_silu_fused
// Run:   ./I3_gelu_silu_fused

#include "common/cuda_check.cuh"
#include "common/timer.cuh"
#include "common/device_info.cuh"
#include "common/nvtx_range.cuh"

#include <cuda_runtime.h>
#include <cuda_fp16.h>

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cassert>
#include <vector>
#include <random>
#include <algorithm>

// ------------------------------------------------------------
// [I3-T03] Problem size
// ------------------------------------------------------------
constexpr int N         = 1 << 20;  // [I3-T04] number of elements (~1M, covers [1..100] tiles)
constexpr int BLOCK_DIM = 256;

// ------------------------------------------------------------
// [I3-T05] tanh-approx GELU coefficients (PyTorch approximate='tanh')
// 0.5 * x * (1 + tanh(sqrt(2/pi) * (x + 0.044715 * x^3)))
// ------------------------------------------------------------
constexpr float GELU_COEF_A = 0.7978845608028654f;  // sqrt(2/pi)
constexpr float GELU_COEF_B = 0.044715f;

// ------------------------------------------------------------
// [I3-T06] Device function: tanh-approx GELU
// ------------------------------------------------------------
__device__ __forceinline__ float gelu_tanh(float x)
{
    // [I3-T07] TODO [REQUIRED] step 2: implement tanh-approx GELU
    // float t = tanhf(GELU_COEF_A * (x + GELU_COEF_B * x * x * x));
    // return 0.5f * x * (1.0f + t);
    return 0.0f; // stub
}

// ------------------------------------------------------------
// [I3-T08] Device function: exact GELU (erff)
// ------------------------------------------------------------
__device__ __forceinline__ float gelu_exact(float x)
{
    // [I3-T09] TODO [REQUIRED] step 3: implement exact GELU
    // Formula: 0.5 * x * (1 + erf(x / sqrt(2)))
    // Note: erff returns erf(x); erfcf returns 1 - erf(x)
    // return 0.5f * x * (1.0f + erff(x * 0.7071067811865476f));
    return 0.0f; // stub
}

// ------------------------------------------------------------
// [I3-T10] Device function: SiLU (numerically stable version)
// ------------------------------------------------------------
__device__ __forceinline__ float silu(float x)
{
    // [I3-T11] TODO [REQUIRED] step 4: implement SiLU = x * sigmoid(x)
    // Numerically stable sigmoid: split positive/negative branches to avoid exp overflow
    // float sig = (x >= 0.0f) ? (1.0f / (1.0f + expf(-x)))
    //                         : (expf(x) / (1.0f + expf(x)));
    // return x * sig;
    return 0.0f; // stub
}

// ------------------------------------------------------------
// [I3-T12] Kernel 1: standalone tanh GELU kernel
// ------------------------------------------------------------
__global__ void kernel_gelu_tanh(const float* __restrict__ in,
                                  float*       __restrict__ out,
                                  int n)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) out[i] = gelu_tanh(in[i]);
}

// ------------------------------------------------------------
// [I3-T13] Kernel 2: standalone exact GELU kernel
// ------------------------------------------------------------
__global__ void kernel_gelu_exact(const float* __restrict__ in,
                                   float*       __restrict__ out,
                                   int n)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) out[i] = gelu_exact(in[i]);
}

// ------------------------------------------------------------
// [I3-T14] Kernel 3: standalone SiLU kernel
// ------------------------------------------------------------
__global__ void kernel_silu(const float* __restrict__ in,
                             float*       __restrict__ out,
                             int n)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) out[i] = silu(in[i]);
}

// ------------------------------------------------------------
// [I3-T15] Kernel 4: fused epilogue kernel
// [I3-T16] Simulates GEMM + bias-add followed by three activations (three output arrays)
//
// [I3-T17] TODO [REQUIRED] step 5: in one kernel apply three activations to the same input
// [I3-T18] Observation: register footprint of each activation (use Nsight Compute to inspect)
// ------------------------------------------------------------
__global__ void kernel_fused_epilogue(const float* __restrict__ in,
                                       float*       __restrict__ out_gelu_tanh,
                                       float*       __restrict__ out_gelu_exact,
                                       float*       __restrict__ out_silu,
                                       int n)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= n) return;

    // [I3-T19] TODO [REQUIRED] step 5: read in[i] once, write three activation results
    // float x = in[i];
    // out_gelu_tanh[i]  = gelu_tanh(x);
    // out_gelu_exact[i] = gelu_exact(x);
    // out_silu[i]       = silu(x);

    // [I3-T20] stub: output 0
    out_gelu_tanh[i]  = 0.0f;
    out_gelu_exact[i] = 0.0f;
    out_silu[i]       = 0.0f;
}

// ------------------------------------------------------------
// [I3-T21] TODO [ADVANCED] BF16 vector intrinsics version (sm_90a __nv_bfloat162)
// ------------------------------------------------------------

// ------------------------------------------------------------
// [I3-T22] TODO [ADVANCED] fused GEMM + GELU epilogue (combined with module H mma.sync)
// ------------------------------------------------------------

// ------------------------------------------------------------
// [I3-T23] TODO [ADVANCED] vary blockDim to observe register spill vs performance trade-off
// ------------------------------------------------------------

// ------------------------------------------------------------
// [I3-T24] CPU reference (full implementation)
// ------------------------------------------------------------
static float cpu_gelu_tanh(float x)
{
    float t = std::tanh(GELU_COEF_A * (x + GELU_COEF_B * x * x * x));
    return 0.5f * x * (1.0f + t);
}

static float cpu_gelu_exact(float x)
{
    return 0.5f * x * (1.0f + std::erf(x * 0.7071067811865476f));
}

static float cpu_silu(float x)
{
    return x / (1.0f + std::exp(-x));
}

static float max_abs_diff(const float* a, const float* b, int n)
{
    float d = 0.0f;
    for (int i = 0; i < n; ++i) d = std::max(d, std::abs(a[i] - b[i]));
    return d;
}

// ------------------------------------------------------------
// [I3-T25] main
// ------------------------------------------------------------
int main()
{
    std::puts("[I3_gelu_silu_fused]");
    print_device_info(0);
    NVTX_RANGE("I3_gelu_silu_fused/main");

    // [I3-T26] -- TODO [REQUIRED] step 1: build Gaussian N(0, 1) input vector --
    // [I3-T27] init already provided
    std::mt19937 rng(42);
    std::normal_distribution<float> ndist(0.0f, 1.0f);

    std::vector<float> h_in(N);
    for (auto& v : h_in) v = ndist(rng);

    // [I3-T28]
    printf("Problem size: N=%d  (%.1f M elements)\n\n", N, N / 1e6f);

    // [I3-T29] CPU reference
    std::vector<float> h_ref_gt(N), h_ref_ge(N), h_ref_sl(N);
    for (int i = 0; i < N; ++i) {
        h_ref_gt[i] = cpu_gelu_tanh (h_in[i]);
        h_ref_ge[i] = cpu_gelu_exact(h_in[i]);
        h_ref_sl[i] = cpu_silu      (h_in[i]);
    }

    // [I3-T30] GPU allocation
    float *d_in = nullptr, *d_gt = nullptr, *d_ge = nullptr, *d_sl = nullptr;
    CUDA_CHECK(cudaMalloc(&d_in, N * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_gt, N * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_ge, N * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_sl, N * sizeof(float)));
    CUDA_CHECK(cudaMemcpy(d_in, h_in.data(), N * sizeof(float), cudaMemcpyHostToDevice));

    int grid = (N + BLOCK_DIM - 1) / BLOCK_DIM;
    // [I3-T31]
    printf("Launch config: grid=%d  block=%d\n\n", grid, BLOCK_DIM);

    CudaEventTimer timer;
    std::vector<float> h_out(N);

    auto report = [&](const char* name, const float* d_out,
                      const std::vector<float>& ref, float ms) {
        CUDA_CHECK(cudaMemcpy(h_out.data(), d_out, N * sizeof(float), cudaMemcpyDeviceToHost));
        float diff = max_abs_diff(ref.data(), h_out.data(), N);
        float bw   = 2.f * N * sizeof(float) / (ms * 1e-3f) / 1e9f;
        // [I3-T32]
        printf("[%s]  %.3f ms  max_err=%.2e  effective_bw=%.1f GB/s\n",
               name, ms, diff, bw);
    };

    // ------------------------------------------------------------
    // [I3-T33] Standalone kernel tests
    // ------------------------------------------------------------
    {
        NVTX_RANGE("I3/gelu_tanh");
        CUDA_CHECK(cudaMemset(d_gt, 0, N * sizeof(float)));
        timer.start();
        kernel_gelu_tanh<<<grid, BLOCK_DIM>>>(d_in, d_gt, N);
        CUDA_CHECK(cudaGetLastError());
        timer.stop();
        report("gelu_tanh ", d_gt, h_ref_gt, timer.elapsed_ms());
    }
    {
        NVTX_RANGE("I3/gelu_exact");
        CUDA_CHECK(cudaMemset(d_ge, 0, N * sizeof(float)));
        timer.start();
        kernel_gelu_exact<<<grid, BLOCK_DIM>>>(d_in, d_ge, N);
        CUDA_CHECK(cudaGetLastError());
        timer.stop();
        report("gelu_exact", d_ge, h_ref_ge, timer.elapsed_ms());
    }
    {
        NVTX_RANGE("I3/silu");
        CUDA_CHECK(cudaMemset(d_sl, 0, N * sizeof(float)));
        timer.start();
        kernel_silu<<<grid, BLOCK_DIM>>>(d_in, d_sl, N);
        CUDA_CHECK(cudaGetLastError());
        timer.stop();
        report("silu      ", d_sl, h_ref_sl, timer.elapsed_ms());
    }

    // ------------------------------------------------------------
    // [I3-T34] Fused epilogue kernel
    // ------------------------------------------------------------
    {
        NVTX_RANGE("I3/fused_epilogue");
        CUDA_CHECK(cudaMemset(d_gt, 0, N * sizeof(float)));
        CUDA_CHECK(cudaMemset(d_ge, 0, N * sizeof(float)));
        CUDA_CHECK(cudaMemset(d_sl, 0, N * sizeof(float)));
        timer.start();
        kernel_fused_epilogue<<<grid, BLOCK_DIM>>>(d_in, d_gt, d_ge, d_sl, N);
        CUDA_CHECK(cudaGetLastError());
        timer.stop();
        float ms = timer.elapsed_ms();
        // [I3-T35]
        printf("[fused_epilogue]  %.3f ms  (read 1x + write 3x = 4x BW, stub outputs zeros)\n", ms);
    }

    // ------------------------------------------------------------
    // [I3-T36] TODO [REQUIRED] step 6: tanh-approx vs exact GELU element-wise diff < 1e-2 (in [-5, 5])
    // ------------------------------------------------------------
    {
        // [I3-T37] compare two GELU variants on [-5, 5]
        float max_diff_gelu = 0.0f;
        for (int i = 0; i < N; ++i) {
            if (h_in[i] >= -5.0f && h_in[i] <= 5.0f)
                max_diff_gelu = std::max(max_diff_gelu,
                    std::abs(h_ref_gt[i] - h_ref_ge[i]));
        }
        // [I3-T38]
        printf("\ntanh-approx vs exact GELU (CPU ref, [-5,5] range): max_diff=%.2e  "
               "(acceptance: < 1e-2)\n", max_diff_gelu);
    }

    // [I3-T39]
    printf("\nAcceptance: after stub is filled, gelu_tanh max_err < 1e-2, silu < 1e-4\n");

    CUDA_CHECK(cudaFree(d_in));
    CUDA_CHECK(cudaFree(d_gt));
    CUDA_CHECK(cudaFree(d_ge));
    CUDA_CHECK(cudaFree(d_sl));

    // [I3-T40]
    printf("\n[I3] done. Use ncu --set full ./I3_gelu_silu_fused to inspect register usage.\n");
    return 0;
}
