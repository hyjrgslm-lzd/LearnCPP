# 练习 C1：block_warp_thread_layout

## 目标

`[C1-T01]` (main.cu:1) 文件标识：`C1_block_warp_thread_layout/main.cu`。
`[C1-T02]` (main.cu:2) 练习 C1：block / warp / thread 三层布局与 occupancy 计算。
`[C1-T03]` (main.cu:4) 学习目标：掌握 `blockIdx` / `threadIdx` 编号；用 `threadIdx.x/32` 得到 warp ID，`threadIdx.x%32` 得到 lane ID；对多种 `blockDim` 计算理论 occupancy，并与 `cudaOccupancyMaxActiveBlocksPerMultiprocessor` 对比。
`[C1-T04]` (main.cu:9) 编译与运行命令；以及 Nsight Compute 采集命令。

通过手动计算与代码验证，深入理解 block、warp、thread 三层映射，以及如何根据计算模式选择合适的 blockDim。

## 前置理解

- grid/block 启动时的 `blockIdx`、`threadIdx` 编号方式
- 一个 block 内最多 1024 个线程
- warp 总是 32 个线程，无法配置
- 共享内存大小限制（典型 96 KB per block）

## 必做任务

`[C1-T05]` (main.cu:25) 数据结构：每个线程记录自身身份信息。
`[C1-T06]` (main.cu:27) 字段 `block_idx`：`blockIdx.x`。
`[C1-T07]` (main.cu:28) 字段 `thread_idx`：`threadIdx.x`（线性化后）。
`[C1-T08]` (main.cu:29) 字段 `warp_id`：`threadIdx_linear / 32`。
`[C1-T09]` (main.cu:30) 字段 `lane_id`：`threadIdx_linear % 32`。
`[C1-T10]` (main.cu:34) Kernel：记录每个线程的 block/warp/lane 信息。
`[C1-T11]` (main.cu:39) TODO [必做-2] 计算线性 `threadIdx`（支持三维 `blockDim`）。
`[C1-T12]` (main.cu:46) TODO [必做-2] 填写 warp ID 和 lane ID。
`[C1-T13]` (main.cu:53) Kernel：仅让 lane 0 打印（减少输出量）。
`[C1-T14]` (main.cu:64) TODO [必做-2] 只让 lane 0（`linear_tid % 32 == 0`）打印。
`[C1-T15]` (main.cu:69) TODO [必做-2] 取消注释，完成后应打印每个 warp 第一个 lane 的信息。
`[C1-T16]` (main.cu:77) 辅助：查询指定 `blockDim` 下的 occupancy。
`[C1-T17]` (main.cu:81) TODO [必做-5] 调用 `cudaOccupancyMaxActiveBlocksPerMultiprocessor`。
`[C1-T22]` (main.cu:113) 分配设备内存。
`[C1-T23]` (main.cu:117) TODO [必做-1/3/4] 尝试多种 `blockDim`，观察 warp 分布。
`[C1-T24]` (main.cu:124) TODO [必做-3] 计算该 `blockDim` 下的 warp 数量。
`[C1-T26]` (main.cu:131) 只在 `N >= bd` 时启动（演示 `blockDim <= N` 的情况）。
`[C1-T28]` (main.cu:152) TODO [必做-5/6] 查询 occupancy。
`[C1-T29]` (main.cu:157) TODO [必做-4] 二维 `blockDim` 示范。
`[C1-T30]` (main.cu:158) 三种 `blockDim` 配置对照：`(256,1,1)` / `(128,2,1)` / `(64,4,1)`。

1. 计算给定 blockDim 下的 warp 数量。例如 blockDim=(256,1,1) 时应有多少个 warp？
2. 写一个 kernel，使用 `threadIdx.x / 32` 计算 warp ID within block，`threadIdx.x % 32` 计算 lane ID within warp。打印每个 warp 的第一个 lane 的信息（lane 0 打一次）。
3. 验证：对于 blockDim=(128,1,1) 和 blockDim=(192,1,1)，分别输出 warp 计数和 lane 分布。
4. 为同一个计算写三种不同的 blockDim：(256,1,1)、(128,2,1)、(64,4,1)。在代码中标记出每种选择可能的用途（例如第二种更适合二维 tiling）。
5. 计算每种 blockDim 对应的 occupancy 限制因素。给定 96 KB shared memory 和每个 block 用 8 KB，最多能同时运行多少个 block？
6. 在 Nsight Compute 中用 `--set full` 运行，记录 Occupancy 数值，与手工计算对比。

## 进阶任务

`[C1-T32]` (main.cu:163) 配置 A：`(256,1,1)` — 典型 1D 配置。
`[C1-T34]` (main.cu:167) TODO 适合 1D 向量操作。
`[C1-T35]` (main.cu:169) 配置 B：`(128,2,1)` — 适合 2D tiling。
`[C1-T37]` (main.cu:173) TODO 适合二维数据的行/列分块。
`[C1-T38]` (main.cu:175) 配置 C：`(64,4,1)` — 更多 y 维度并行。
`[C1-T40]` (main.cu:179) TODO 适合 warp-tiling 场景，y 维度对应行。
`[C1-T41]` (main.cu:184) TODO [进阶] `blockDim=(32,32,1)` 的 2D 寻址。
`[C1-T42]` (main.cu:185) TODO [进阶] `cudaOccupancyMaxPotentialBlockSize` 查询最优 `blockDim`。

- 尝试 blockDim=(32,32,1)，在代码中标记出 2D 线程寻址的含义（block 内哪些线程被视为一个 warp）。
- 实验一个 blockDim=(1024,1,1) 的配置，观察 Nsight Compute 中 occupancy 的变化；同时用 `cudaOccupancyMaxPotentialBlockSize` 查询 occupancy 建议。

## 验收点

`[C1-T18]` (main.cu:107) 主程序入口。
`[C1-T19]` (main.cu:110) 打印设备基本信息。
`[C1-T20]` (main.cu:114) `N`：演示用，不需太大。
`[C1-T21]` (main.cu:115) `SMEM_PER_BLOCK`：假设每 block 用 8 KB smem。

- 代码能准确打印出每个 warp 的 warp ID 与 lane ID
- 三种 blockDim 的 occupancy 计算与实测数值在 Nsight Compute 中一致
- 能指出"occupancy 高不一定性能好"的例子（例如 occupancy 满但寄存器压力导致 spill）
- 复盘时能说清为什么 warp 大小固定为 32（硬件调度粒度）

## 观察点

- warp 是 SM 的调度最小单位，多个 warp 可在同一 SM 上分时复用
- blockDim 的选择不仅影响 occupancy，也影响 smem 布局、L1 缓存命中、寄存器压力
- Nsight Compute 的 Occupancy 数字是"理论上最多活跃 warp / 最大可能"的比值

## 常见坑

- 误以为 blockDim.x 可以随意设置，实际上必须满足"最多 1024 个线程"和"warp 对齐最优化"的权衡
- 计算 warp ID 时用错公式（应是整除，不是模运算）
- 没有考虑 smem 用量对 occupancy 的影响（smem 多 → block 少 → occupancy 低）
- 以为提高 occupancy 就能直接提升性能（实际需要看瓶颈：memory-bound 时 occupancy 帮助有限）
- 在二维 blockDim 下混淆线程的物理序列（以 x 为快变量还是 y 为快变量）
- 多块 block 竞争同一 SM 时，没有预期到 active block 数量受限
- blockDim 太小（例如 32）导致 occupancy 虽然满但 block 太多反而不高效
- 在 blockDim=(a,b,c) 三维配置下，没有正确理解 threadIdx 的物理顺序（linearization）

## 提示

- 线性化 threadIdx：`linearIdx = threadIdx.z * blockDim.x * blockDim.y + threadIdx.y * blockDim.x + threadIdx.x`
- warp ID 总是用整除 32：`warpId = linearIdx / 32`；lane ID 总是模 32：`laneId = linearIdx % 32`
- 验证 occupancy：已知每 block 使用的寄存器数、smem 字节数，用 `cudaOccupancyMaxPotentialBlockSize` 或 CUDA Programming Guide 的查表法
- Nsight Compute 的 "Theoretical Occupancy" 是 blockDim/smem/registers 的函数；"Achieved Occupancy" 是实测

## 复盘问题

- 给定一个 kernel 用了 100 寄存器/thread 和 16 KB smem，SM 上最多能运行多少个 block（假设 sm_90a 有 256 KB smem 和 256 寄存器/thread）？
- 如果你用 blockDim=(256,1,1)，实际上 GPU 看到了几个 warp？为什么必须是 32 的倍数比较好？
- warp 内的 32 个线程是串行的还是并行的？为什么说"warp 是 GPU 上同步的最小单位"？
- 为什么不能有 blockDim=(33,1,1) 这样的配置（从硬件角度）？

## 对应官方参考

- CUDA C++ Programming Guide Section 2.2: "Threads and Blocks"
- CUDA C++ Programming Guide Section 4.1: "Compute Capability"
- CUDA Runtime API: `cudaOccupancyMaxPotentialBlockSize`
- Nsight Compute: Occupancy Analysis section

## 输出对照（printf / std::puts 原文）

- `[C1-T25]` (main.cu:127) 原文：`[blockDim=%d] warp 数量 = %d` → 现：`[blockDim=%d] warp count = %d`
- `[C1-T27]` (main.cu:134) 原文：`启动配置: grid=%d block=%d` → 现：`launch config: grid=%d block=%d`
- `[C1-T31]` (main.cu:155) 原文：`--- 二维 blockDim 对照 ---` → 现：`--- 2D blockDim comparison ---`
- `[C1-T33]` (main.cu:160) 原文：`配置 A (256,1,1): grid=(%d,1,1)` → 现：`Config A (256,1,1): grid=(%d,1,1)`
- `[C1-T36]` (main.cu:165) 原文：`配置 B (128,2,1): grid=(%d,%d,1)` → 现：`Config B (128,2,1): grid=(%d,%d,1)`
- `[C1-T39]` (main.cu:170) 原文：`配置 C (64,4,1):  grid=(%d,%d,1)` → 现：`Config C (64,4,1):  grid=(%d,%d,1)`
- `[C1-T43]` (main.cu:189) 原文：`[C1] 完成。请在 Nsight Compute 中用 --set full 验证 occupancy 数值。` → 现：`[C1] done. Use Nsight Compute --set full to verify occupancy numbers.`
