# CUTLASS `48_hopper_warp_specialized_gemm` 精读笔记

> 本文件是分支 A 的**源码精读分析模板**，供学生逐节填写。
> 所有 `> TODO 填入...` 块均需替换为实际分析内容，最终字数应 ≥ 1500 字。
> 完成后作为交付物之一提交。

---

## 0. 概述

> TODO 填入：
> - 选定的 CUTLASS example 文件名（例如 `48_hopper_warp_specialized_gemm.cu`）
> - 问题规模：M / N / K，数据类型，block tile 大小
> - 为什么选这个 example（代码行数、覆盖的特性）
> - 与朴素 cuBLAS 调用相比，这个 kernel 的编写目的是什么

---

## 1. 5 层 Hierarchy 对照表

CUTLASS 的设计哲学是将 kernel 优化从"整体 kernel 调优"变为"分层参数调优"。
每层都有清晰的职责边界，下层实现对上层透明。

```
┌─────────────────────────────────────────┐
│  Device Layer: grid[x,y], block[x,y,z] │
│  (CTA scheduling, block tile (M,N))     │
└──────────────────┬──────────────────────┘
                   │
┌──────────────────┴──────────────────────┐
│  Kernel Layer: entry_point(A,B,D,...)   │
│  (thread layout, shared memory alloc)   │
└──────────────────┬──────────────────────┘
                   │
┌──────────────────┴──────────────────────┐
│  Collective Layer: CollectiveMainloop   │
│  (producer-consumer warp specialization)│
└──────────────────┬──────────────────────┘
                   │
┌──────────────────┴──────────────────────┐
│  Tiled MMA + Copy: mma_pipelined        │
│  (inner loop: TMA load → barrier        │
│   → wgmma accumulate → barrier sync)   │
└──────────────────┬──────────────────────┘
                   │
┌──────────────────┴──────────────────────┐
│  Atom Layer: PTX primitives             │
│  (cp.async.bulk.tensor, wgmma.mma_async)│
└─────────────────────────────────────────┘
```

| 层级 | 对应类/文件 | 文件路径 | 关键行号 | 职责 |
|------|------------|----------|----------|------|
| Device Layer | `GemmUniversalAdapter` | `cutlass/gemm/device/gemm_universal_adapter.h` | > TODO 填入行号 | grid 划分、kernel 参数打包 |
| Kernel Layer | `GemmUniversal` | `cutlass/gemm/kernel/gemm_universal.hpp` | > TODO 填入行号 | kernel entry，smem 分配，thread layout |
| Collective Layer | `CollectiveMainloop` | `cutlass/gemm/collective/...` | > TODO 填入行号 | producer-consumer warp 分工、pipeline |
| Tiled MMA+Copy | `TiledMMA`, `TiledCopy` | `cute/algorithm/...` | > TODO 填入行号 | 内层循环、TMA copy atom、wgmma atom |
| Atom Layer | PTX 内联汇编 | `cute/arch/mma_sm90.hpp` | > TODO 填入行号 | `wgmma.mma_async`, `cp.async.bulk.tensor` |

> TODO 填入：每层的职责说明（3–5 句话），数据流向（从哪里来、到哪里去）

---

## 2. CollectiveMainloop 展开

### 2.1 TMA Load 路径

> TODO 填入：
> - `cuTensorMapEncodeTiled` 的调用位置（文件 + 行号）
> - `CUtensorMap` 包含哪些信息（rank、globalDim、globalStride、boxDim、swizzle）
> - producer warp 如何发出 `cp.async.bulk.tensor`（代码片段 + 注释）
> - 一条 TMA 请求传输多少字节？需要多少条指令？

```cpp
// TODO 填入：producer warp 的 TMA load 伪代码（含 barrier arrive）
```

### 2.2 wgmma 计算路径

> TODO 填入：
> - consumer warp-group 如何等待 `mbarrier`（`mbarrier.wait` 指令语法）
> - `wgmma.mma_async.sync.aligned` 的参数（m, n, k, dtype, a_desc, b_desc, accum）
> - 一次 wgmma 处理的矩阵形状（m16n8k16 还是更大？）
> - 循环展开深度（unroll factor）与寄存器占用的关系

```ptx
// TODO 填入：wgmma 调用的内联 PTX 片段
```

### 2.3 mbarrier 同步机制

> TODO 填入：
> - mbarrier 的 arrive/wait 语义（与 `__syncthreads` 的本质区别）
> - pipeline depth = N 时，需要几个 mbarrier 槽位？
> - arrive count 如何设置（与 TMA 事务数对应）
> - complete_tx 和 arrive 的区别

### 2.4 Producer-Consumer 流水线时间线图

```
时间 →
         Stage 0          Stage 1          Stage 2
Producer: TMA_load[0] --  TMA_load[1] --  TMA_load[2]
                      |              |
Consumer:         wgmma[0]      wgmma[1]      wgmma[2]
```

> TODO 填入：
> - 实际采集到的 timeline（可用 Nsight Systems NVTX 截图或文字描述）
> - producer 和 consumer 的重叠比例（理论 vs 实测）
> - pipeline 深度 = 2 vs = 3 时的延迟隐藏效果差异

---

## 3. Shared Memory Layout 与 Swizzle

### 3.1 A tile 和 B tile 的 smem 布局

> TODO 填入（含 ASCII art 示意图）：
> - A tile（BM × BK，FP16）占用 smem 多少字节？
> - B tile（BK × BN，FP16）占用 smem 多少字节？
> - 两者的 smem 起始 offset（以 bytes 为单位）
> - pipeline depth = 2 时总 smem 占用

```
smem 布局示意（以 BM=64, BK=64, BN=128 为例）：

offset 0:      [A_tile stage0: 64×64×2 = 8 KB]
offset 8192:   [A_tile stage1: 64×64×2 = 8 KB]
offset 16384:  [B_tile stage0: 64×128×2 = 16 KB]
offset 32768:  [B_tile stage1: 64×128×2 = 16 KB]
offset 49152:  [mbarrier × 2: 16 bytes]
总计：TODO 填入实际值

> TODO 填入实际 smem 分配（从 CUTLASS 源码提取）
```

### 3.2 Swizzle 规则

> TODO 填入：
> - CUTLASS 使用的 swizzle 模式（`SWIZZLE_128B` / `SWIZZLE_64B` / `SWIZZLE_32B`）
> - swizzle 如何将连续行的同一列映射到不同 bank，避免 bank conflict
> - TMA 的 swizzle 参数（`CU_TENSOR_MAP_SWIZZLE_128B`）与 smem 的 `__align__` 关系
> - 用 Nsight Compute 验证 bank conflict 为 0 的截图或数值

```
Swizzle 示意：
bank:  0   1   2   ...  31
row0: [元素0, 元素1, ..., 元素31]   ← 未 swizzle：全部命中 bank 0–31
row1: [元素32, ...]                  ← 与 row0 的 bank 分布相同 → conflict!

Swizzle 后：
row0: [→ bank 0, → bank 1, ...]     ← 按 swizzle 函数重新映射
row1: [→ bank 8, → bank 9, ...]     ← 错开 bank
```

> TODO 填入：实际 swizzle 函数的数学定义（从 CUTLASS cute/swizzle.hpp 提取）

---

## 4. 寄存器分配（setmaxnreg）

> TODO 填入：
> - Hopper warp-specialized kernel 中 `setmaxnreg` 的调用位置
> - producer warp 申请多少寄存器（通常较少，因为只做 TMA）
> - consumer warp-group 申请多少寄存器（累加器 + 中间变量）
> - 寄存器数量与 occupancy 的权衡：寄存器多 → occupancy 低，但 spill 少
> - 用 Nsight Compute 验证 register spill 为 0 的数值

```
setmaxnreg 调用位置：TODO 填入（文件 + 行号）
producer max registers：TODO（例如 40）
consumer max registers：TODO（例如 232）

理论计算：
  SM 寄存器总数：65536
  consumer 232 regs × 128 threads = 29696 regs/CTA
  最大并发 CTA 数：65536 / 29696 ≈ 2 CTA/SM（occupancy = 2/4 = 50%）
  TODO：填入实际 occupancy（ncu 报告的 Achieved Occupancy）
```

---

## 5. Cluster Launch 配置

> TODO 填入：
> - cluster shape 的选择原则（例如 (1, 2, 1) vs (2, 1, 1)）
> - `cudaLaunchKernelEx` 的参数（cudaLaunchConfig_t 结构）
> - cluster 内 CTA 如何通过 distributed shared memory 共享数据
> - cluster launch 相比普通 launch 的额外开销（调度延迟）
> - Nsight Systems timeline 中 cluster 的可视化方法

---

## 6. 性能对比表

> TODO 填入（实测数据，需要 Nsight Compute 采集）：

| 指标 | CUTLASS CollectiveBuilder | 手写 warp-specialized | 差距原因 |
|------|--------------------------|----------------------|---------|
| 平均耗时 (ms) | TODO | TODO | — |
| TFLOPS | TODO | TODO | — |
| % 理论峰值 | TODO | TODO | — |
| Tensor Core 利用率 | TODO | TODO | TODO 分析 |
| L2 命中率 | TODO | TODO | TODO 分析 |
| 寄存器 spill | TODO | TODO | TODO 分析 |
| Warp 占用率 | TODO | TODO | TODO 分析 |

Nsight Compute 采集命令：
```bash
ncu --set full -o capstone_a.ncu-rep ./CAPSTONE2_A_cutlass_deep_dive
ncu --import capstone_a.ncu-rep | grep -E "Tensor|sm__pipe|l2_"
```

---

## 7. 瓶颈分析

> TODO 填入（基于 Nsight Compute 数据）：
> - 当前实现的主要瓶颈（计算 / L1 带宽 / L2 带宽 / HBM 带宽 / 延迟）
> - Roofline 模型：本 kernel 的算术强度（FLOPs/byte）落在哪个区间？
> - 如果把 block tile 从 (64, 128) 改为 (128, 256)，occupancy 和 performance 如何变化？
> - pipeline depth = 2 vs = 3 的实测差异（如果有时间做对比）

```
Roofline 分析（手工计算）：
  FLOPs = 2 × M × N × K = 2 × 4096³ ≈ 137 TFLOPS（以 4096 为例）
  数据量 = (M×K + K×N) × sizeof(fp16) + M×N × sizeof(fp32)
         = TODO KB/GB
  算术强度 = TODO FLOPS/byte

  H100 SXM5：
    FP16 Tensor Core 峰值：1979 TFLOPS
    HBM 带宽峰值：3.35 TB/s
    Roofline 拐点：1979e12 / 3.35e12 ≈ 590 FLOPS/byte
  → 本 kernel 算术强度 TODO > 590 → 计算受限（Tensor Core 成瓶颈）
```

---

## 8. 收获与疑问

### 收获

> TODO 填入至少 5 条具体收获（不是泛泛的"学到了很多"），例如：
> 1. 第一次理解了 `mbarrier.arrive` 与 `__syncthreads` 的本质差异：前者是异步通知，后者是全量同步屏障……
> 2. ……

### 疑问

> TODO 填入至少 3 条疑问，以及尝试解答：
> 1. Q：为什么 pipeline depth > 3 反而可能降低性能？
>    A（尝试）：……
> 2. ……

---

## 9. 关键设计决策总结

| 决策 | 原因 | 性能影响 |
|------|------|---------|
| Warp specialization | producer 专注 TMA，consumer 专注 wgmma，无竞争 | 提升 Tensor Core 利用率 ~20–30% |
| TMA 代替 cp.async | 多维硬件流水，零 warp stall | 隐藏 HBM → L2 → smem 全链路延迟 |
| mbarrier 代替 __syncthreads | 异步语义，允许 pipeline depth > 1 | 使 wgmma 与下一 tile TMA 重叠 |
| 128B swizzle | 消除 smem bank conflict | smem 带宽提升 TODO × |
| cluster (1,2,1) | 两个 CTA 共享 smem（distributed）| 减少 B tile 的 HBM 重复读取 |
| TODO 填入第 6 条 | TODO | TODO |

---

*笔记模板版本：2025-05。请在完成精读后删除所有 `> TODO` 提示行，保留实际内容。*
