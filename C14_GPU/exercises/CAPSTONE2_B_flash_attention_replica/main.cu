// CAPSTONE2_B_flash_attention_replica/main.cu
// ============================================================
// 结课项目 2 — 分支 B：Flash-Attention-2 前向源码精读 + 简化复现
//
// 任务描述：
//   1. 阅读 Flash-Attention-2 repo csrc/flash_attn/ 前向 kernel
//   2. 理解 tile 划分（Br×Bc）、在线 softmax、output rescale
//   3. 复现一个功能等价的简化版 FA-2 前向 kernel（FP16）
//   4. 对标 CPU 参考精度（max rel error < 1e-2）
//   5. 测量 TFLOPS，对比 unfused naive 实现
//
// 问题规模：B=4, H=16, N=2048, d_k=64（单头维度）
//   每次 kernel 调用处理 1 个 head 的 (N, d_k) attention
//
// 在线 softmax 算法（FA-2 核心）：
//   m = -inf, l = 0, accum = 0
//   for j in 0..num_kv_blocks:
//     S = Q @ K_j^T * scale          // (Br, Bc)
//     if causal: mask S[row > col]
//     m_new = max(m, row_max(S))
//     P = exp(S - m_new)             // (Br, Bc)
//     l_new = l * exp(m - m_new) + row_sum(P)
//     accum = accum * (l / l_new) + P @ V_j
//     m = m_new; l = l_new
//   output = accum / l
//
// 编译：cmake --build build --target CAPSTONE2_B_flash_attention_replica
// 参考：FlashAttention-2 arXiv 2307.08691
// ============================================================

// ─────────────────────────────────────────────────────────────
// 公共头文件
// ─────────────────────────────────────────────────────────────
#include "cuda_check.cuh"
#include "device_info.cuh"
#include "nvtx_range.cuh"
#include "timer.cuh"

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
#include <numeric>

// ─────────────────────────────────────────────────────────────
// 问题规模
// ─────────────────────────────────────────────────────────────
static constexpr int BATCH   = 4;     // batch size
static constexpr int HEADS   = 16;    // 注意力头数
static constexpr int SEQ_LEN = 2048;  // 序列长度 N
static constexpr int HEAD_DIM = 64;   // 单头维度 d_k（=d_v）

// 缩放因子：1/sqrt(d_k)
static constexpr float SCALE = 0.125f;  // 1/sqrt(64)

// FA-2 tile 大小（Br=行块，Bc=列块）
// 依据：smem 占用 = (Br+2*Bc)*d*sizeof(fp16)
// Br=64, Bc=64, d=64 → (64+128)*64*2 = 24 KB (< 48 KB 默认 smem)
static constexpr int Br = 64;   // Q tile 行数（outer loop 步长）
static constexpr int Bc = 64;   // K/V tile 列数（inner loop 步长）

// benchmark 轮数
static constexpr int WARMUP_ITERS = 10;
static constexpr int BENCH_ITERS  = 100;

// ─────────────────────────────────────────────────────────────
// 设备端工具函数
// ─────────────────────────────────────────────────────────────

// warp 内规约最大值（用于 softmax max 计算）
__device__ __forceinline__ float warp_reduce_max(float v)
{
    for (int offset = 16; offset > 0; offset >>= 1)
        v = fmaxf(v, __shfl_down_sync(0xffffffff, v, offset));
    return v;
}

// warp 内规约求和（用于 softmax sum 计算）
__device__ __forceinline__ float warp_reduce_sum(float v)
{
    for (int offset = 16; offset > 0; offset >>= 1)
        v += __shfl_down_sync(0xffffffff, v, offset);
    return v;
}

// block 内规约最大值（通过共享内存，支持多 warp）
__device__ __forceinline__ float block_reduce_max(float v, float* smem_buf, int tid, int nthreads)
{
    int lane = tid % 32;
    int wid  = tid / 32;
    int nwarps = (nthreads + 31) / 32;

    v = warp_reduce_max(v);
    if (lane == 0) smem_buf[wid] = v;
    __syncthreads();
    v = (tid < nwarps) ? smem_buf[tid] : -FLT_MAX;
    if (wid == 0) v = warp_reduce_max(v);
    __syncthreads();
    return v;
}

// block 内规约求和
__device__ __forceinline__ float block_reduce_sum(float v, float* smem_buf, int tid, int nthreads)
{
    int lane = tid % 32;
    int wid  = tid / 32;
    int nwarps = (nthreads + 31) / 32;

    v = warp_reduce_sum(v);
    if (lane == 0) smem_buf[wid] = v;
    __syncthreads();
    v = (tid < nwarps) ? smem_buf[tid] : 0.0f;
    if (wid == 0) v = warp_reduce_sum(v);
    __syncthreads();
    return v;
}

// ─────────────────────────────────────────────────────────────
// CPU 参考实现（标准 O(N^2) attention，用于正确性验证）
//
// 输入/输出均为 FP32，避免 FP16 精度影响参考值。
// 支持 causal mask（上三角置 -inf）。
// ─────────────────────────────────────────────────────────────
static void cpu_attention_ref(
    const float* __restrict__ Q,   // [N, d]
    const float* __restrict__ K,   // [N, d]
    const float* __restrict__ V,   // [N, d]
    float*       __restrict__ O,   // [N, d]
    int N, int d, float scale, bool causal)
{
    std::vector<float> S(N * N, 0.0f);

    // Step 1: S = Q @ K^T * scale，并应用 causal mask
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            if (causal && j > i) { S[i * N + j] = -1e9f; continue; }
            float dot = 0.0f;
            for (int k = 0; k < d; ++k)
                dot += Q[i * d + k] * K[j * d + k];
            S[i * N + j] = dot * scale;
        }
    }

    // Step 2: softmax（每行独立）
    for (int i = 0; i < N; ++i) {
        float* row = S.data() + i * N;
        float  mx  = *std::max_element(row, row + N);
        float  sm  = 0.0f;
        for (int j = 0; j < N; ++j) { row[j] = std::exp(row[j] - mx); sm += row[j]; }
        for (int j = 0; j < N; ++j) row[j] /= sm;
    }

    // Step 3: O = softmax(S) @ V
    for (int i = 0; i < N; ++i)
        for (int c = 0; c < d; ++c) {
            float acc = 0.0f;
            for (int j = 0; j < N; ++j) acc += S[i * N + j] * V[j * d + c];
            O[i * d + c] = acc;
        }
}

// ─────────────────────────────────────────────────────────────
// Naive unfused GPU attention（O(N^2) 中间矩阵，对标 IO）
//
// 仅用于 IO 字节数对比，不作为正确性参考。
// ─────────────────────────────────────────────────────────────
__global__ void naive_attention_kernel(
    const __half* __restrict__ Q,   // [N, d]
    const __half* __restrict__ K,   // [N, d]
    const __half* __restrict__ V,   // [N, d]
    float*        __restrict__ S,   // [N, N] 临时 score 矩阵
    __half*       __restrict__ O,   // [N, d]
    int N, int d, float scale, bool causal)
{
    // 简单实现：每个线程计算 S[i, j]
    int i = blockIdx.y * blockDim.y + threadIdx.y;
    int j = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= N || j >= N) return;

    if (causal && j > i) {
        S[i * N + j] = -1e9f;
        return;
    }
    float dot = 0.0f;
    for (int k = 0; k < d; ++k)
        dot += __half2float(Q[i * d + k]) * __half2float(K[j * d + k]);
    S[i * N + j] = dot * scale;
}

// ─────────────────────────────────────────────────────────────
// ============================================================
// Flash-Attention-2 简化版前向 kernel
//
// 每个 CUDA block 处理一个 Q tile（Br 行查询序列）。
// blockDim.x = HEAD_DIM（每个 thread 负责 output 向量的一个维度）
// blockDim.y = Br / warps_per_row（按需调整）
//
// 共享内存布局：
//   smem_Q : [Br][HEAD_DIM]  FP16，Q tile 缓存（outer loop 前加载一次）
//   smem_K : [Bc][HEAD_DIM]  FP16，K tile 缓存（inner loop 每轮更新）
//   smem_V : [Bc][HEAD_DIM]  FP16，V tile 缓存（inner loop 每轮更新）
//   smem_S : [Br][Bc]        FP32，attention score 临时存储
//
// online softmax 状态（每行 1 个，存于 register）：
//   m_i : 当前行的 running max
//   l_i : 当前行的 running sum of exp
//   o_acc[HEAD_DIM] : 当前行的 output 累加器（FP32）
//
// TODO [必做] 步骤编号与 14-结课项目2.md 对应：
//   步骤 2 = Q block outer loop 划分
//   步骤 3 = K/V block inner loop + online softmax
//   步骤 4 = output rescale + P@V 累加
//   步骤 5 = causal mask
// ============================================================
__global__ void flash_attention_fwd(
    const __half* __restrict__ Q,       // [N, d]
    const __half* __restrict__ K,       // [N, d]
    const __half* __restrict__ V,       // [N, d]
    __half*       __restrict__ O,       // [N, d]
    int N, int d_model, float scale,
    bool causal)
{
    // ── 共享内存声明 ─────────────────────────────────────────
    // smem_Q: 外层 Q tile（整个 block 生命周期内固定）
    __shared__ __half smem_Q[Br][HEAD_DIM];
    // smem_K, smem_V: 内层 K/V tile（每个 KV block 更新）
    __shared__ __half smem_K[Bc][HEAD_DIM];
    __shared__ __half smem_V[Bc][HEAD_DIM];
    // smem_S: attention score tile [Br][Bc]（FP32，online softmax 需要）
    __shared__ float  smem_S[Br][Bc];
    // reduce 辅助（最多 32 warps）
    __shared__ float  smem_reduce[32];

    const int tid = threadIdx.x;       // 0 .. blockDim.x-1（= Bc 或 HEAD_DIM）
    const int nthreads = blockDim.x;

    // ── TODO [必做] 步骤 2：block 划分（外层 Q tile）────────────────────
    //
    // 每个 CUDA block 处理一个 Q tile（Br 行）：
    //   q_block_idx = blockIdx.x   → 第 q_block_idx 个 Q tile
    //   q_start     = q_block_idx * Br
    //   q_end       = min(q_start + Br, N)
    //
    // 实现提示：
    //   1. 协作加载 Q tile 到 smem_Q（threadIdx.x 遍历 HEAD_DIM，多次）
    //   2. 初始化 m_i = -FLT_MAX, l_i = 0, o_acc = 0（per-row register）
    //   3. 进入 KV block 内层循环

    // ── stub：跳过计算，写零输出 ─────────────────────────────
    int q_start = blockIdx.x * Br;
    if (q_start >= N) return;
    int q_end = min(q_start + Br, N);

    // TODO [必做] 取消注释并实现以下框架：
    //
    // --- 加载 Q tile ---
    // for (int row = 0; row < Br; ++row) {
    //     int global_row = q_start + row;
    //     for (int col = tid; col < HEAD_DIM; col += nthreads) {
    //         smem_Q[row][col] = (global_row < N)
    //             ? Q[global_row * HEAD_DIM + col]
    //             : __float2half(0.0f);
    //     }
    // }
    // __syncthreads();
    //
    // --- per-row online softmax state（每个 thread 负责一行）---
    // // 假设 blockDim.x >= Br，每个 thread 负责一行；或用 tid % Br
    // int my_row = tid;   // 适用于 blockDim.x = Br = 64 时
    // float m_i   = -FLT_MAX;
    // float l_i   = 0.0f;
    // float o_acc[HEAD_DIM];
    // for (int c = 0; c < HEAD_DIM; ++c) o_acc[c] = 0.0f;
    //
    // int num_kv_blocks = (N + Bc - 1) / Bc;
    //
    // for (int kv_block = 0; kv_block < num_kv_blocks; ++kv_block) {
    //
    //     // TODO [必做] 步骤 5（causal）：block-level 提前退出
    //     // K/V 列块起点 > 当前 Q 行块终点 → 全部 mask → 可跳过
    //     if (causal && kv_block * Bc > q_start + Br - 1) break;
    //
    //     int kv_start = kv_block * Bc;
    //
    //     // --- 协作加载 K tile ---
    //     for (int row = 0; row < Bc; ++row) {
    //         int global_row = kv_start + row;
    //         for (int col = tid; col < HEAD_DIM; col += nthreads) {
    //             smem_K[row][col] = (global_row < N)
    //                 ? K[global_row * HEAD_DIM + col]
    //                 : __float2half(0.0f);
    //         }
    //     }
    //     // --- 协作加载 V tile ---
    //     for (int row = 0; row < Bc; ++row) {
    //         int global_row = kv_start + row;
    //         for (int col = tid; col < HEAD_DIM; col += nthreads) {
    //             smem_V[row][col] = (global_row < N)
    //                 ? V[global_row * HEAD_DIM + col]
    //                 : __float2half(0.0f);
    //         }
    //     }
    //     __syncthreads();
    //
    //     // TODO [必做] 步骤 3：计算 S = Q_tile @ K_tile^T * scale
    //     // 每个 thread 计算自己负责的行（my_row）与所有 Bc 列的 dot product
    //     if (my_row < Br && (q_start + my_row) < N) {
    //         for (int j = 0; j < Bc && (kv_start + j) < N; ++j) {
    //             float dot = 0.0f;
    //             for (int k = 0; k < HEAD_DIM; ++k)
    //                 dot += __half2float(smem_Q[my_row][k])
    //                      * __half2float(smem_K[j][k]);
    //             smem_S[my_row][j] = dot * scale;
    //
    //             // TODO [必做] 步骤 5（causal）：元素级 mask
    //             if (causal && (kv_start + j) > (q_start + my_row))
    //                 smem_S[my_row][j] = -1e9f;
    //         }
    //     }
    //     __syncthreads();
    //
    //     // TODO [必做] 步骤 3：online softmax 更新
    //     if (my_row < Br && (q_start + my_row) < N) {
    //         // a) 计算本 tile 的行最大值
    //         float m_tile = -FLT_MAX;
    //         for (int j = 0; j < Bc && (kv_start + j) < N; ++j)
    //             m_tile = fmaxf(m_tile, smem_S[my_row][j]);
    //
    //         // b) 更新 running max
    //         float m_new = fmaxf(m_i, m_tile);
    //
    //         // c) 计算 P_tilde = exp(S - m_new) 并求和
    //         float l_tile = 0.0f;
    //         for (int j = 0; j < Bc && (kv_start + j) < N; ++j) {
    //             smem_S[my_row][j] = expf(smem_S[my_row][j] - m_new);
    //             l_tile += smem_S[my_row][j];
    //         }
    //
    //         // TODO [必做] 步骤 4：rescale 旧的 o_acc（数值稳定性关键！）
    //         // 公式：o_acc *= exp(m_old - m_new)
    //         float rescale = expf(m_i - m_new);
    //         for (int c = 0; c < HEAD_DIM; ++c)
    //             o_acc[c] *= rescale;
    //
    //         // 更新 running lse
    //         float l_new = l_i * rescale + l_tile;
    //         m_i = m_new;
    //         l_i = l_new;
    //
    //         // TODO [必做] 步骤 4：累加 P_tilde @ V_tile
    //         for (int j = 0; j < Bc && (kv_start + j) < N; ++j) {
    //             float p = smem_S[my_row][j];
    //             for (int c = 0; c < HEAD_DIM; ++c)
    //                 o_acc[c] += p * __half2float(smem_V[j][c]);
    //         }
    //     }
    //     __syncthreads();
    // }  // end inner KV loop
    //
    // // TODO [必做]：归一化并写出 O（FP16）
    // if (my_row < Br && (q_start + my_row) < N) {
    //     float inv_l = (l_i > 1e-9f) ? (1.0f / l_i) : 0.0f;
    //     int global_row = q_start + my_row;
    //     for (int c = 0; c < HEAD_DIM; ++c)
    //         O[global_row * HEAD_DIM + c] = __float2half(o_acc[c] * inv_l);
    // }

    // ── stub：输出全零（验证预期 FAIL，实现 TODO 后替换）───────
    for (int row = q_start; row < q_end; ++row)
        for (int col = tid; col < d_model; col += nthreads)
            O[row * d_model + col] = __float2half(0.0f);

    (void)smem_K; (void)smem_V; (void)smem_S; (void)smem_reduce;
    (void)causal; (void)scale;
}

// ─────────────────────────────────────────────────────────────
// TODO [进阶] Hopper TMA 版本（FA-3 风格）
//
// 参考 G4 的 TMA 异步加载模式：
//   - 构造 Q/K/V 的 CUtensorMap（cuTensorMapEncodeTiled）
//   - producer warp 用 cp.async.bulk.tensor 异步加载 K/V tile
//   - consumer warp 用 wgmma.mma_async 做 QK^T 和 PV 计算
//   - mbarrier 同步 producer/consumer
//
// 这是 Flash-Attention-3 的核心思路（arXiv 2407.08608）。
// 在 sm_90a 上可以达到 FA-2 的 1.5-2× 性能。
// ─────────────────────────────────────────────────────────────

// ─────────────────────────────────────────────────────────────
// 辅助：精度验证
// ─────────────────────────────────────────────────────────────
static float compute_max_rel_error(
    const float*  ref,
    const __half* got,
    int n)
{
    float max_err = 0.0f;
    for (int i = 0; i < n; ++i) {
        float a = ref[i];
        float b = __half2float(got[i]);
        float rel = std::fabs(a - b) / (std::fabs(a) + 1e-6f);
        max_err = std::max(max_err, rel);
    }
    return max_err;
}

// ─────────────────────────────────────────────────────────────
// 辅助：TFLOPS 计算（attention: 2*N*N*d 的 QK^T + 2*N*N*d 的 PV）
// ─────────────────────────────────────────────────────────────
static double calc_attention_tflops(int N, int d, float ms)
{
    // QK^T: 2 * N * N * d FLOPs
    // PV  : 2 * N * N * d FLOPs
    double flops = 4.0 * static_cast<double>(N) * N * d;
    return flops / (static_cast<double>(ms) * 1e-3) / 1e12;
}

// ─────────────────────────────────────────────────────────────
// 主程序
// ─────────────────────────────────────────────────────────────
int main()
{
    std::puts("============================================================");
    std::puts("[CAPSTONE2_B] Flash-Attention-2 mini 复现");
    std::puts("============================================================");
    print_device_info(0);

    NVTX_RANGE("CAPSTONE2_B/main");

    const int N = SEQ_LEN;
    const int d = HEAD_DIM;
    const bool causal = true;

    std::fprintf(stdout,
        "\n问题规模：B=%d  H=%d  N=%d  d_k=%d  causal=%s  scale=%.4f\n",
        BATCH, HEADS, N, d, causal ? "true" : "false", SCALE);
    std::fprintf(stdout,
        "Tile 参数：Br=%d  Bc=%d\n", Br, Bc);

    // smem 需求估算
    size_t smem_q  = (size_t)Br * d * sizeof(__half);
    size_t smem_kv = 2 * (size_t)Bc * d * sizeof(__half);
    size_t smem_s  = (size_t)Br * Bc * sizeof(float);
    size_t smem_total = smem_q + smem_kv + smem_s + 32 * sizeof(float);
    std::fprintf(stdout,
        "共享内存需求：smem_Q=%.1f KB + smem_K+V=%.1f KB + smem_S=%.1f KB = %.1f KB\n\n",
        smem_q / 1024.0, smem_kv / 1024.0, smem_s / 1024.0, smem_total / 1024.0);

    // ── 主机数据准备（取第一个 batch + head 做验证）───────────
    const int qkv_elems = N * d;
    std::mt19937 rng(42);
    std::normal_distribution<float> ndist(0.0f, 0.1f);

    std::vector<float>  h_Q_f(qkv_elems), h_K_f(qkv_elems), h_V_f(qkv_elems);
    std::vector<__half> h_Q(qkv_elems), h_K(qkv_elems), h_V(qkv_elems);

    for (int i = 0; i < qkv_elems; ++i) {
        h_Q_f[i] = ndist(rng);  h_Q[i] = __float2half(h_Q_f[i]);
        h_K_f[i] = ndist(rng);  h_K[i] = __float2half(h_K_f[i]);
        h_V_f[i] = ndist(rng);  h_V[i] = __float2half(h_V_f[i]);
    }

    // CPU 参考（FP32，正确性基准）
    std::vector<float> h_ref_O(qkv_elems);
    std::fprintf(stdout, "正在计算 CPU 参考（N=%d d=%d，可能需要数秒）...\n", N, d);
    cpu_attention_ref(h_Q_f.data(), h_K_f.data(), h_V_f.data(),
                      h_ref_O.data(), N, d, SCALE, causal);
    std::puts("CPU 参考完成。\n");

    // IO 字节数对比（理论估算）
    {
        // FA-2 IO：读 Q/K/V 各一次（O(N*d)），写 O 一次
        float fa2_io  = 3.f * qkv_elems * sizeof(__half) + qkv_elems * sizeof(__half);
        // Unfused IO：读 Q/K/V + 写 S[N,N] + 读 S + 写 O
        float unf_io  = 3.f * qkv_elems * sizeof(__half)
                       + 2.f * N * N * sizeof(float)
                       + qkv_elems * sizeof(__half);
        std::puts("── 理论 IO 对比 ────────────────────────────────────────");
        std::fprintf(stdout,
            "  Unfused（含 S[%d×%d] 中间矩阵）：%.1f MB\n", N, N, unf_io / 1e6f);
        std::fprintf(stdout,
            "  FA-2（无 N×N buffer）：%.1f MB\n", fa2_io / 1e6f);
        std::fprintf(stdout,
            "  IO 减少：%.0f%%\n\n", (1.0f - fa2_io / unf_io) * 100.0f);
    }

    // ── 设备内存 ─────────────────────────────────────────────
    __half *d_Q = nullptr, *d_K = nullptr, *d_V = nullptr;
    __half *d_O_fa2 = nullptr;
    std::vector<__half> h_O_fa2(qkv_elems);

    CUDA_CHECK(cudaMalloc(&d_Q,    qkv_elems * sizeof(__half)));
    CUDA_CHECK(cudaMalloc(&d_K,    qkv_elems * sizeof(__half)));
    CUDA_CHECK(cudaMalloc(&d_V,    qkv_elems * sizeof(__half)));
    CUDA_CHECK(cudaMalloc(&d_O_fa2, qkv_elems * sizeof(__half)));

    CUDA_CHECK(cudaMemcpy(d_Q, h_Q.data(), qkv_elems * sizeof(__half), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_K, h_K.data(), qkv_elems * sizeof(__half), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_V, h_V.data(), qkv_elems * sizeof(__half), cudaMemcpyHostToDevice));

    // ── kernel 启动配置 ────────────────────────────────────────
    // 每个 block 处理一个 Q tile（Br 行）
    // blockDim.x = Br（每个 thread 负责一行的 softmax 和 output 累加）
    // grid.x = ceil(N / Br)
    int num_q_blocks = (N + Br - 1) / Br;
    dim3 grid_fa2(num_q_blocks);
    dim3 block_fa2(Br);  // 64 threads/block（1 thread/row）

    std::fprintf(stdout,
        "── FA-2 kernel 启动配置：grid=(%d,1)  block=(%d,1)\n",
        num_q_blocks, Br);
    std::fprintf(stdout,
        "   每 block 处理 Q[%d 行]，内层遍历 %d 个 KV tile\n\n",
        Br, (N + Bc - 1) / Bc);

    // ── 路径：FA-2 简化版 ─────────────────────────────────────
    std::puts("── 路径：FA-2 简化版前向（TODO 实现后替换 stub）──────────");
    float ms_fa2 = 0.0f;
    {
        NVTX_RANGE("CAPSTONE2_B/fa2_kernel");
        CUDA_CHECK(cudaMemset(d_O_fa2, 0, qkv_elems * sizeof(__half)));

        // 预热
        for (int i = 0; i < WARMUP_ITERS; ++i) {
            flash_attention_fwd<<<grid_fa2, block_fa2>>>(
                d_Q, d_K, d_V, d_O_fa2, N, d, SCALE, causal);
        }
        CUDA_CHECK(cudaDeviceSynchronize());
        CUDA_CHECK(cudaGetLastError());

        // 计时
        CudaEventTimer timer;
        timer.start();
        for (int i = 0; i < BENCH_ITERS; ++i) {
            flash_attention_fwd<<<grid_fa2, block_fa2>>>(
                d_Q, d_K, d_V, d_O_fa2, N, d, SCALE, causal);
        }
        timer.stop();
        ms_fa2 = timer.elapsed_ms() / BENCH_ITERS;
        CUDA_CHECK(cudaGetLastError());
    }

    CUDA_CHECK(cudaMemcpy(h_O_fa2.data(), d_O_fa2, qkv_elems * sizeof(__half), cudaMemcpyDeviceToHost));

    // ── 正确性验证 ────────────────────────────────────────────
    std::puts("── 正确性验证（vs CPU FP32 参考）───────────────────────────");
    float max_err_fa2 = compute_max_rel_error(h_ref_O.data(), h_O_fa2.data(), qkv_elems);
    bool  pass_fa2    = (max_err_fa2 < 1e-2f);
    std::fprintf(stdout,
        "  [fa2_simplified] %s  max_rel_err=%.3e  (阈值 1e-2)\n",
        pass_fa2 ? "PASS" : "FAIL (stub 阶段预期 FAIL)",
        (double)max_err_fa2);
    if (!pass_fa2)
        std::puts("  -> 实现 main.cu 中所有 TODO [必做] 步骤后重新运行。");

    // ── 性能汇总 ─────────────────────────────────────────────
    double tflops_fa2 = calc_attention_tflops(N, d, ms_fa2);

    // 理论内存带宽（用于有效带宽计算）
    float fa2_io_bytes = 4.f * qkv_elems * sizeof(__half);  // Q+K+V+O
    float bw_gbps = fa2_io_bytes / (ms_fa2 * 1e-3f) / 1e9f;

    std::puts("\n── 性能汇总 ──────────────────────────────────────────────");
    std::fprintf(stdout,
        "%-30s | %8s | %10s | %12s\n",
        "变体", "ms/iter", "TFLOPS", "有效带宽 GB/s");
    std::fprintf(stdout,
        "%-30s | %8.3f | %10.4f | %12.1f\n",
        "fa2_simplified (stub)",
        (double)ms_fa2, tflops_fa2, (double)bw_gbps);

    std::puts("\n目标（实现 TODO 后）：");
    std::fprintf(stdout,
        "  - max_rel_err < 1e-2（FP16 精度容限）\n"
        "  - TFLOPS ≥ 官方 FA-2 的 70%%（取决于 GPU）\n"
        "  - Tensor Core 利用率 ≥ 50%%（可用 mma.sync 实现）\n");

    std::puts("\nNsight Compute 分析命令：");
    std::puts("  ncu --set full -o capstone_b.ncu-rep ./CAPSTONE2_B_flash_attention_replica");
    std::puts("  ncu --import capstone_b.ncu-rep | grep -E \"Tensor|smem|l2\"");
    std::puts("\n关键指标：");
    std::puts("  l1tex__data_pipe_lsu_wavefronts_mem_shared_op_ld.sum  (smem 读)");
    std::puts("  l1tex__data_bank_conflicts_pipe_lsu_mem_shared_op_ld.sum (bank conflict)");
    std::puts("  sm__pipe_tensor_cycles_active.avg.pct_of_peak_sustained_active (TC 利用率)");

    // ── 释放资源 ─────────────────────────────────────────────
    CUDA_CHECK(cudaFree(d_Q));
    CUDA_CHECK(cudaFree(d_K));
    CUDA_CHECK(cudaFree(d_V));
    CUDA_CHECK(cudaFree(d_O_fa2));

    std::puts("\n[CAPSTONE2_B] 完成。");
    return 0;
}
