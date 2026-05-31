// I5_flash_attention_v2_style/main.cu
// [I5-T01] Exercise I5: Flash-Attention-2 style forward (2D tile + online softmax)
//
// [I5-T02] Goals:
//   - 2D tile partition (Br x Bc): outer loop over Q blocks, inner loop over K/V blocks
//   - on-device online softmax: running max m_i + lse l_i + output correction
//   - causal version: inner loop early exit (block-level)
//   - IO byte comparison vs the unfused version
//   - understand "why no N x N softmax buffer is needed"
//
// Build: cmake --build build --target I5_flash_attention_v2_style
// Run:   ./I5_flash_attention_v2_style
//
// Reference: FlashAttention-2 arXiv 2307.08691

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
// [I5-T03] Problem size (consistent with I4)
// ------------------------------------------------------------
constexpr int SEQ   = 1024;  // [I5-T04] N
constexpr int D_K   = 64;    // [I5-T05] head dimension (d_k = d_v)
constexpr float SCALE = 0.125f; // [I5-T06] 1/sqrt(64)

// [I5-T07] FA-2 tile sizes
// [I5-T08] Choice rationale: 2 * Br * d * sizeof(fp16) + 2 * Bc * d * sizeof(fp16) < smem limit
// [I5-T09] Br=64, Bc=64, d=64 -> 2*64*64*2 + 2*64*64*2 = 32 KB (fits typical smem limits)
constexpr int Br = 64;  // [I5-T10] Q tile rows
constexpr int Bc = 64;  // [I5-T11] K/V tile cols (along seq dimension)

constexpr int WARP_SIZE = 32;

// ------------------------------------------------------------
// [I5-T12] Device helpers
// ------------------------------------------------------------
__device__ __forceinline__ float warp_reduce_max(float v)
{
    for (int o = 16; o > 0; o >>= 1)
        v = fmaxf(v, __shfl_down_sync(0xffffffff, v, o));
    return v;
}

__device__ __forceinline__ float warp_reduce_sum(float v)
{
    for (int o = 16; o > 0; o >>= 1)
        v += __shfl_down_sync(0xffffffff, v, o);
    return v;
}

// ------------------------------------------------------------
// [I5-T13] Kernel: Flash-Attention-2 style forward
//
// [I5-T14] Why no N x N softmax buffer is needed:
//   each block keeps only running max m_i and running lse l_i (Br scalars each),
//   not the full attention score matrix S[N, N].
//   Output O is accumulated incrementally inside the inner loop and corrected each
//   step with the factor exp(m_old - m_new).
//
// [I5-T15] Each CUDA block handles one Q tile (Br query rows)
//
// [I5-T16] TODO [REQUIRED] step 2: implement block partitioning logic (outer loop)
// [I5-T17] TODO [REQUIRED] step 3: inner loop with fused online softmax
// [I5-T18] TODO [REQUIRED] step 4: P_block . V_block accumulated into O
// [I5-T19] TODO [REQUIRED] step 5: causal version (block-level + element-level mask)
// ------------------------------------------------------------
__global__ void flash_attention_v2(const __half* __restrict__ Q,   // [N, d]
                                    const __half* __restrict__ K,   // [N, d]
                                    const __half* __restrict__ V,   // [N, d]
                                    __half*       __restrict__ O,   // [N, d]
                                    int N, int d, float scale, bool causal)
{
    // [I5-T20] -- TODO [REQUIRED] step 2: block partitioning --
    // each block corresponds to one Q tile
    // int q_block_idx = blockIdx.x;        // index of Q tile being processed
    // int q_start     = q_block_idx * Br;  // seq starting row for this tile
    // if (q_start >= N) return;
    // int q_end = min(q_start + Br, N);    // actual rows (boundary handling)

    // [I5-T21] -- shared memory declarations (uncomment after sizing) --
    // __shared__ __half smem_Q[Br][D_K];   // Q tile cache
    // __shared__ __half smem_K[Bc][D_K];   // K tile cache
    // __shared__ __half smem_V[Bc][D_K];   // V tile cache

    // [I5-T22] -- registers: per-row running state (Br/blockDim.y rows per thread) --
    // float m_i = -INFINITY;  // running max
    // float l_i = 0.0f;       // running lse (= running sum of exp)
    // float o_acc[D_K] = {0.0f}; // accumulated output

    // [I5-T23] -- TODO [REQUIRED] step 3: inner loop (walk K/V tiles) --
    // for (int kv_block_idx = 0; kv_block_idx < (N + Bc - 1) / Bc; ++kv_block_idx) {
    //
    //     // TODO [REQUIRED] step 5 (causal): block-level early exit
    //     if (causal && kv_block_idx * Bc > q_start + Br - 1) break;
    //
    //     int kv_start = kv_block_idx * Bc;
    //
    //     // cooperatively load K tile into smem_K (blockDim threads cooperating)
    //     // cooperatively load V tile into smem_V
    //     // __syncthreads();
    //
    //     // compute S_block[Br, Bc] = Q_tile . K_tile^T / sqrt(d)
    //     // float s_block[Bc] = {0.0f};  // each thread handles one row's Bc scores
    //     // for (int k = 0; k < d; ++k)
    //     //     s_block[j] += smem_Q[row_local][k] * smem_K[j][k] * scale;
    //
    //     // apply causal mask (element-wise: kv_start + j > q_start + row_local)
    //
    //     // online softmax update:
    //     // float m_new = max(m_i, max(s_block[:]));
    //     // float l_new = l_i * exp(m_i - m_new) + sum(exp(s_block[:] - m_new));
    //
    //     // correct previous o_acc (online softmax correction):
    //     // o_acc[j] *= exp(m_i - m_new);   // NOTE: exp(old_max - new_max)
    //
    //     // accumulate this tile's contribution:
    //     // for (int j = 0; j < Bc; ++j) {
    //     //     float p = exp(s_block[j] - m_new);
    //     //     for (int k = 0; k < d; ++k)
    //     //         o_acc[k] += p * __half2float(smem_V[j][k]);
    //     // }
    //
    //     // m_i = m_new; l_i = l_new;
    //     // __syncthreads();
    // }

    // [I5-T24] -- TODO [REQUIRED] step 4: normalize and write out O --
    // int q_start = blockIdx.x * Br;
    // for (int d = threadIdx.x; d < D_K; d += blockDim.x)
    //     O[(q_start + row_local) * D_K + d] = __float2half(o_acc[d] / l_i);

    // [I5-T25] stub: output 0
    int q_start = blockIdx.x * Br;
    if (q_start >= N) return;
    int q_end = min(q_start + Br, N);
    for (int row = q_start; row < q_end; ++row)
        for (int c = threadIdx.x; c < d; c += blockDim.x)
            O[row * d + c] = __float2half(0.0f);
}

// ------------------------------------------------------------
// [I5-T26] TODO [ADVANCED] FP16 input FP32 accumulator (placeholder is in stub)
// ------------------------------------------------------------

// ------------------------------------------------------------
// [I5-T27] TODO [ADVANCED] Hopper TMA async load Q/K/V tiles (combined with module G4)
// ------------------------------------------------------------

// ------------------------------------------------------------
// [I5-T28] TODO [ADVANCED] MQA/GQA support (multi-query / grouped query attention)
// ------------------------------------------------------------

// ------------------------------------------------------------
// [I5-T29] CPU reference: standard attention (O(N^2*d), fully correct)
// ------------------------------------------------------------
static void cpu_attention_ref(const std::vector<float>& Q,
                               const std::vector<float>& K,
                               const std::vector<float>& V,
                               std::vector<float>&       O,
                               int N, int d, float scale, bool causal)
{
    std::vector<float> S(N * N, 0.0f);
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            if (causal && j > i) { S[i * N + j] = -1e9f; continue; }
            float dot = 0.0f;
            for (int k = 0; k < d; ++k)
                dot += Q[i * d + k] * K[j * d + k];
            S[i * N + j] = dot * scale;
        }
    }
    for (int i = 0; i < N; ++i) {
        float* row = S.data() + i * N;
        float  mx  = *std::max_element(row, row + N);
        float  sm  = 0.0f;
        for (int j = 0; j < N; ++j) { row[j] = std::exp(row[j] - mx); sm += row[j]; }
        for (int j = 0; j < N; ++j) row[j] /= sm;
    }
    for (int i = 0; i < N; ++i)
        for (int c = 0; c < d; ++c) {
            float acc = 0.0f;
            for (int j = 0; j < N; ++j) acc += S[i * N + j] * V[j * d + c];
            O[i * d + c] = acc;
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
// [I5-T30] main
// ------------------------------------------------------------
int main()
{
    std::puts("[I5_flash_attention_v2_style]");
    print_device_info(0);
    NVTX_RANGE("I5_flash_attention_v2_style/main");

    constexpr int N   = SEQ, d = D_K;
    constexpr bool causal = true;
    constexpr int qkv_size = N * d, o_size = N * d;

    // [I5-T31]
    printf("M=%d N=%d d_k=%d Br=%d Bc=%d  causal=%s  scale=%.4f\n\n",
           N, N, d, Br, Bc, causal ? "true" : "false", SCALE);

    // [I5-T32] check smem requirement
    // 2 * Br * d * sizeof(fp16) + 2 * Bc * d * sizeof(fp16)
    size_t smem_needed = (2 * Br * d + 2 * Bc * d) * sizeof(__half);
    // [I5-T33]
    printf("Shared memory estimate: %zu KB  (Br=%d Bc=%d d=%d)\n\n",
           smem_needed / 1024, Br, Bc, d);

    // [I5-T34] -- TODO [REQUIRED] step 1: build Q, K, V (FP16) --
    std::mt19937 rng(42);
    std::normal_distribution<float> ndist(0.0f, 0.1f);

    std::vector<float>  h_Q_f(qkv_size), h_K_f(qkv_size), h_V_f(qkv_size);
    std::vector<__half> h_Q(qkv_size), h_K(qkv_size), h_V(qkv_size);
    for (int i = 0; i < qkv_size; ++i) {
        h_Q_f[i] = ndist(rng); h_Q[i] = __float2half(h_Q_f[i]);
        h_K_f[i] = ndist(rng); h_K[i] = __float2half(h_K_f[i]);
        h_V_f[i] = ndist(rng); h_V[i] = __float2half(h_V_f[i]);
    }

    // [I5-T35] CPU reference
    std::vector<float> h_ref_O(o_size);
    // [I5-T36]
    printf("Computing CPU reference (N=%d d=%d, may take a few seconds)...\n", N, d);
    cpu_attention_ref(h_Q_f, h_K_f, h_V_f, h_ref_O, N, d, SCALE, causal);
    // [I5-T37]
    printf("CPU reference done.\n\n");

    // [I5-T38] GPU allocation
    __half *d_Q = nullptr, *d_K = nullptr, *d_V = nullptr, *d_O = nullptr;
    CUDA_CHECK(cudaMalloc(&d_Q, qkv_size * sizeof(__half)));
    CUDA_CHECK(cudaMalloc(&d_K, qkv_size * sizeof(__half)));
    CUDA_CHECK(cudaMalloc(&d_V, qkv_size * sizeof(__half)));
    CUDA_CHECK(cudaMalloc(&d_O, o_size   * sizeof(__half)));

    CUDA_CHECK(cudaMemcpy(d_Q, h_Q.data(), qkv_size * sizeof(__half), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_K, h_K.data(), qkv_size * sizeof(__half), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_V, h_V.data(), qkv_size * sizeof(__half), cudaMemcpyHostToDevice));

    // [I5-T39] launch config: each block handles one Q tile (Br rows), blockDim.x = Bc
    int num_q_blocks = (N + Br - 1) / Br;
    dim3 grid(num_q_blocks), block(Bc);  // [I5-T40] blockDim can be tuned for d
    // [I5-T41]
    printf("Launch config: grid=(%d,1,1)  block=(%d,1,1)\n", num_q_blocks, Bc);
    // [I5-T42]
    printf("  each block handles Q tile [%d rows], inner loop walks K/V tiles [%d cols]\n\n",
           Br, Bc);

    CudaEventTimer timer;
    std::vector<__half> h_gpu_O(o_size);

    // ------------------------------------------------------------
    // [I5-T43] Flash-Attention-2 kernel
    // ------------------------------------------------------------
    {
        NVTX_RANGE("I5/flash_attn_v2");
        CUDA_CHECK(cudaMemset(d_O, 0, o_size * sizeof(__half)));
        timer.start();
        flash_attention_v2<<<grid, block>>>(d_Q, d_K, d_V, d_O, N, d, SCALE, causal);
        CUDA_CHECK(cudaGetLastError());
        timer.stop();
        float ms = timer.elapsed_ms();

        CUDA_CHECK(cudaMemcpy(h_gpu_O.data(), d_O, o_size * sizeof(__half),
                              cudaMemcpyDeviceToHost));
        float diff = max_abs_diff_fp16(h_gpu_O.data(), h_ref_O.data(), o_size);

        // [I5-T44] theoretical IO: FA-2 reads Q/K/V once each, writes O once
        // (ignoring tile reload). Unfused IO adds S[N,N] + P[N,N] (2x intermediate).
        float fa2_io_bytes = 3.f * qkv_size * sizeof(__half) + o_size * sizeof(__half);
        float bw_gbps = fa2_io_bytes / (ms * 1e-3f) / 1e9f;

        // [I5-T45]
        printf("[flash_attn_v2]  %.3f ms  max_err=%.2e  "
               "effective_bw=%.1f GB/s  (stub: expect err < 1e-2 once implemented)\n",
               ms, diff, bw_gbps);
    }

    // ------------------------------------------------------------
    // [I5-T46] TODO [REQUIRED] step 6: compare FA-2 vs unfused (I4) IO bytes
    // ------------------------------------------------------------
    {
        // [I5-T47] IO estimate (theoretical)
        float unfused_io = (float)(3 * qkv_size * sizeof(__half)   // QKV reads
                                 + 2LL * N * N * sizeof(float)     // S write + P read
                                 + (long long)N * N * sizeof(float)// P write
                                 + o_size * sizeof(__half));       // O write
        float fa2_io = (float)(3 * qkv_size * sizeof(__half)
                               + o_size * sizeof(__half));
        // [I5-T48]
        printf("\nTheoretical IO comparison:\n");
        // [I5-T49]
        printf("  unfused: %.1f MB (includes S[%dx%d] intermediate)\n",
               unfused_io / 1e6f, N, N);
        // [I5-T50]
        printf("  FA-2:    %.1f MB (no NxN buffer)\n", fa2_io / 1e6f);
        // [I5-T51]
        printf("  IO reduction: %.0f%%\n",
               (1.0f - fa2_io / unfused_io) * 100.0f);
    }

    // [I5-T52]
    printf("\nAcceptance: FA-2 vs CPU ref max_err < 1e-02 (FP16 accumulator), epsilon looser than I4\n");
    // [I5-T53]
    printf("Current stub outputs all zeros; CPU check will report mismatch (expected).\n");

    CUDA_CHECK(cudaFree(d_Q));
    CUDA_CHECK(cudaFree(d_K));
    CUDA_CHECK(cudaFree(d_V));
    CUDA_CHECK(cudaFree(d_O));

    // [I5-T54]
    printf("\n[I5] done. Use ncu --metrics l1tex__t_bytes,smsp__sass_inst_executed "
           "./I5_flash_attention_v2_style to analyze smem utilization.\n");
    return 0;
}
