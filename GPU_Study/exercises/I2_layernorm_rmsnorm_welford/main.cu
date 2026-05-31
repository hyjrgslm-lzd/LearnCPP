// I2_layernorm_rmsnorm_welford/main.cu
// [I2-T01] Exercise I2: LayerNorm / RMSNorm with Welford one-pass algorithm
//
// [I2-T02] Goals:
//   - Welford one-pass: simultaneously compute mean + M2 (second moment) in a single sweep
//   - distributed Welford: merge stats across warps/blocks (Pebay 2008 formula)
//   - FP16 input + FP32 accumulator: avoid mean drift
//   - LayerNorm vs RMSNorm variants (selected via use_rmsnorm flag)
//
// Build: cmake --build build --target I2_layernorm_rmsnorm_welford
// Run:   ./I2_layernorm_rmsnorm_welford

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
// [I2-T03] Problem size
// ------------------------------------------------------------
constexpr int B         = 32;    // [I2-T04] batch (rows)
constexpr int H         = 4096;  // [I2-T05] hidden dim (length per row)
constexpr int WARP_SIZE = 32;
constexpr int NUM_WARPS = 8;     // [I2-T06] blockDim.x = 256 = 8 warps
constexpr float EPS     = 1e-5f;

// ------------------------------------------------------------
// [I2-T07] Device helper: warp reduce sum (FP32)
// ------------------------------------------------------------
__device__ __forceinline__ float warp_reduce_sum(float val)
{
    for (int offset = 16; offset > 0; offset >>= 1)
        val += __shfl_down_sync(0xffffffff, val, offset);
    return val;
}

// ------------------------------------------------------------
// [I2-T08] Kernel 1: Naive LayerNorm (two-pass: mean first, then var)
// [I2-T09] Baseline used to validate the Welford version
// ------------------------------------------------------------
__global__ void layernorm_naive(const __half* __restrict__ in,
                                 float*        __restrict__ out,
                                 const float*  __restrict__ gamma,
                                 const float*  __restrict__ beta,
                                 int rows, int cols)
{
    // [I2-T10] one block per row (blockDim.x = 256)
    int row  = blockIdx.x;
    int tid  = threadIdx.x;
    int lane = tid % WARP_SIZE;
    int wid  = tid / WARP_SIZE;
    if (row >= rows) return;

    __shared__ float smem_vals[NUM_WARPS]; // [I2-T11] partial sum per warp

    const __half* row_in  = in  + row * cols;
    float*        row_out = out + row * cols;

    // [I2-T12] Pass 1: compute mean (FP32 accumulator)
    float sum = 0.0f;
    for (int i = tid; i < cols; i += blockDim.x)
        sum += __half2float(row_in[i]);
    sum = warp_reduce_sum(sum);
    if (lane == 0) smem_vals[wid] = sum;
    __syncthreads();

    float total_sum = 0.0f;
    if (wid == 0) {
        float v = (lane < NUM_WARPS) ? smem_vals[lane] : 0.0f;
        total_sum = warp_reduce_sum(v);
    }
    total_sum = __shfl_sync(0xffffffff, total_sum, 0);
    // [I2-T13] broadcast to all threads via smem
    __shared__ float s_mean;
    if (tid == 0) s_mean = total_sum / cols;
    __syncthreads();
    float mean = s_mean;

    // [I2-T14] Pass 2: compute var
    float var_sum = 0.0f;
    for (int i = tid; i < cols; i += blockDim.x) {
        float x = __half2float(row_in[i]) - mean;
        var_sum += x * x;
    }
    var_sum = warp_reduce_sum(var_sum);
    if (lane == 0) smem_vals[wid] = var_sum;
    __syncthreads();

    float total_var = 0.0f;
    if (wid == 0) {
        float v = (lane < NUM_WARPS) ? smem_vals[lane] : 0.0f;
        total_var = warp_reduce_sum(v);
    }
    total_var = __shfl_sync(0xffffffff, total_var, 0);
    __shared__ float s_inv_std;
    if (tid == 0) s_inv_std = rsqrtf(total_var / cols + EPS);
    __syncthreads();
    float inv_std = s_inv_std;

    // [I2-T15] write out: (x - mean) * inv_std * gamma + beta
    for (int i = tid; i < cols; i += blockDim.x) {
        float x = __half2float(row_in[i]);
        row_out[i] = (x - mean) * inv_std * gamma[i] + beta[i];
    }
}

// ------------------------------------------------------------
// [I2-T16] Kernel 2: Online Welford LayerNorm / RMSNorm (single-pass)
//
// [I2-T17] TODO [REQUIRED] step 2: warp-level Welford
//   Initialize mean=0, M2=0, count=0 (FP32 accumulators)
//   per element: delta = x - mean; mean += delta/(count+1); delta2 = x - mean; M2 += delta*delta2
// [I2-T18] TODO [REQUIRED] step 3: block-level Welford reduce (distributed Pebay merge across warps)
// [I2-T19] TODO [REQUIRED] step 4: gamma/beta broadcast apply (per-feature scale + shift)
// [I2-T20] TODO [REQUIRED] step 5: use_rmsnorm branch (skip mean, use RMS = sqrt(mean(x^2)))
// ------------------------------------------------------------
__global__ void layernorm_welford(const __half* __restrict__ in,
                                   float*        __restrict__ out,
                                   const float*  __restrict__ gamma,
                                   const float*  __restrict__ beta,
                                   int rows, int cols,
                                   bool use_rmsnorm)
{
    int row = blockIdx.x;
    int tid = threadIdx.x;
    if (row >= rows) return;

    const __half* row_in  = in  + row * cols;
    float*        row_out = out + row * cols;

    // [I2-T21] -- TODO [REQUIRED] step 2: warp-level Welford --
    // float welf_mean = 0.0f, welf_M2 = 0.0f;
    // int   welf_count = 0;
    // for (int i = tid; i < cols; i += blockDim.x) {
    //     float x = __half2float(row_in[i]);
    //     welf_count++;
    //     float delta  = x - welf_mean;
    //     welf_mean   += delta / welf_count;
    //     float delta2 = x - welf_mean;   // NOTE: use new mean, not old mean
    //     welf_M2     += delta * delta2;
    // }
    //
    // [I2-T22] -- TODO [REQUIRED] step 3: cross-warp merge (Pebay 2008 distributed variance) --
    // n_total = n_a + n_b
    // delta   = b_mean - a_mean
    // mean_c  = a_mean + delta * n_b / n_total
    // M2_c    = M2_a + M2_b + delta^2 * n_a * n_b / n_total
    //
    // [I2-T23] -- TODO [REQUIRED] step 4/5: normalize and write out --
    // float var     = welf_M2 / cols;          // LayerNorm
    // float rms_var = ...;                     // RMSNorm: mean(x^2)
    // float inv_std = rsqrtf(var + EPS);       // or rms_var
    // float norm    = use_rmsnorm ? (x / sqrt(rms_var + EPS))
    //                             : ((x - welf_mean) * inv_std);
    // row_out[i] = norm * gamma[i] + beta[i]; // (RMSNorm beta is typically 0)

    // [I2-T24] stub: output 0
    for (int i = tid; i < cols; i += blockDim.x)
        row_out[i] = 0.0f;
}

// ------------------------------------------------------------
// [I2-T25] TODO [ADVANCED] cudaMemsetAsync to init stats buffer, avoid CPU sync
// ------------------------------------------------------------

// ------------------------------------------------------------
// [I2-T26] TODO [ADVANCED] gamma/beta in FP16 (convert to FP32 for math)
// ------------------------------------------------------------

// ------------------------------------------------------------
// [I2-T27] TODO [ADVANCED] fused normalization + linear layer (sets up I3 fusion)
// ------------------------------------------------------------

// ------------------------------------------------------------
// [I2-T28] CPU reference: LayerNorm (full implementation)
// ------------------------------------------------------------
static void cpu_layernorm_ref(const std::vector<float>& in,
                               std::vector<float>&       out,
                               const std::vector<float>& gamma,
                               const std::vector<float>& beta,
                               int rows, int cols)
{
    for (int r = 0; r < rows; ++r) {
        const float* xr = in.data()  + r * cols;
        float*       yr = out.data() + r * cols;

        double mean = 0.0, var = 0.0;
        for (int c = 0; c < cols; ++c) mean += xr[c];
        mean /= cols;
        for (int c = 0; c < cols; ++c) {
            double d = xr[c] - mean;
            var += d * d;
        }
        var /= cols;
        float inv_std = 1.0f / sqrtf((float)var + EPS);
        for (int c = 0; c < cols; ++c)
            yr[c] = ((float)xr[c] - (float)mean) * inv_std * gamma[c] + beta[c];
    }
}

// [I2-T29] CPU reference: RMSNorm (full implementation)
static void cpu_rmsnorm_ref(const std::vector<float>& in,
                             std::vector<float>&       out,
                             const std::vector<float>& gamma,
                             int rows, int cols)
{
    for (int r = 0; r < rows; ++r) {
        const float* xr = in.data()  + r * cols;
        float*       yr = out.data() + r * cols;

        double rms2 = 0.0;
        for (int c = 0; c < cols; ++c) rms2 += (double)xr[c] * xr[c];
        rms2 /= cols;
        float inv_rms = 1.0f / sqrtf((float)rms2 + EPS);
        for (int c = 0; c < cols; ++c)
            yr[c] = xr[c] * inv_rms * gamma[c];
    }
}

static float max_abs_diff(const float* a, const float* b, int n)
{
    float d = 0.0f;
    for (int i = 0; i < n; ++i)
        d = std::max(d, std::abs(a[i] - b[i]));
    return d;
}

// ------------------------------------------------------------
// [I2-T30] main
// ------------------------------------------------------------
int main()
{
    std::puts("[I2_layernorm_rmsnorm_welford]");
    print_device_info(0);
    NVTX_RANGE("I2_layernorm_rmsnorm_welford/main");

    constexpr int total = B * H;

    // [I2-T31] -- TODO [REQUIRED] step 1: build FP16 matrix, range [-1, 1] --
    // [I2-T32] init already provided
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

    // [I2-T33] FP32 for CPU ref, FP16 for GPU
    std::vector<float>  h_in_f32(total), h_gamma(H, 1.0f), h_beta(H, 0.0f);
    std::vector<__half> h_in_fp16(total);
    for (int i = 0; i < total; ++i) {
        h_in_f32[i]  = dist(rng);
        h_in_fp16[i] = __float2half(h_in_f32[i]);
    }
    // [I2-T34] gamma in [0.5, 1.5], beta in [-0.1, 0.1]
    for (int i = 0; i < H; ++i) {
        h_gamma[i] = 0.5f + dist(rng) * 0.5f + 0.5f;  // [0.5, 1.5]
        h_beta[i]  = dist(rng) * 0.1f;
    }

    // [I2-T35]
    printf("Problem size: B=%d  H=%d  total_floats=%d\n\n", B, H, total);

    // [I2-T36] CPU reference
    std::vector<float> h_ref_ln(total), h_ref_rms(total);
    cpu_layernorm_ref(h_in_f32, h_ref_ln,  h_gamma, h_beta, B, H);
    cpu_rmsnorm_ref  (h_in_f32, h_ref_rms, h_gamma,         B, H);

    // [I2-T37] GPU allocation
    __half *d_in     = nullptr;
    float  *d_out    = nullptr, *d_gamma = nullptr, *d_beta = nullptr;
    CUDA_CHECK(cudaMalloc(&d_in,    total * sizeof(__half)));
    CUDA_CHECK(cudaMalloc(&d_out,   total * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_gamma, H     * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&d_beta,  H     * sizeof(float)));

    CUDA_CHECK(cudaMemcpy(d_in,    h_in_fp16.data(), total * sizeof(__half), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_gamma, h_gamma.data(),   H     * sizeof(float),  cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_beta,  h_beta.data(),    H     * sizeof(float),  cudaMemcpyHostToDevice));

    // [I2-T38] one block per row, blockDim.x = NUM_WARPS * WARP_SIZE = 256
    constexpr int BLOCK = NUM_WARPS * WARP_SIZE;
    // [I2-T39]
    printf("Launch config: grid=(%d,1,1)  block=(%d,1,1)\n\n", B, BLOCK);

    CudaEventTimer timer;
    std::vector<float> h_gpu_out(total);

    // ------------------------------------------------------------
    // [I2-T40] Naive LayerNorm (baseline)
    // ------------------------------------------------------------
    {
        NVTX_RANGE("I2/layernorm_naive");
        CUDA_CHECK(cudaMemset(d_out, 0, total * sizeof(float)));
        timer.start();
        layernorm_naive<<<B, BLOCK>>>(d_in, d_out, d_gamma, d_beta, B, H);
        CUDA_CHECK(cudaGetLastError());
        timer.stop();
        float ms = timer.elapsed_ms();

        CUDA_CHECK(cudaMemcpy(h_gpu_out.data(), d_out, total * sizeof(float), cudaMemcpyDeviceToHost));
        float diff = max_abs_diff(h_ref_ln.data(), h_gpu_out.data(), total);
        float bw   = 2.f * total * (sizeof(__half) + sizeof(float)) / (ms * 1e-3f) / 1e9f;
        // [I2-T41]
        printf("[naive_ln]    %.3f ms  max_err=%.2e  effective_bw=%.1f GB/s\n", ms, diff, bw);
    }

    // ------------------------------------------------------------
    // [I2-T42] Welford LayerNorm (to be implemented)
    // ------------------------------------------------------------
    {
        NVTX_RANGE("I2/layernorm_welford");
        CUDA_CHECK(cudaMemset(d_out, 0, total * sizeof(float)));
        timer.start();
        layernorm_welford<<<B, BLOCK>>>(d_in, d_out, d_gamma, d_beta, B, H, false);
        CUDA_CHECK(cudaGetLastError());
        timer.stop();
        float ms = timer.elapsed_ms();

        CUDA_CHECK(cudaMemcpy(h_gpu_out.data(), d_out, total * sizeof(float), cudaMemcpyDeviceToHost));
        float diff = max_abs_diff(h_ref_ln.data(), h_gpu_out.data(), total);
        float bw   = 1.f * total * (sizeof(__half) + sizeof(float)) / (ms * 1e-3f) / 1e9f;
        // [I2-T43]
        printf("[welford_ln]  %.3f ms  max_err=%.2e  effective_bw=%.1f GB/s  "
               "(stub: expect err < 1e-3 once implemented)\n", ms, diff, bw);
    }

    // ------------------------------------------------------------
    // [I2-T44] Welford RMSNorm (to be implemented)
    // ------------------------------------------------------------
    {
        NVTX_RANGE("I2/rmsnorm_welford");
        CUDA_CHECK(cudaMemset(d_out, 0, total * sizeof(float)));
        timer.start();
        layernorm_welford<<<B, BLOCK>>>(d_in, d_out, d_gamma, d_beta, B, H, true);
        CUDA_CHECK(cudaGetLastError());
        timer.stop();
        float ms = timer.elapsed_ms();

        CUDA_CHECK(cudaMemcpy(h_gpu_out.data(), d_out, total * sizeof(float), cudaMemcpyDeviceToHost));
        float diff = max_abs_diff(h_ref_rms.data(), h_gpu_out.data(), total);
        // [I2-T45]
        printf("[welford_rms] %.3f ms  max_err=%.2e  "
               "(stub: expect err < 1e-3 once implemented)\n", ms, diff);
    }

    // ------------------------------------------------------------
    // [I2-T46] TODO [REQUIRED] step 6: compare Welford vs naive precision and throughput
    // [I2-T47] TODO [REQUIRED] step 6: Nsight Compute measure block-level Welford reduce throughput
    // ------------------------------------------------------------
    // [I2-T48]
    printf("\nAcceptance: Welford LayerNorm max_err < 1e-03 (FP16 precision)\n");
    // [I2-T49]
    printf("Current stub outputs all zeros; CPU check will report mismatch (expected).\n");

    CUDA_CHECK(cudaFree(d_in));
    CUDA_CHECK(cudaFree(d_out));
    CUDA_CHECK(cudaFree(d_gamma));
    CUDA_CHECK(cudaFree(d_beta));

    // [I2-T50]
    printf("\n[I2] done. Use ncu --set full ./I2_layernorm_rmsnorm_welford to analyze.\n");
    return 0;
}
