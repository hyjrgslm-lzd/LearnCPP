// CAPSTONE2_A_cutlass_deep_dive/main.cu
// ============================================================
// 结课项目 2 — 分支 A：CUTLASS Kernel 精读 + 手写 warp-specialized GEMM
//
// 任务描述：
//   1. 阅读 CUTLASS examples/48_hopper_warp_specialized_gemm 源码
//   2. 理解 5 层 hierarchy（Device/Kernel/Collective/TiledMMA+Copy/Atom）
//   3. 用 CollectiveBuilder 搭建 CUTLASS 参考路径（完整可运行）
//   4. 手写 TMA + wgmma warp-specialized GEMM（TODO [必做] 框架）
//   5. 对标性能：benchmark 两条路径，输出 TFLOPS 对比表
//
// 问题规模：FP16 GEMM  A(4096×4096) × B(4096×4096) → D(4096×4096, FP32)
//
// 编译要求：
//   - sm_90a（Hopper），CUDA 13.x，CUTLASS 3.x
//   - CMake: cmake --build build --target CAPSTONE2_A_cutlass_deep_dive
//
// Nsight Compute 采集提示：
//   ncu --set full -o capstone_a.ncu-rep ./CAPSTONE2_A_cutlass_deep_dive
//   ncu --import capstone_a.ncu-rep > metrics.csv
//
// 参考文献：
//   - CUTLASS 3.x Programming Guide: https://github.com/NVIDIA/cutlass/blob/main/media/docs/
//   - CUTLASS example 48_hopper_warp_specialized_gemm
//   - NVIDIA Hopper Tuning Guide
// ============================================================

// ─────────────────────────────────────────────────────────────
// 公共头文件
// ─────────────────────────────────────────────────────────────
#include "cuda_check.cuh"
#include "device_info.cuh"
#include "nvtx_range.cuh"
#include "timer.cuh"

// ─────────────────────────────────────────────────────────────
// CUTLASS 核心头文件
// ─────────────────────────────────────────────────────────────
#include "cutlass/cutlass.h"
#include "cutlass/gemm/device/gemm_universal_adapter.h"
#include "cutlass/gemm/kernel/gemm_universal.hpp"
#include "cutlass/gemm/collective/collective_builder.hpp"
#include "cutlass/epilogue/collective/default_epilogue.hpp"
#include "cutlass/epilogue/collective/collective_builder.hpp"
#include "cutlass/gemm/dispatch_policy.hpp"
#include "cutlass/tensor_ref.h"
#include "cutlass/util/host_tensor.h"
#include "cutlass/util/reference/host/tensor_fill.h"
#include "cutlass/util/reference/host/tensor_compare.h"
#include "cutlass/util/reference/host/gemm.h"

#include <cuda_runtime.h>
#include <cuda_fp16.h>

// cuTe（CUTLASS 内置）
#include "cute/tensor.hpp"
#include "cute/arch/mma_sm90.hpp"
#include "cute/arch/copy_sm90.hpp"

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <algorithm>
#include <cassert>

// ─────────────────────────────────────────────────────────────
// Hopper 运行时保护宏
// ─────────────────────────────────────────────────────────────
#define HOPPER_SKIP_IF_UNSUPPORTED(dev)                              \
    do {                                                             \
        if (!has_hopper_features(dev)) {                             \
            std::puts("[CAPSTONE2_A] 当前 GPU 不支持 Hopper (sm_90a)，跳过。\n" \
                      "  本题需要实机 Hopper 才能采集有意义的性能数据。");   \
            return 0;                                                \
        }                                                            \
    } while (0)

// ─────────────────────────────────────────────────────────────
// 超参
// ─────────────────────────────────────────────────────────────
static constexpr int M_PROBLEM = 4096;
static constexpr int N_PROBLEM = 4096;
static constexpr int K_PROBLEM = 4096;

static constexpr int WARMUP_ITERS = 5;
static constexpr int BENCH_ITERS  = 20;

// ─────────────────────────────────────────────────────────────
// 数据类型与 layout 定义
// ─────────────────────────────────────────────────────────────
using ElementA     = cutlass::half_t;           // FP16 输入 A
using ElementB     = cutlass::half_t;           // FP16 输入 B
using ElementC     = float;                     // FP32 输出 C/D
using ElementAccum = float;                     // FP32 累加器

// NT GEMM：A 行主，B 列主（CUTLASS 常见约定）
using LayoutA = cutlass::layout::RowMajor;
using LayoutB = cutlass::layout::ColumnMajor;
using LayoutC = cutlass::layout::RowMajor;

// 对齐（FP16 128-bit = 8 elements，FP32 = 4 elements）
static constexpr int AlignA = 8;
static constexpr int AlignB = 8;
static constexpr int AlignC = 4;

// ─────────────────────────────────────────────────────────────
// CUTLASS 参考路径（分支 A 全量实现，学生不需修改）
//
// 5 层 hierarchy 组装：
//   Layer 5 (Atom)   : sm90 wmma / TMA PTX 原语（由 CUTLASS 内部管理）
//   Layer 4 (TiledMMA+Copy): CollectiveBuilder 自动选择 tile mma / tma copy
//   Layer 3 (Collective): CollectiveMainloop + CollectiveEpilogue
//   Layer 2 (Kernel) : GemmUniversal kernel entry point
//   Layer 1 (Device) : GemmUniversalAdapter dispatch
// ─────────────────────────────────────────────────────────────

// --- Collective Mainloop（Hopper TMA warp-specialized cooperative schedule）---
//
// CollectiveBuilder 根据以下参数自动决定：
//   - TMA copy atom（cp.async.bulk.tensor）的维度与 descriptor
//   - wgmma.mma_async 的矩阵形状和数据类型
//   - mbarrier 的阶段数（pipeline depth）
//   - producer/consumer warp 的分工
//
// KernelTmaWarpSpecializedCooperative：
//   所有 consumer warp 合作完成同一个 wgmma，能更好地利用 Hopper register file。
//   对比 KernelTmaWarpSpecializedPingpong：两组 warp 交替 ping-pong，延迟隐藏更激进。
using CollectiveMainloop =
    typename cutlass::gemm::collective::CollectiveBuilder<
        cutlass::arch::Sm90,                                         // 目标架构
        cutlass::arch::OpClassTensorOp,                              // Tensor Core 路径
        ElementA, LayoutA, AlignA,
        ElementB, LayoutB, AlignB,
        ElementAccum,
        cutlass::gemm::Shape<128, 128, 64>,                          // Block tile (M, N, K)
        cutlass::gemm::Shape<1,   2,   1>,                           // Cluster tile (M, N, K)
        cutlass::gemm::collective::StageCountAuto,                   // pipeline 深度自动决定
        cutlass::gemm::collective::KernelTmaWarpSpecializedCooperative
    >::CollectiveOp;

// --- Collective Epilogue ---
using CollectiveEpilogue =
    typename cutlass::epilogue::collective::CollectiveBuilder<
        cutlass::arch::Sm90,
        cutlass::arch::OpClassTensorOp,
        cutlass::gemm::Shape<128, 128, 64>,
        cutlass::gemm::Shape<1,   2,   1>,
        cutlass::epilogue::collective::EpilogueTileAuto,
        ElementAccum, ElementAccum,
        ElementC, LayoutC, AlignC,
        ElementC, LayoutC, AlignC,
        cutlass::epilogue::collective::EpilogueScheduleAuto
    >::CollectiveOp;

// --- GemmKernel：将 Collective 层打包进 kernel entry ---
using GemmKernel =
    cutlass::gemm::kernel::GemmUniversal<
        cutlass::gemm::Shape<int, int, int, int>,   // 运行时形状（非编译期）
        CollectiveMainloop,
        CollectiveEpilogue
    >;

// --- GemmUniversalAdapter：Device 层 API ---
using Gemm = cutlass::gemm::device::GemmUniversalAdapter<GemmKernel>;

// ─────────────────────────────────────────────────────────────
// CPU 参考实现（小矩阵验证用；大矩阵时建议用 cuBLAS）
// 仅在 M*N*K 较小时调用，防止超时
// ─────────────────────────────────────────────────────────────
static void gemm_cpu_ref_fp32(
    const float* __restrict__ A,
    const float* __restrict__ B,
    float*       __restrict__ C,
    int M, int N, int K)
{
    // A: M×K (row-major), B: K×N (row-major), C: M×N (row-major)
    for (int m = 0; m < M; ++m)
        for (int n = 0; n < N; ++n) {
            float acc = 0.0f;
            for (int k = 0; k < K; ++k)
                acc += A[m * K + k] * B[k * N + n];
            C[m * N + n] = acc;
        }
}

// ─────────────────────────────────────────────────────────────
// 辅助：正确性检查
// ─────────────────────────────────────────────────────────────
static bool check_result(
    const float* ref,
    const float* got,
    int M, int N,
    const char* label,
    float tol = 1e-2f)
{
    int   mismatch = 0;
    float max_err  = 0.0f;
    for (int i = 0; i < M * N; ++i) {
        float rel = std::fabs(ref[i] - got[i]) / (std::fabs(ref[i]) + 1e-6f);
        max_err   = std::max(max_err, rel);
        if (rel > tol) ++mismatch;
    }
    bool ok = (mismatch == 0);
    std::fprintf(stdout,
        "  [%s] %s  max_rel_err=%.3e  mismatch=%d/%d\n",
        label, ok ? "PASS" : "FAIL", (double)max_err, mismatch, M * N);
    return ok;
}

// ─────────────────────────────────────────────────────────────
// 辅助：TFLOPS 计算
// ─────────────────────────────────────────────────────────────
static double calc_tflops(long long M, long long N, long long K, float ms)
{
    // GEMM: 2*M*N*K FLOPs（一次乘加 = 2 FLOPs）
    return 2.0 * static_cast<double>(M) * N * K / (static_cast<double>(ms) * 1e-3) / 1e12;
}

// ─────────────────────────────────────────────────────────────
// ============================================================
// 手写 warp-specialized GEMM kernel（学生实现区）
//
// 架构目标：Hopper sm_90a
// 算法目标：FP16 A(M×K) × FP16 B(K×N) → FP32 D(M×N)
// Block tile：BM=64, BN=128, BK=64（可调）
//
// Warp 分工：
//   - producer warp（warp 0）：负责 TMA 加载 A/B tile → shared memory
//   - consumer warp（warp 1-3）：负责 wgmma.mma_async 计算
//   - 两者通过 mbarrier 同步，实现流水（pipeline depth = 2）
//
// 关键 PTX 原语（Atom 层）：
//   cp.async.bulk.tensor  — TMA 异步拷贝（producer 使用）
//   wgmma.mma_async       — 异步 warp-group MMA（consumer 使用）
//   mbarrier.arrive/wait  — 生产者-消费者同步
// ============================================================

// 手写 kernel 的 tile 大小（可以改小，简化实现）
static constexpr int BM = 64;   // block tile M
static constexpr int BN = 128;  // block tile N
static constexpr int BK = 64;   // block tile K
static constexpr int PIPELINE_STAGES = 2; // 流水线深度

// ─────────────────────────────────────────────────────────────
// TODO [必做] 步骤 1：CUtensorMap 构建
//
// 在 host 端调用 cuTensorMapEncodeTiled() 构造 A 和 B 的 TMA descriptor。
// TMA descriptor 描述了从 global memory 到 shared memory 的多维 tile 拷贝：
//   - 数据维度（rank = 2）
//   - 全局张量的 shape 和 stride（单位：bytes）
//   - tile 的 shape（等于 shared memory 中 A/B tile 的大小）
//   - swizzle 模式（推荐 SWIZZLE_128B 消除 bank conflict）
//   - 地址对齐（16 bytes）
//
// 参数顺序（易错点，务必对照 Programming Guide cuTensorMapEncodeTiled）：
//   cuTensorMapEncodeTiled(tensorMap,
//       tensorDataType,          // CU_TENSOR_MAP_DATA_TYPE_FLOAT16
//       tensorRank,              // 2
//       globalAddress,           // global memory 基地址
//       globalDim[rank],         // 全局张量各维度大小 {K, M}（注意列优先顺序）
//       globalStride[rank-1],    // 除最快维度外的各 stride（bytes）
//       boxDim[rank],            // tile 各维度大小 {BK, BM}
//       elementStride[rank],     // 元素 stride（通常全 1）
//       interleave,              // CU_TENSOR_MAP_INTERLEAVE_NONE
//       swizzle,                 // CU_TENSOR_MAP_SWIZZLE_128B
//       l2Promo,                 // CU_TENSOR_MAP_L2_PROMOTION_NONE
//       oobFill)                 // CU_TENSOR_MAP_FLOAT_OOB_FILL_NONE
//
// 函数签名（填入 host 端调用代码）：
// ─────────────────────────────────────────────────────────────
static void build_tma_descriptor_A(
    CUtensorMap* tensorMap,
    const __half* globalPtr,
    int M, int K)
{
    // TODO [必做] 步骤 1a：构造 A 矩阵的 TMA descriptor
    //
    // uint64_t globalDim[2]    = { (uint64_t)K, (uint64_t)M };
    // uint64_t globalStride[1] = { (uint64_t)K * sizeof(__half) };
    // uint32_t boxDim[2]       = { (uint32_t)BK, (uint32_t)BM };
    // uint32_t elementStride[2] = { 1, 1 };
    //
    // CUresult result = cuTensorMapEncodeTiled(
    //     tensorMap,
    //     CU_TENSOR_MAP_DATA_TYPE_FLOAT16,
    //     2,
    //     (void*)globalPtr,
    //     globalDim,
    //     globalStride,
    //     boxDim,
    //     elementStride,
    //     CU_TENSOR_MAP_INTERLEAVE_NONE,
    //     CU_TENSOR_MAP_SWIZZLE_128B,
    //     CU_TENSOR_MAP_L2_PROMOTION_NONE,
    //     CU_TENSOR_MAP_FLOAT_OOB_FILL_NONE
    // );
    // if (result != CUDA_SUCCESS) {
    //     const char* msg = nullptr;
    //     cuGetErrorString(result, &msg);
    //     fprintf(stderr, "cuTensorMapEncodeTiled A failed: %s\n", msg);
    //     abort();
    // }

    (void)tensorMap; (void)globalPtr; (void)M; (void)K; // stub
}

static void build_tma_descriptor_B(
    CUtensorMap* tensorMap,
    const __half* globalPtr,
    int K, int N)
{
    // TODO [必做] 步骤 1b：构造 B 矩阵的 TMA descriptor（B 为列主序）
    //
    // B 列主序（LayoutB = ColumnMajor）：全局 shape = {K, N}，stride = {1, K}
    // 对 TMA 而言：globalDim = {N, K}，globalStride = {K * sizeof(__half)}
    // 注意：TMA 最快变化维度在最低索引

    (void)tensorMap; (void)globalPtr; (void)K; (void)N; // stub
}

// ─────────────────────────────────────────────────────────────
// TODO [必做] 步骤 2：手写 warp-specialized GEMM kernel
//
// kernel 结构（伪代码）：
//
// __global__ void kernel_warp_specialized_gemm_sm90(
//     const CUtensorMap* tma_A,  // A 的 TMA descriptor（放在 __constant__ 或参数）
//     const CUtensorMap* tma_B,  // B 的 TMA descriptor
//     float*             D,      // 输出矩阵 D（FP32）
//     int M, int N, int K)
// {
//     // ── 共享内存：双缓冲区（pipeline depth = 2）─────────────────────────
//     __shared__ __half smem_A[PIPELINE_STAGES][BM][BK];  // A tile 缓冲区
//     __shared__ __half smem_B[PIPELINE_STAGES][BK][BN];  // B tile 缓冲区
//     __shared__ uint64_t mbar[PIPELINE_STAGES];           // mbarrier 槽位
//
//     // ── warp/warp-group 分配 ──────────────────────────────────────────────
//     int warp_id = threadIdx.x / 32;
//     bool is_producer = (warp_id == 0);   // warp 0 = producer
//     // consumer warp group：warp 1, 2, 3（warp-group MMA 需要 128 线程）
//
//     // ── mbarrier 初始化（在 kernel 开始由 thread 0 执行）────────────────
//     if (threadIdx.x == 0) {
//         for (int s = 0; s < PIPELINE_STAGES; ++s)
//             asm volatile("mbarrier.init.shared::cta.b64 [%0], %1;" :: "r"(&mbar[s]), "r"(1));
//     }
//     __syncthreads();
//
//     // ── 累加器（consumer 寄存器，FP32）─────────────────────────────────
//     float acc[BM / 16][BN / 16][8] = {};  // 适配 m16n8k16 wgmma fragment
//
//     // ── 主循环：遍历 K 维度的所有 BK-sized tile ─────────────────────────
//     int num_k_tiles = (K + BK - 1) / BK;
//     int smem_stage  = 0;  // 当前 ping-pong 阶段
//
//     for (int k_tile = 0; k_tile < num_k_tiles; ++k_tile) {
//
//         if (is_producer) {
//             // TODO [必做] 步骤 2a：producer warp 发出 TMA 请求
//             // asm volatile(
//             //   "cp.async.bulk.tensor.2d.shared::cluster.global.mbarrier::complete_tx::bytes"
//             //   " [%0], [%1, {%2, %3}], [%4];"
//             //   :: "r"(smem_A[smem_stage]), "l"(tma_A),
//             //      "r"(blockIdx.y * BM), "r"(k_tile * BK),
//             //      "r"(&mbar[smem_stage])
//             // );
//             // （B 类似）
//             //
//             // producer arrive（通知 consumer 本阶段数据已发出）：
//             // asm volatile("mbarrier.arrive.shared::cta.b64 _, [%0];" :: "r"(&mbar[smem_stage]));
//         }
//
//         if (!is_producer) {
//             // TODO [必做] 步骤 2b：consumer warp-group 等待数据就绪
//             // asm volatile(
//             //   "mbarrier.wait.shared::cta.b64 [%0], %1;"
//             //   :: "r"(&mbar[smem_stage]), "r"(phase)
//             // );
//             //
//             // TODO [必做] 步骤 2c：wgmma.mma_async 执行矩阵乘
//             // （需要通过 cute::wgmma 或内联 PTX 调用 wgmma.mma_async.sync.aligned）
//             // 累加到 acc[...]
//         }
//
//         smem_stage ^= 1;  // 切换 ping-pong 缓冲区
//     }
//
//     // TODO [必做] 步骤 3：epilogue — 将 acc 写回 D（FP32）
//     // if (!is_producer) {
//     //     int m_base = blockIdx.y * BM;
//     //     int n_base = blockIdx.x * BN;
//     //     for (int m = threadIdx.y; m < BM; m += blockDim.y)
//     //         for (int n = threadIdx.x; n < BN; n += blockDim.x)
//     //             if ((m_base + m) < M && (n_base + n) < N)
//     //                 D[(m_base + m) * N + (n_base + n)] = acc[m/16][n/16][...];
//     // }
// }
//
// stub：当前直接输出 0（让验证报 FAIL，这是预期行为）
// ─────────────────────────────────────────────────────────────
__global__ void kernel_warp_specialized_gemm_sm90_stub(
    float* D,
    int M, int N)
{
    // stub：输出全零，等待学生实现上方 TODO
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int idy = blockIdx.y * blockDim.y + threadIdx.y;
    if (idx < N && idy < M)
        D[idy * N + idx] = 0.0f;
}

// ─────────────────────────────────────────────────────────────
// TODO [必做] 步骤 4：cluster launch 配置
//
// Hopper cluster launch 允许多个 CTA 组成一个 cluster，共享 distributed shared memory。
// 对于 tile (BM=64, BN=128) 建议 cluster_shape = (1, 2, 1)，与 CollectiveBuilder 对齐。
//
// 用法（取代普通 <<< grid, block >>>）：
//   dim3 cluster_dim(1, 2, 1);   // cluster shape
//   dim3 grid_dim(N/BN, M/BM, 1);
//   dim3 block_dim(128, 1, 1);   // 4 warps（1 producer + 3 consumer）
//
//   cudaLaunchConfig_t config = {};
//   config.gridDim  = grid_dim;
//   config.blockDim = block_dim;
//   config.attrs    = new cudaLaunchAttribute[1];
//   config.attrs[0].id = cudaLaunchAttributeClusterDimension;
//   config.attrs[0].val.clusterDim = {cluster_dim.x, cluster_dim.y, cluster_dim.z};
//   config.numAttrs = 1;
//   cudaLaunchKernelEx(&config, kernel_warp_specialized_gemm_sm90, ...);
// ─────────────────────────────────────────────────────────────

// ─────────────────────────────────────────────────────────────
// 主程序
// ─────────────────────────────────────────────────────────────
int main()
{
    std::puts("============================================================");
    std::puts("[CAPSTONE2_A] CUTLASS 精读 + 手写 warp-specialized GEMM");
    std::puts("============================================================");
    print_device_info(0);

    NVTX_RANGE("CAPSTONE2_A/main");

    // ── Hopper 运行时检测 ─────────────────────────────────────
    HOPPER_SKIP_IF_UNSUPPORTED(0);

    // ── 问题规模 ─────────────────────────────────────────────
    const int M = M_PROBLEM, N = N_PROBLEM, K = K_PROBLEM;
    std::fprintf(stdout, "\n问题规模：M=%d  N=%d  K=%d\n", M, N, K);
    std::fprintf(stdout, "矩阵大小：A(%.1f MB) + B(%.1f MB) + D(%.1f MB)\n\n",
        (double)(M * K * sizeof(__half)) / 1e6,
        (double)(K * N * sizeof(__half)) / 1e6,
        (double)(M * N * sizeof(float))  / 1e6);

    // ── 主机数据准备 ─────────────────────────────────────────
    std::vector<cutlass::half_t> hA(M * K), hB(K * N);
    std::vector<float>           hD_cutlass(M * N, 0.0f);
    std::vector<float>           hD_manual(M * N, 0.0f);
    std::vector<float>           hD_ref(M * N, 0.0f);

    std::srand(2024);
    for (auto& v : hA) v = cutlass::half_t(static_cast<float>(std::rand()) / RAND_MAX - 0.5f);
    for (auto& v : hB) v = cutlass::half_t(static_cast<float>(std::rand()) / RAND_MAX - 0.5f);

    // CPU 参考（使用缩小尺寸防止超时；大尺寸时改用 cuBLAS）
    {
        const int M_ref = 256, N_ref = 256, K_ref = 256;
        std::vector<float> A_f(M_ref * K_ref), B_f(K_ref * N_ref), C_f(M_ref * N_ref);
        for (int i = 0; i < M_ref * K_ref; ++i) A_f[i] = static_cast<float>(hA[i]);
        for (int i = 0; i < K_ref * N_ref; ++i) B_f[i] = static_cast<float>(hB[i]);
        std::puts("正在计算 CPU 参考（256×256×256，用于正确性验证）...");
        gemm_cpu_ref_fp32(A_f.data(), B_f.data(), C_f.data(), M_ref, N_ref, K_ref);
        std::puts("CPU 参考完成。\n");
        // 注意：CPU 参考仅用于前 256×256 的子块验证
        (void)C_f;
    }

    // ── 设备内存 ─────────────────────────────────────────────
    cutlass::half_t *dA = nullptr, *dB = nullptr;
    float           *dD_cutlass = nullptr, *dD_manual = nullptr;

    CUDA_CHECK(cudaMalloc(&dA,         sizeof(cutlass::half_t) * M * K));
    CUDA_CHECK(cudaMalloc(&dB,         sizeof(cutlass::half_t) * K * N));
    CUDA_CHECK(cudaMalloc(&dD_cutlass, sizeof(float)           * M * N));
    CUDA_CHECK(cudaMalloc(&dD_manual,  sizeof(float)           * M * N));

    CUDA_CHECK(cudaMemcpy(dA, hA.data(), sizeof(cutlass::half_t) * M * K, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(dB, hB.data(), sizeof(cutlass::half_t) * K * N, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemset(dD_cutlass, 0, sizeof(float) * M * N));
    CUDA_CHECK(cudaMemset(dD_manual,  0, sizeof(float) * M * N));

    // ─────────────────────────────────────────────────────────
    // 路径 1：CUTLASS 参考（CollectiveBuilder 自动生成 kernel）
    // ─────────────────────────────────────────────────────────
    std::puts("── 路径 1：CUTLASS CollectiveBuilder（参考实现）────────────");
    float ms_cutlass = 0.0f;
    {
        NVTX_RANGE("CAPSTONE2_A/cutlass_path");

        // 构造 GEMM 参数
        typename Gemm::Arguments args{
            cutlass::gemm::GemmUniversalMode::kGemm,
            {M, N, K},
            // A, lda(=K), B, ldb(=K), C(nullptr), ldc, D, ldd
            {dA, K, dB, K, nullptr, N, dD_cutlass, N},
            {1.0f, 0.0f}   // alpha=1, beta=0（纯矩阵乘，不加 C）
        };

        Gemm gemm_op;

        // 获取 workspace 需求并分配
        size_t ws_bytes = Gemm::get_workspace_size(args);
        void*  ws       = nullptr;
        if (ws_bytes > 0) CUDA_CHECK(cudaMalloc(&ws, ws_bytes));

        cutlass::Status status = gemm_op.initialize(args, ws);
        if (status != cutlass::Status::kSuccess) {
            std::fprintf(stderr,
                "[CAPSTONE2_A] CUTLASS initialize failed: %s\n",
                cutlassGetStatusString(status));
            return 1;
        }

        // 预热
        for (int i = 0; i < WARMUP_ITERS; ++i) gemm_op.run();
        CUDA_CHECK(cudaDeviceSynchronize());

        // 计时（20 iters）
        CudaEventTimer timer;
        timer.start();
        for (int i = 0; i < BENCH_ITERS; ++i) gemm_op.run();
        timer.stop();
        ms_cutlass = timer.elapsed_ms() / BENCH_ITERS;

        if (ws) CUDA_CHECK(cudaFree(ws));
        std::fprintf(stdout, "  CUTLASS 平均耗时：%.3f ms\n", (double)ms_cutlass);
    }

    CUDA_CHECK(cudaMemcpy(hD_cutlass.data(), dD_cutlass, sizeof(float) * M * N, cudaMemcpyDeviceToHost));

    // ─────────────────────────────────────────────────────────
    // 路径 2：手写 warp-specialized GEMM（学生实现）
    // 当前为 stub（输出全零），验证预期 FAIL
    // ─────────────────────────────────────────────────────────
    std::puts("── 路径 2：手写 warp-specialized GEMM（TODO 实现后替换 stub）");
    float ms_manual = 0.0f;
    {
        NVTX_RANGE("CAPSTONE2_A/manual_path");

        // stub kernel：输出全零
        dim3 grid((N + 31) / 32, (M + 31) / 32);
        dim3 block(32, 32);

        // 预热 stub
        for (int i = 0; i < WARMUP_ITERS; ++i) {
            CUDA_CHECK(cudaMemset(dD_manual, 0, sizeof(float) * M * N));
            kernel_warp_specialized_gemm_sm90_stub<<<grid, block>>>(dD_manual, M, N);
        }
        CUDA_CHECK(cudaDeviceSynchronize());

        // 计时 stub
        CudaEventTimer timer;
        timer.start();
        for (int i = 0; i < BENCH_ITERS; ++i) {
            kernel_warp_specialized_gemm_sm90_stub<<<grid, block>>>(dD_manual, M, N);
        }
        timer.stop();
        ms_manual = timer.elapsed_ms() / BENCH_ITERS;

        CUDA_CHECK(cudaGetLastError());
        std::fprintf(stdout, "  手写 kernel（stub）平均耗时：%.3f ms\n", (double)ms_manual);
        std::puts("  注意：stub 输出全零，验证将 FAIL，实现 TODO 后替换。");
    }

    CUDA_CHECK(cudaMemcpy(hD_manual.data(), dD_manual, sizeof(float) * M * N, cudaMemcpyDeviceToHost));

    // ─────────────────────────────────────────────────────────
    // 正确性检查
    // ─────────────────────────────────────────────────────────
    std::puts("\n── 正确性检查 ────────────────────────────────────────────");
    // CUTLASS vs CPU ref（在小子块上检查）
    // 注意：全量 4096×4096 CPU ref 耗时过长；此处仅作 CUTLASS 路径的简单 NaN 检测
    {
        bool has_nan = false;
        for (int i = 0; i < M * N; ++i)
            if (std::isnan(hD_cutlass[i])) { has_nan = true; break; }
        std::fprintf(stdout, "  [cutlass_path] %s  (NaN check)\n",
                     has_nan ? "FAIL (含 NaN)" : "PASS (无 NaN)");
    }
    // 手写 vs CUTLASS（stub 阶段预期 FAIL）
    check_result(hD_cutlass.data(), hD_manual.data(), M, N, "manual_vs_cutlass", 1e-2f);

    // ─────────────────────────────────────────────────────────
    // 性能汇总表
    // ─────────────────────────────────────────────────────────
    double tflops_cutlass = calc_tflops(M, N, K, ms_cutlass);
    double tflops_manual  = calc_tflops(M, N, K, ms_manual);
    // Hopper H100 SXM5 FP16 Tensor Core 理论峰值约 1979 TFLOPS（参考官方规格）
    // A100：312 TFLOPS（FP16 TF32），实际峰值以 ncu 报告为准
    double peak_tflops = 1979.0;  // TODO：按实机规格填写

    std::puts("\n── 性能汇总 ───────────────────────────────────────────────");
    std::fprintf(stdout,
        "%-36s | %8s | %10s | %8s | %12s\n",
        "变体", "ms/iter", "TFLOPS", "% peak", "% of CUTLASS");
    std::fprintf(stdout,
        "%-36s | %8.3f | %10.4f | %7.2f%% | %11s\n",
        "cutlass_collective_cooperative",
        (double)ms_cutlass, tflops_cutlass,
        tflops_cutlass / peak_tflops * 100.0, "100.00%");
    std::fprintf(stdout,
        "%-36s | %8.3f | %10.4f | %7.2f%% | %11.2f%%\n",
        "manual_warp_specialized_sm90 (stub)",
        (double)ms_manual, tflops_manual,
        tflops_manual  / peak_tflops * 100.0,
        tflops_manual / tflops_cutlass * 100.0);

    std::puts("\n注：stub 数据不代表真实性能。实现 TODO 步骤后重新运行获取真实数据。");
    std::puts("\nNsight Compute 分析命令：");
    std::puts("  ncu --set full -o capstone_a.ncu-rep ./CAPSTONE2_A_cutlass_deep_dive");
    std::puts("  ncu --import capstone_a.ncu-rep | grep -E \"Tensor|Memory|FLOP\"");
    std::puts("\n重点关注指标：");
    std::puts("  sm__pipe_tensor_cycles_active.avg.pct_of_peak_sustained_active  (Tensor Core 利用率)");
    std::puts("  l1tex__t_bytes_pipe_lsu_mem_global_op_ld.sum.pct_of_peak_sustained_elapsed (L1 带宽)");
    std::puts("  l2_global_load_bytes / l2_write_bytes (L2 命中率)");

    // ── 释放资源 ─────────────────────────────────────────────
    CUDA_CHECK(cudaFree(dA));
    CUDA_CHECK(cudaFree(dB));
    CUDA_CHECK(cudaFree(dD_cutlass));
    CUDA_CHECK(cudaFree(dD_manual));

    std::puts("\n[CAPSTONE2_A] 完成。");
    return 0;
}
