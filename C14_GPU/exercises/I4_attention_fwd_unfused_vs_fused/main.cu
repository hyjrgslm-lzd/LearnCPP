// I4_attention_fwd_unfused_vs_fused/main.cu
// [I4-T01] Exercise I4: Attention forward - unfused (3 kernels) vs fused (1 kernel)
//
// [I4-T02] Goals:
//   - unfused path: QK^T -> softmax -> *V (three independent kernels)
//   - fused path: tile K/V in smem + in-block softmax (single kernel)
//   - FP16 data path, FP32 accumulator
//   - causal mask (lower triangular -inf)
//   - compare global memory traffic via Nsight Compute
//
// Build: cmake --build build --target I4_attention_fwd_unfused_vs_fused
// Run:   ./I4_attention_fwd_unfused_vs_fused

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
// [I4-T03] Problem size (single-head version, batched via blockIdx)
// ------------------------------------------------------------
constexpr int BATCH  = 4;
constexpr int HEADS  = 16;
constexpr int SEQ    = 1024;   // [I4-T04] N (sequence length)
constexpr int D_K    = 64;     // [I4-T05] head dimension
constexpr int D_V    = 64;     // [I4-T06] value dimension (= D_K)
constexpr float SCALE = 0.125f; // [I4-T07] 1 / sqrt(64)
constexpr int WARP_SIZE = 32;

// ------------------------------------------------------------
// [I4-T08] Device helpers
// ------------------------------------------------------------
__device__ __forceinline__ float warp_reduce_max(float val)
{
    for (int offset = 16; offset > 0; offset >>= 1)
        val = fmaxf(val, __shfl_down_sync(0xffffffff, val, offset));
    return val;
}

__device__ __forceinline__ float warp_reduce_sum(float val)
{
    for (int offset = 16; offset > 0; offset >>= 1)
        val += __shfl_down_sync(0xffffffff, val, offset);
    return val;
}

// ------------------------------------------------------------
// [I4-T09] Kernel 1 (unfused): QK^T matmul -> S[N, N]
// [I4-T10] Q: [N, d_k]  K: [N, d_k]  S: [N, N] (scale applied here)
//
// [I4-T11] TODO [REQUIRED] step 2a: implement Q . K^T, FP16 inputs FP32 accumulator
// ------------------------------------------------------------
__global__ void kernel_qk_matmul(const __half* __restrict__ Q,
                                   const __half* __restrict__ K,
                                   float*        __restrict__ S,
                                   int seq, int d_k, float scale)
{
    // [I4-T12] simple impl: each thread computes S[row, col]
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;
    if (row >= seq || col >= seq) return;

    // [I4-T13] TODO [REQUIRED] step 2a: FP16 dot product, FP32 accumulator
    // float acc = 0.0f;
    // for (int k = 0; k < d_k; ++k)
    //     acc += __half2float(Q[row * d_k + k]) * __half2float(K[col * d_k + k]);
    // S[row * seq + col] = acc * scale;

    S[row * seq + col] = 0.0f; // stub
}

// ------------------------------------------------------------
// [I4-T14] Kernel 2 (unfused): softmax S -> P (supports causal mask)
//
// [I4-T15] TODO [REQUIRED] step 2b: online softmax (warp per row) + causal mask
// [I4-T16] TODO [REQUIRED] step 4: causal mask: j > row => add -INFINITY
// ------------------------------------------------------------
__global__ void kernel_softmax_causal(float* __restrict__ S,
                                       int seq, bool causal)
{
    int row  = blockIdx.x;
    int lane = threadIdx.x;
    if (row >= seq) return;

    float* row_s = S + row * seq;

    // [I4-T17] TODO [REQUIRED] step 2b: online softmax (refer to I1)
    // If causal: first set j > row positions to -INFINITY
    // float r_max = -INFINITY, r_sum = 0.0f;
    // for (int j = lane; j < seq; j += WARP_SIZE) {
    //     float v = (causal && j > row) ? -INFINITY : row_s[j];
    //     float new_max = fmaxf(r_max, v);
    //     r_sum = r_sum * expf(r_max - new_max) + expf(v - new_max);
    //     r_max = new_max;
    // }
    // // warp reduce
    // float g_max = ..., g_sum = ...;
    // for (int j = lane; j < seq; j += WARP_SIZE) {
    //     float v = (causal && j > row) ? -INFINITY : row_s[j];
    //     row_s[j] = (causal && j > row) ? 0.0f : expf(v - g_max) / g_sum;
    // }

    (void)S; (void)causal; // stub
}

// ------------------------------------------------------------
// [I4-T18] Kernel 3 (unfused): P . V -> O
// [I4-T19] P: [N, N]  V: [N, d_v]  O: [N, d_v]
//
// [I4-T20] TODO [REQUIRED] step 2c: FP16 V, FP32 P, FP32 accumulator into FP32 O
// ------------------------------------------------------------
__global__ void kernel_pv_matmul(const float*  __restrict__ P,
                                   const __half* __restrict__ V,
                                   __half*       __restrict__ O,
                                   int seq, int d_v)
{
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;
    if (row >= seq || col >= d_v) return;

    // [I4-T21] TODO [REQUIRED] step 2c: FP32 accumulate then cast to FP16
    // float acc = 0.0f;
    // for (int k = 0; k < seq; ++k)
    //     acc += P[row * seq + k] * __half2float(V[k * d_v + col]);
    // O[row * d_v + col] = __float2half(acc);

    O[row * d_v + col] = __float2half(0.0f); // stub
}

// ------------------------------------------------------------
// [I4-T22] Kernel 4: fused attention (softmax + PV fused into single kernel)
//
// [I4-T23] Each block handles one Q row Q[row, :]
// [I4-T24] Inner loop walks K/V tiles, smem holds the tile, online softmax + partial PV accumulation
//
// [I4-T25] TODO [REQUIRED] step 3: implement fused kernel
//   - outer: each block -> one Q row
//   - inner: tile K/V (WARP_SIZE columns at a time), smem stores K and V tiles
//   - in-block online softmax: maintain running_max + running_sum
//   - correct previous partial output: O_acc *= exp(old_max - new_max)
// ------------------------------------------------------------
__global__ void kernel_attention_fused(const __half* __restrict__ Q,
                                        const __half* __restrict__ K,
                                        const __half* __restrict__ V,
                                        __half*       __restrict__ O,
                                        int seq, int d_k, int d_v,
                                        float scale, bool causal)
{
    // [I4-T26] each block handles one row of q
    int q_row = blockIdx.x;
    if (q_row >= seq) return;

    // [I4-T27] TODO [REQUIRED] step 3: fused kernel implementation
    // __shared__ float smem_k[WARP_SIZE][D_K];  // K tile cache
    // __shared__ float smem_v[WARP_SIZE][D_V];  // V tile cache
    //
    // float o_acc[D_V] = {0.0f};  // accumulator (registers)
    // float running_max = -INFINITY, running_sum = 0.0f;
    //
    // for (int kv_start = 0; kv_start < seq; kv_start += WARP_SIZE) {
    //     if (causal && kv_start > q_row) break; // causal early exit
    //
    //     // load K tile to smem (cooperatively)
    //     // compute qk = Q[q_row] . K[kv_start+lane]^T / sqrt(d_k)
    //     // apply causal mask (element-wise)
    //     // online softmax update running_max, running_sum
    //     // correct o_acc: o_acc[j] *= exp(old_max - new_max)
    //     // accumulate: o_acc[j] += p * V[kv_start+lane, j]
    // }
    //
    // // normalize and write out
    // for (int d = threadIdx.x; d < d_v; d += blockDim.x)
    //     O[q_row * d_v + d] = __float2half(o_acc[d] / running_sum);

    // [I4-T28] stub: output 0
    int tid = threadIdx.x;
    for (int d = tid; d < d_v; d += blockDim.x)
        O[q_row * d_v + d] = __float2half(0.0f);
}

// ------------------------------------------------------------
// [I4-T29] TODO [ADVANCED] block-sparse mask attention
// ------------------------------------------------------------

// ------------------------------------------------------------
// [I4-T30] TODO [ADVANCED] FP8 attention path (Q/K/V FP8 quantization)
// ------------------------------------------------------------

// ------------------------------------------------------------
// [I4-T31] TODO [ADVANCED] Flash-Attention-v1 style (multi-warp block-wise softmax)
// ------------------------------------------------------------

// ------------------------------------------------------------
// [I4-T32] CPU reference: standard attention (full implementation, FP32)
// ------------------------------------------------------------
static void cpu_attention_ref(const std::vector<float>& Q,
                               const std::vector<float>& K,
                               const std::vector<float>& V,
                               std::vector<float>&       O,
                               int seq, int d_k, int d_v,
                               float scale, bool causal)
{
    // [I4-T33] QK^T
    std::vector<float> S(seq * seq, 0.0f);
    for (int i = 0; i < seq; ++i) {
        for (int j = 0; j < seq; ++j) {
            if (causal && j > i) { S[i * seq + j] = -1e9f; continue; }
            float dot = 0.0f;
            for (int k = 0; k < d_k; ++k)
                dot += Q[i * d_k + k] * K[j * d_k + k];
            S[i * seq + j] = dot * scale;
        }
    }
    // [I4-T34] softmax
    for (int i = 0; i < seq; ++i) {
        float* row = S.data() + i * seq;
        float  mx  = *std::max_element(row, row + seq);
        float  sm  = 0.0f;
        for (int j = 0; j < seq; ++j) { row[j] = std::exp(row[j] - mx); sm += row[j]; }
        for (int j = 0; j < seq; ++j) row[j] /= sm;
    }
    // [I4-T35] P . V
    for (int i = 0; i < seq; ++i) {
        for (int d = 0; d < d_v; ++d) {
            float acc = 0.0f;
            for (int j = 0; j < seq; ++j)
                acc += S[i * seq + j] * V[j * d_v + d];
            O[i * d_v + d] = acc;
        }
    }
}

static float max_abs_diff_fp16(const __half* a, const float* b, int n)
{
    float d = 0.0f;
    for (int i = 0; i < n; ++i)
        d = std::max(d, std::abs(__half2float(a[i]) - b[i]));
    return d;
}

// ------------------------------------------------------------
// [I4-T36] main (single-head, batch=1 demo)
// ------------------------------------------------------------
int main()
{
    std::puts("[I4_attention_fwd_unfused_vs_fused]");
    print_device_info(0);
    NVTX_RANGE("I4_attention_fwd_unfused_vs_fused/main");

    // [I4-T37] demo with B=1, H=1 single head; multi-head extension is in advanced tasks
    constexpr int seq = SEQ, d_k = D_K, d_v = D_V;
    constexpr bool causal = true;

    // [I4-T38]
    printf("Problem size: B=%d  H=%d  N=%d  d_k=%d  d_v=%d  causal=%s\n\n",
           BATCH, HEADS, seq, d_k, d_v, causal ? "true" : "false");

    constexpr int qkv_size = seq * d_k;
    constexpr int o_size   = seq * d_v;
    constexpr int s_size   = seq * seq;

    // [I4-T39] -- TODO [REQUIRED] step 1: build Q, K, V (FP16) --
    std::mt19937 rng(42);
    std::normal_distribution<float> ndist(0.0f, 0.1f);

    std::vector<float>  h_Q_f32(qkv_size), h_K_f32(qkv_size), h_V_f32(qkv_size);
    std::vector<__half> h_Q(qkv_size), h_K(qkv_size), h_V(qkv_size);
    for (int i = 0; i < qkv_size; ++i) {
        h_Q_f32[i] = ndist(rng); h_Q[i] = __float2half(h_Q_f32[i]);
        h_K_f32[i] = ndist(rng); h_K[i] = __float2half(h_K_f32[i]);
        h_V_f32[i] = ndist(rng); h_V[i] = __float2half(h_V_f32[i]);
    }

    // [I4-T40] CPU reference (small scale; SEQ=1024 may be a bit slow, expected)
    std::vector<float> h_ref_O(o_size);
    // [I4-T41] CPU ref O(N^2*d) is slow at SEQ=1024, used only for correctness
    printf("Computing CPU reference (SEQ=%d, may take a few seconds)...\n", seq);
    cpu_attention_ref(h_Q_f32, h_K_f32, h_V_f32, h_ref_O,
                      seq, d_k, d_v, SCALE, causal);
    // [I4-T42]
    printf("CPU reference done.\n\n");

    // [I4-T43] GPU allocation
    __half *d_Q = nullptr, *d_K = nullptr, *d_V = nullptr;
    __half *d_O_unf = nullptr, *d_O_fused = nullptr;
    float  *d_S = nullptr;  // [I4-T44] unfused intermediate result

    CUDA_CHECK(cudaMalloc(&d_Q,      qkv_size * sizeof(__half)));
    CUDA_CHECK(cudaMalloc(&d_K,      qkv_size * sizeof(__half)));
    CUDA_CHECK(cudaMalloc(&d_V,      qkv_size * sizeof(__half)));
    CUDA_CHECK(cudaMalloc(&d_O_unf,  o_size   * sizeof(__half)));
    CUDA_CHECK(cudaMalloc(&d_O_fused,o_size   * sizeof(__half)));
    CUDA_CHECK(cudaMalloc(&d_S,      s_size   * sizeof(float)));

    CUDA_CHECK(cudaMemcpy(d_Q, h_Q.data(), qkv_size * sizeof(__half), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_K, h_K.data(), qkv_size * sizeof(__half), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_V, h_V.data(), qkv_size * sizeof(__half), cudaMemcpyHostToDevice));

    CudaEventTimer timer;
    std::vector<__half> h_gpu_O(o_size);

    // ------------------------------------------------------------
    // [I4-T45] Unfused path: K1(QK^T) + K2(softmax) + K3(PV)
    // ------------------------------------------------------------
    {
        NVTX_RANGE("I4/unfused");
        CUDA_CHECK(cudaMemset(d_S,     0, s_size   * sizeof(float)));
        CUDA_CHECK(cudaMemset(d_O_unf, 0, o_size   * sizeof(__half)));

        timer.start();
        // [I4-T46] K1: QK^T (16x16 thread tile)
        dim3 blk_qk(16, 16), grd_qk((seq + 15) / 16, (seq + 15) / 16);
        kernel_qk_matmul<<<grd_qk, blk_qk>>>(d_Q, d_K, d_S, seq, d_k, SCALE);
        CUDA_CHECK(cudaGetLastError());

        // [I4-T47] K2: softmax (warp per row)
        kernel_softmax_causal<<<seq, WARP_SIZE>>>(d_S, seq, causal);
        CUDA_CHECK(cudaGetLastError());

        // [I4-T48] K3: PV (16x16 thread tile)
        dim3 blk_pv(16, 16), grd_pv((d_v + 15) / 16, (seq + 15) / 16);
        kernel_pv_matmul<<<grd_pv, blk_pv>>>(d_S, d_V, d_O_unf, seq, d_v);
        CUDA_CHECK(cudaGetLastError());
        timer.stop();

        float ms = timer.elapsed_ms();
        CUDA_CHECK(cudaMemcpy(h_gpu_O.data(), d_O_unf, o_size * sizeof(__half),
                              cudaMemcpyDeviceToHost));
        float diff = max_abs_diff_fp16(h_gpu_O.data(), h_ref_O.data(), o_size);
        // [I4-T49]
        printf("[unfused]  %.3f ms  max_err=%.2e  (stub: expect < 1e-3 once implemented)\n",
               ms, diff);
    }

    // ------------------------------------------------------------
    // [I4-T50] Fused path
    // ------------------------------------------------------------
    {
        NVTX_RANGE("I4/fused");
        CUDA_CHECK(cudaMemset(d_O_fused, 0, o_size * sizeof(__half)));
        timer.start();
        kernel_attention_fused<<<seq, WARP_SIZE>>>(
            d_Q, d_K, d_V, d_O_fused, seq, d_k, d_v, SCALE, causal);
        CUDA_CHECK(cudaGetLastError());
        timer.stop();

        float ms = timer.elapsed_ms();
        CUDA_CHECK(cudaMemcpy(h_gpu_O.data(), d_O_fused, o_size * sizeof(__half),
                              cudaMemcpyDeviceToHost));
        float diff = max_abs_diff_fp16(h_gpu_O.data(), h_ref_O.data(), o_size);
        // [I4-T51]
        printf("[fused]    %.3f ms  max_err=%.2e  (stub: expect < 1e-3 once implemented)\n",
               ms, diff);
    }

    // ------------------------------------------------------------
    // [I4-T52] TODO [REQUIRED] step 5: validate unfused vs fused diff < 1e-3 (FP16)
    // [I4-T53] TODO [REQUIRED] step 6: Nsight Compute compare global memory traffic (unfused larger)
    // ------------------------------------------------------------
    // [I4-T54]
    printf("\nAcceptance: fused vs unfused diff < 1e-03, "
           "fused global memory traffic reduced by >= 20%%\n");

    CUDA_CHECK(cudaFree(d_Q));
    CUDA_CHECK(cudaFree(d_K));
    CUDA_CHECK(cudaFree(d_V));
    CUDA_CHECK(cudaFree(d_O_unf));
    CUDA_CHECK(cudaFree(d_O_fused));
    CUDA_CHECK(cudaFree(d_S));

    // [I4-T55]
    printf("\n[I4] done. Use ncu --metrics l1tex__t_bytes,lts__t_bytes "
           "./I4_attention_fwd_unfused_vs_fused to compare memory traffic.\n");
    return 0;
}
