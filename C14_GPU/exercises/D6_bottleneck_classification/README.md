# 练习 D6：bottleneck_classification

## 目标

`[D6-T01]` (main.cu:2) 练习 D6：瓶颈分类。
`[D6-T02]` (main.cu:3) memory-bound / compute-bound / latency-bound。

学会用 Nsight Compute 的指标和 roofline 模型，对给定的 kernel 进行瓶颈分类（compute-bound / memory-bound / latency-bound），并提出有针对性的优化方向。

## 前置理解

- 完成前五个练习，理解 warp 原语、occupancy 等概念
- 理解 roofline 模型的基础：peak throughput（计算或内存）、arithmetic intensity（计算量/内存访问量）
- 理解"Speed of Light"分析（能达到的最大吞吐）

## 必做任务

`[D6-T03]` (main.cu:5) 学习目标。
`[D6-T04]` (main.cu:6) 3 个对比 kernel：scan（内存密集）/ matmul_tile（计算密集）/ atomic_hist（延迟密集）。
`[D6-T05]` (main.cu:7) Nsight Compute Speed-of-Light：SM / L1 / L2 / DRAM Throughput。
`[D6-T06]` (main.cu:8) roofline 模型：arithmetic intensity = ops / bytes。
`[D6-T07]` (main.cu:9) 根据瓶颈分类提出有针对性的优化方向。
`[D6-T09]` (main.cu:26) 4M 元素（scan / hist）。
`[D6-T10]` (main.cu:27) matmul tile 大小。
`[D6-T11]` (main.cu:28) 矩阵尺寸（512×512）。
`[D6-T12]` (main.cu:30) 直方图桶数。
`[D6-T13]` (main.cu:33) Kernel 1：memory-bound scan（前缀和近似）。
`[D6-T14]` (main.cu:34) 特点：高 DRAM / L2 带宽利用率，极低计算强度。
`[D6-T15]` (main.cu:35) arithmetic intensity ≈ 2 ops / 8 bytes = 0.25 FLOP/byte。
`[D6-T16]` (main.cu:36) TODO [必做-1]。
`[D6-T18]` (main.cu:48) Kernel 2：compute-bound matmul tile（简化版）。
`[D6-T19]` (main.cu:49) 特点：共享内存 tiling，高 FMA 密度。
`[D6-T20]` (main.cu:50) arithmetic intensity ≈ TILE_DIM / 2 FLOP/byte（高）。
`[D6-T21]` (main.cu:51) TODO [必做-1]。
`[D6-T23]` (main.cu:81) TODO [必做-1] 计算 tile 内积。
`[D6-T25]` (main.cu:93) Kernel 3：latency-bound atomic histogram（不规则访问）。
`[D6-T26]` (main.cu:94) 特点：大量 global atomicAdd，随机写地址 → L2/DRAM 高延迟。
`[D6-T27]` (main.cu:95) arithmetic intensity ≈ 1 FLOP / 8 bytes（低），但不是 DRAM 带宽受限——是延迟受限。
`[D6-T28]` (main.cu:96) TODO [必做-1]。
`[D6-T48]` (main.cu:191) TODO [必做-3] Nsight Compute 确认 DRAM Throughput 接近峰值。
`[D6-T53]` (main.cu:213) TODO [必做-3] Nsight Compute 确认 SM Throughput 接近峰值。
`[D6-T58]` (main.cu:231) TODO [必做-3] Nsight Compute 确认 pipeline stall 率高。
`[D6-T65]` (main.cu:246) TODO [必做-4] 为每个 kernel 提出一个优化方向（填写注释）。
`[D6-T66]` (main.cu:247) TODO [必做-5] 在 roofline 图上标注三个 kernel 的位置。
`[D6-T67]` (main.cu:248) TODO [必做-6] 对 atomic_hist 实施一个优化（例如 shared memory 局部直方图），再次测量。

1. 给定三个典型 kernel：(a) reduction（memory-bound），(b) matrix tile multiply（compute-bound），(c) indirect memory access（latency-bound），分别用 Nsight Compute `--set full` 运行。
2. 对每个 kernel 记录：achieved throughput、peak throughput、L2 bandwidth、achieved occupancy。
3. 用 Nsight Compute 的"Speed of Light"部分判定每个 kernel 的瓶颈。记录 SM Throughput、L1 Throughput、L2 Throughput、DRAM Throughput 四个指标中哪个最接近 peak。
4. 根据瓶颈分类，为每个 kernel 提出一个可行的优化方向（例如 memory-bound 可增加 arithmetic intensity，compute-bound 可增加 occupancy）。
5. 绘制或记录三个 kernel 在 roofline 图上的位置（横轴：arithmetic intensity，纵轴：throughput）。观察它们分别落在"计算墙"还是"内存墙"一侧。
6. 对其中一个 kernel 实施一个优化，再次运行 Nsight Compute，对比优化前后的指标。

## 进阶任务

`[D6-T68]` (main.cu:249) TODO [进阶] 混合 kernel（既有 compute 也有 memory），分析复杂瓶颈。
`[D6-T69]` (main.cu:250) TODO [进阶] 对比 sm_80 vs sm_90a 上同一 kernel 的瓶颈分类差异。

- 实现一个混合 kernel（既有 compute 也有 memory），分析其复杂的瓶颈特性
- 对比不同硬件（sm_80 vs sm_90a）上同一 kernel 的瓶颈分类差异

## 验收点

- 三个 kernel 的 roofline 分析结果清晰，分别落在不同象限
- 瓶颈分类与 Nsight Compute "Speed of Light"指标一致
- 优化建议具体可行（例如"减少 L2 访问通过 shared memory tiling"）
- 优化后的数据显示相应指标的改善

## 观察点

- memory-bound kernel 的吞吐被 L2/DRAM 带宽限制，occupancy 高也帮不了
- compute-bound kernel 的吞吐被 SM 计算能力限制，需要增加 occupancy 或减少 warp divergence
- latency-bound kernel（如不规则内存访问）的瓶颈难以直观判断，需要关注 L1/L2 miss 率和 pipeline stall
- roofline 图能快速定位优化方向

## 常见坑

- 混淆"achieved throughput"和"peak throughput"（achieved 是实际，peak 是理论最大）
- 在 memory-bound kernel 上追求 occupancy 优化（没有帮助）
- 没有考虑到"实际可达的吞吐"受多个指标共同限制（例如既受 L2 带宽也受 DRAM 延迟）
- 在 roofline 图上读错坐标，误判瓶颈
- 对非 compute-heavy 的 kernel 盲目应用 CUTLASS 等高度优化库（可能没有对应的专化实现）

## 提示

- arithmetic intensity = (float ops) / (bytes accessed)，越高越偏向 compute-bound
- 常见阈值（sm_90a）：intensity > ~10 时 compute-bound，< 1 时 memory-bound，1–10 之间看其他因素
- Nsight Compute 的"Memory Throughput / Peak Memory Throughput"接近 100% = memory-bound
- Nsight Compute 的"SM Throughput / Peak SM Throughput"接近 100% = compute-bound

## 复盘问题

- 如果一个 kernel 的 roofline 点正好在计算墙和内存墙的交界处，说明什么？
- latency-bound kernel 应该如何优化？
- 为什么同一个 kernel 在 sm_80 和 sm_90a 上的瓶颈可能不同？
- 能否构造一个"完全 latency-bound"的 kernel？

## 对应官方参考

- Nsight Compute Documentation: "Kernel Profiling Guide"
- "Roofline Model" paper (Williams et al.)
- CUDA C++ Best Practices: "Performance Metrics" section
- NVIDIA blog posts on roofline analysis

## 输出对照（printf / std::puts 原文）

- `[D6-T24]` (main.cu:88) 原文：`stub：修复后改为 acc` -> 现：`stub: replace with acc after fix`
- `[D6-T29]` (main.cu:103) 原文：`随机写 → high L2 miss rate` -> 现：`random write -> high L2 miss rate`
- `[D6-T37]` (main.cu:165) 原文：`--- Roofline 分析（arithmetic intensity 估算）---` -> 现：`--- Roofline analysis (estimated arithmetic intensity) ---`
- `[D6-T45]` (main.cu:179) 原文：`--- Kernel 1: scan（memory-bound）---` -> 现：`--- Kernel 1: scan (memory-bound) ---`
- `[D6-T46]` (main.cu:183) 原文：`启动: grid=%d block=%d` -> 现：`launch: grid=%d block=%d`
- `[D6-T47]` (main.cu:190) 原文：`时间=%.3f ms  带宽=%.1f GB/s` -> 现：`time=%.3f ms  bandwidth=%.1f GB/s`
- `[D6-T50]` (main.cu:197) 原文：`--- Kernel 2: matmul_tile（compute-bound）---` -> 现：`--- Kernel 2: matmul_tile (compute-bound) ---`
- `[D6-T51]` (main.cu:202) 原文：`启动: grid=(%d,%d) block=(%d,%d)` -> 现：`launch: grid=(%d,%d) block=(%d,%d)`
- `[D6-T52]` (main.cu:212) 原文：`时间=%.3f ms  GFLOP/s=%.1f (stub: TODO 完成后填入正确数)` -> 现：`time=%.3f ms  GFLOP/s=%.1f (stub: correct value after TODO)`
- `[D6-T55]` (main.cu:219) 原文：`--- Kernel 3: atomic_hist（latency-bound）---` -> 现：`--- Kernel 3: atomic_hist (latency-bound) ---`
- `[D6-T56]` (main.cu:225) 原文：`启动: grid=%d block=%d` -> 现：`launch: grid=%d block=%d`
- `[D6-T57]` (main.cu:230) 原文：`(latency-bound: 高 L2 miss 与 atomic 竞争)` -> 现：`(latency-bound: high L2 miss and atomic contention)`
- `[D6-T60]` (main.cu:240) 原文：`--- 瓶颈分类（Speed-of-Light 解读指引）---` -> 现：`--- Bottleneck classification (Speed-of-Light interpretation) ---`
- `[D6-T61]` (main.cu:241) 原文：`kernel          | 预期瓶颈           | Nsight 指标` -> 现：`kernel          | expected bottleneck | Nsight metric`
- `[D6-T62]` (main.cu:242) 原文：`scan            | memory-bound       | DRAM Throughput ≈ peak` -> 现：`scan            | memory-bound        | DRAM Throughput ~ peak`
- `[D6-T63]` (main.cu:243) 原文：`matmul_tile     | compute-bound      | SM Throughput ≈ peak` -> 现：`matmul_tile     | compute-bound       | SM Throughput ~ peak`
- `[D6-T64]` (main.cu:244) 原文：`atomic_hist     | latency-bound      | L2 miss 高, pipeline stall 高` -> 现：`atomic_hist     | latency-bound       | high L2 miss, high pipeline stall`
- `[D6-T70]` (main.cu:262) 原文：`[D6] 完成。用 ncu --set full -o d6_profile 采集后在 Nsight Compute GUI 分析。` -> 现：`[D6] done. Use ncu --set full -o d6_profile, then analyze in Nsight Compute GUI.`
