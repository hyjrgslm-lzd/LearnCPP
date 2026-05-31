# 练习 H2：gemm_warp_tile_and_mma

## 1. 目标

`[H2-T01]` (main.cu:3) 练习 H2：Warp Tile + `mma.sync m16n8k16` Tensor Core GEMM。
`[H2-T02]` (main.cu:5) 目标：在 H1 shared-memory tiled 版本基础上，引入 warp tile（每 warp 负责 16×16 的 C 子 tile），通过 `ldmatrix` 从 smem 加载 fragment，用 `mma.sync.aligned.m16n8k16` 执行 Tensor Core 计算，对比与 H1 的 TFLOPS 差距。
`[H2-T03]` (main.cu:11) 编译要求：sm_80+，CUDA 13.x，C++20 device / C++26 host。

在 H1 shared-memory tiled 版本基础上，进一步分解：每个 warp 负责 16×16 的 warp tile（C 的子 tile）。使用 `mma.sync.aligned.m16n8k16` 指令（PTX 内联），通过 `ldmatrix` 从 shared memory 加载矩阵 fragment 到寄存器，然后用 Tensor Core 计算。这一步是从“普通 GEMM 优化”向“Tensor Core 特化”的转折。

## 2. 前置知识

- 完成 H1，理解 shared memory tile 版本的结构
- 理解 wmma / mma.sync 的基础用法（模块 G1/G2）
- 理解寄存器 fragment 的概念（fragment 存在寄存器中，访问速度远超 global/shared memory）

## 3. 硬件要求

- sm_80+（Ampere/Ada/Hopper）
- sm_70（Volta）支持 mma.sync m16n16k16 但形状略有不同
- CUDA Toolkit 13.x+

## 4. 文件说明

| 文件 | 说明 |
|------|------|
| `main.cu` | FP32 Tiled 基线 + Warp MMA (FP16→FP32) 版本 |
| `CMakeLists.txt` | CUDA_ARCHITECTURES = 80;86;89;90a |
| `README.md` | 本文件 |

## 5. 必做任务

### 步骤 1 — 改写 block tile 分解

`[H2-T04]` (main.cu:28) 常量与超参。
`[H2-T05]` (main.cu:30) block tile：每个 block 负责 BM×BN 的 C tile。
`[H2-T06]` (main.cu:31) `BM`：block M 维。
`[H2-T07]` (main.cu:32) `BN`：block N 维。
`[H2-T08]` (main.cu:33) `BK`：block K（每次从 global 加载的 K 宽）。
`[H2-T09]` (main.cu:34) warp tile：每个 warp 负责 WM×WN（与 mma shape 对齐）。
`[H2-T10]` (main.cu:35) `WM`：warp M（mma m 维度）。
`[H2-T11]` (main.cu:36) `WN`：warp N（两次 mma n=8 合成 16）。
`[H2-T12]` (main.cu:41) 版本 1（基线）：从 H1 继承的 Tiled GEMM（FP32 ALU）。
`[H2-T13]` (main.cu:56) TODO [必做] 步骤 1：从 H1 复制 tiled GEMM 逻辑作为 FP32 基线。
`[H2-T14]` (main.cu:62) stub。
`[H2-T15]` (main.cu:65) 版本 2：Warp Tile + mma.sync（FP16 输入，FP32 累加）。
`[H2-T16]` (main.cu:76) `A`：`[M x K]`，行主，FP16。
`[H2-T17]` (main.cu:77) `B`：`[K x N]`，列主，FP16（注意 layout！）。
`[H2-T18]` (main.cu:78) `C`：`[M x N]`，行主，FP32。
`[H2-T19]` (main.cu:81) TODO [必做] 步骤 1：声明 smem tile（FP16，以便 ldmatrix 对齐）。
`[H2-T20]` (main.cu:87) TODO [必做] 步骤 2：确定 warp id 与 warp 内 lane id。
`[H2-T21]` (main.cu:94) TODO [必做] 步骤 2（续）：每个 warp 的 C tile 行/列起始。
`[H2-T22]` (main.cu:101) TODO [必做] 步骤 3：声明 C/D 累加 fragment（FP32，m16n8 × 2）。
`[H2-T23]` (main.cu:103) stub 声明，实际 4 个 FP32/thread/mma。
`[H2-T24]` (main.cu:105) TODO [必做] 步骤 4：外层 K tile 循环。
`[H2-T25]` (main.cu:132) TODO [必做] 步骤 5：将 fragment 写回 global C（需要反算每 thread 对应的行/列）。
`[H2-T26]` (main.cu:138) stub：产生零输出。

保持 block tile 为 32×32（与 H1 相同），但现在把它分解为 2×2 个 warp tile，每个 warp tile = 16×16。`blockDim = (32, 8)` = 2 warps，每个 warp 处理不同行的 warp tile。

### 步骤 2 — ldmatrix 加载 fragment

从 shared memory 用 `ldmatrix.sync.aligned.x4.m8n8.shared.b16` 加载 A fragment（m16k16，128bit/thread），用 `ldmatrix.sync.aligned.x2.m8n8.shared.b16` 加载 B fragment（k16n8，64bit/thread）。

```ptx
// 加载 A fragment（4 个 32-bit 寄存器）
asm volatile(
  "ldmatrix.sync.aligned.x4.m8n8.shared.b16 {%0,%1,%2,%3}, [%4];"
  : "=r"(fragA[0]), "=r"(fragA[1]), "=r"(fragA[2]), "=r"(fragA[3])
  : "r"(smem_ptr_A));
```

### 步骤 3 — mma.sync m16n8k16

每个 warp tile（16×16）需要两次 mma.sync（n=8 × 2 = n=16）：

```ptx
// 第一次：n=8 左半
asm volatile(
  "mma.sync.aligned.m16n8k16.row.col.f32.f16.f16.f32 "
  "{%0,%1,%2,%3}, {%4,%5,%6,%7}, {%8,%9}, {%0,%1,%2,%3};"
  : "+r"(fragC[0][0]), "+r"(fragC[0][1]), "+r"(fragC[0][2]), "+r"(fragC[0][3])
  : "r"(fragA[0]), "r"(fragA[1]), "r"(fragA[2]), "r"(fragA[3]),
    "r"(fragB[0]), "r"(fragB[1]));
// 第二次：n=8 右半（fragB 偏移）
```

### 步骤 4 — 累加与 store 回 global

C fragment 累加在寄存器中，循环结束后按 thread 对应关系 store 回 global C。

### 步骤 5 — 测量 TFLOPS

`[H2-T38]` (main.cu:236) TODO [必做] 步骤 1 & 4：测量 FP32 Tiled 基线。
`[H2-T39]` (main.cu:251) TODO [必做] 步骤 5：测量 Warp MMA 版本。

应明显高于 H1 纯 ALU 版本（通常快 3-5 倍）。

### 步骤 6 — Nsight Compute Tensor Core 利用率

`[H2-T46]` (main.cu:289) TODO [必做] 步骤 6：Nsight Compute 查 Tensor Core Utilization。

```bash
ncu --metrics sm__pipe_tensor_cycles_active.avg.pct_of_peak_sustained_active \
    ./H2_gemm_warp_tile_and_mma
```

## 6. 进阶任务

`[H2-T47]` (main.cu:295) TODO [进阶] 尝试 `mma.sync.aligned.m16n8k32`（两个 BK=16 tile 合并）。
`[H2-T48]` (main.cu:299) TODO [进阶] 尝试 warp tile 16×32（需要四次 m16n8k16）。
`[H2-T49]` (main.cu:303) TODO [进阶] 对比手写 PTX `mma.sync` 与 wmma wrapper 生成代码差异。

- 尝试 `mma.sync.aligned.m16n8k32`（BK=32，一次加载两个 K tile），观察吞吐提升
- 尝试改变 warp tile 大小（16×32 或 32×16），测量吞吐变化并分析 mma.sync 调用次数变化
- 对比手写 PTX mma.sync 与 `#include <mma.h>` wmma wrapper 的生成代码差异（看 PTX 输出）

## 7. 验收标准

- kernel 编译通过，结果与 H1 逐元素一致（FP16 累加允许略松的容差 1e-2）
- TFLOPS 相比 H1 提升 3-5 倍
- Tensor Core Utilization > 80%
- ldmatrix 指令在 PTX 视图中可见，且与 mma.sync 正确配对

## 8. 常见坑

| 坑 | 说明 |
|----|------|
| ldmatrix / mma shape 不对应 | 混用 x2 / x4 导致 fragment 部分计算被忽略 |
| A/B layout 混淆 | A 行主（row.col），B 需要列主（col.row）才能配合 mma.sync m16n8k16 |
| warp tile 与 mma shape 不整除 | 某些 thread 处理不完整的 tile，导致越界写 |
| 寄存器压力 | 3 个 fragment（A、B、C）可能用掉 60-100 寄存器/thread，occupancy 下降 |
| sm_70 vs sm_80 差异 | sm_70 的 mma.sync shape 略有不同（m16n16k16），需要条件编译 |

## 9. 观察点

- Tensor Core 吞吐相比标量 ALU 快数倍，现代 GPU 高性能的关键来源
- fragment 住在寄存器中，访问速度是 global/shared memory 的 1000 倍
- warp tile 分解使每个 warp 独立承载一部分计算，提升并行性与寄存器利用率

## 10. 复盘问题

1. Tensor Core 相比标量 ALU，为什么能提升 3-5 倍吞吐？
2. warp tile 16×16 与 mma.sync m16n8k16 的关系是什么，为什么需要两次 mma_sync？
3. 如果 warp tile 改为 16×32，对 mma.sync 调用次数与寄存器占用各有什么影响？
4. 寄存器压力如何影响 occupancy 与最终性能？

## 参考资料

- CUDA C++ Programming Guide Section B.30 “Warp Matrix Functions”
- CUTLASS examples `examples/cute_tiled_mm`
- PTX ISA Guide Section 9.7.13 “Warp-Level Matrix Multiply-Accumulate Instructions”

## 输出对照（printf / std::puts 原文）

- `[H2-T27]` (main.cu:144) CPU 参考实现。
- `[H2-T28]` (main.cu:170) 原文：`FP16 累加容差略松` → 现：`FP16 accumulation: looser tolerance`
- `[H2-T29]` (main.cu:174) 原文：`最大相对误差 ... 不一致元素` → 现：`max_rel_err ... mismatch`
- `[H2-T30]` (main.cu:184) `main` 入口。
- `[H2-T31]` (main.cu:194) 原文：`问题规模：M=%d  N=%d  K=%d` → 现：`Problem size: M=%d  N=%d  K=%d`
- `[H2-T32]` (main.cu:197) 主机内存。
- `[H2-T33]` (main.cu:213) 原文：`正在计算 CPU 参考（FP32）...` → 现：`Computing CPU reference (FP32)...`
- `[H2-T34]` (main.cu:217) 设备内存。
- `[H2-T35]` (main.cu:230) 启动配置。
- `[H2-T36]` (main.cu:231) 注释：(32, 32)。
- `[H2-T37]` (main.cu:232) 注释：2 warps per block, 32 threads per warp。
- `[H2-T40]` (main.cu:275) 正确性检查。
- `[H2-T41]` (main.cu:277) 原文：`── 正确性检查（stub 阶段预期 FAIL）──` → 现：`-- Correctness check (FAIL expected at stub stage) --`
- `[H2-T42]` (main.cu:282) 性能汇总。
- `[H2-T43]` (main.cu:286) 原文：`FP32 占位` → 现：`FP32 placeholder`。
- `[H2-T44]` (main.cu:289) 原文：`── 性能汇总 ──...` → 现：`-- Performance summary --`
- `[H2-T45]` (main.cu:296) 原文：`加速比 (mma / tiled_fp32): %.2fx` → 现：`speedup (mma / tiled_fp32): %.2fx`
- `[H2-T50]` (main.cu:325) 原文：`[H2] 完成。` → 现：`[H2] done.`
