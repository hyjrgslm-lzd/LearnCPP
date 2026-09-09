# 练习 D5：occupancy_and_launch_bounds

## 目标

`[D5-T01]` (main.cu:2) 练习 D5：Occupancy 与 `__launch_bounds__`。
`[D5-T02]` (main.cu:3) 寄存器压力、spill 与 occupancy 权衡。

理解 occupancy 的定义、计算、与性能的关系，以及如何用 `__launch_bounds__` 属性和 `cudaOccupancyMaxPotentialBlockSize` API 调控 blockDim、寄存器压力和 occupancy 的权衡。

## 前置理解

- 理解 block/warp/thread 映射和 SM 资源（寄存器、smem、并发 warp 数）
- 理解寄存器溅出（spill）的概念

## 必做任务

`[D5-T03]` (main.cu:5) 学习目标。
`[D5-T04]` (main.cu:6) `cudaOccupancyMaxPotentialBlockSize` 查询推荐的 blockDim。
`[D5-T05]` (main.cu:7) `__launch_bounds__(maxThreads, minBlocksPerSM)` 三种参数对比。
`[D5-T06]` (main.cu:8) 寄存器 spill 时在 PTX 中看到 `st.local` / `ld.local`。
`[D5-T07]` (main.cu:9) Nsight Compute occupancy 数据与 `-Xptxas=-v` 统计。
`[D5-T09]` (main.cu:30) Kernel 1：无 `__launch_bounds__`（基线）。
`[D5-T10]` (main.cu:31) TODO [必做-1] — 用 `cudaOccupancyMaxPotentialBlockSize` 查询此 kernel 的推荐 blockDim。
`[D5-T11]` (main.cu:43) Kernel 2：`__launch_bounds__(256, 0)` — 声明最大 256 线程，不限 minBlocks。
`[D5-T12]` (main.cu:44) TODO [必做-2] — 对照组 A。
`[D5-T13]` (main.cu:57) Kernel 3：`__launch_bounds__(256, 2)` — 至少 2 block/SM 同时运行。
`[D5-T14]` (main.cu:58) TODO [必做-2] — 对照组 B（编译器可能减少寄存器以满足 minBlocks）。
`[D5-T15]` (main.cu:71) Kernel 4：`__launch_bounds__(256, 4)` — 更激进的 minBlocks。
`[D5-T16]` (main.cu:72) TODO [必做-2] — 对照组 C。
`[D5-T17]` (main.cu:85) Kernel 5：寄存器压力较大的 kernel（制造 spill）。
`[D5-T18]` (main.cu:86) TODO [必做-5] — 用 `nvcc -Xptxas -v` 查看是否有 local memory 使用。
`[D5-T19]` (main.cu:93) 故意使用大量寄存器变量（不优化掉）。
`[D5-T20]` (main.cu:113) 防止编译器优化掉（使用所有变量）。
`[D5-T21]` (main.cu:114) TODO [必做-5] 取消 `#pragma unroll` 注释，观察对寄存器的影响。
`[D5-T24]` (main.cu:161) TODO [必做-1] `cudaOccupancyMaxPotentialBlockSize` 查询推荐 blockDim。
`[D5-T28]` (main.cu:174) TODO [必做-3] 打印各 kernel 的 occupancy。
`[D5-T34]` (main.cu:228) TODO [必做-6] 寄存器统计：`nvcc -Xptxas -v`。
`[D5-T35]` (main.cu:229) 输出格式：`ptxas info: Used X registers, Y bytes lmem, ...`。
`[D5-T36]` (main.cu:230) lmem > 0 表示发生 spill。

1. 写一个 kernel 不添加任何 attribute，用 `cudaOccupancyMaxPotentialBlockSize` 查询推荐的 blockDim。输出查询结果。
2. 在 kernel 前添加 `__launch_bounds__(maxThreadsPerBlock, minBlocksPerMultiprocessor)` attribute，尝试三组值：(256, 0)、(256, 2)、(256, 4)。
3. 对于每一组，运行 kernel 并在 Nsight Compute 中记录 occupancy 和寄存器使用量。
4. 对比三个版本的性能（吞吐或延迟），找出最优的 attribute 设置。
5. 实现一个寄存器压力较大的 kernel（例如每个 thread 用 80+ 寄存器），观察 spill 如何发生、如何影响性能。
6. 用 `nvcc -Xptxas -v` 编译，查看寄存器使用统计。

## 进阶任务

`[D5-T37]` (main.cu:231) TODO [进阶] 递增寄存器用量，绘制 occupancy vs 寄存器数曲线。
`[D5-T38]` (main.cu:232) TODO [进阶] `#pragma unroll` 对寄存器的影响。

- 实现一个递归或循环的 kernel，逐步增加寄存器用量，绘制"occupancy vs 寄存器数"的曲线
- 尝试用 `#pragma unroll` 控制循环展开，观察对寄存器和 occupancy 的影响

## 验收点

- `cudaOccupancyMaxPotentialBlockSize` 的推荐值合理（根据寄存器和 smem 用量）
- `__launch_bounds__` 的不同参数组合显示 occupancy 的变化趋势
- Nsight Compute 数据清晰显示 occupancy 与性能的关系
- 寄存器 spill 时能在 PTX 中看到 `st.local` / `ld.local` 指令

## 观察点

- occupancy 高不一定性能好（取决于瓶颈）
- `__launch_bounds__(maxThreads, minBlocks)` 中 minBlocks 提示编译器至少要保证多少 block 同时运行，以此优化寄存器分配
- 寄存器 spill 到 local memory（实际上是 global memory）会导致巨大延迟
- 某些情况下降低 occupancy 反而能减少寄存器压力，提升性能

## 常见坑

- 盲目追求 occupancy = 100%，导致寄存器 spill
- `__launch_bounds__` 的 minBlocks 设置过高，导致编译器无法满足而忽略 attribute
- 混淆"理论 occupancy"（资源限制）和"实际 occupancy"（运行时实测）
- 在 blockDim 远小于 SM 资源能承载的情况下追求 occupancy（此时 occupancy 自动很高）
- 没有用 `__restrict__` 等指令帮助编译器优化，导致寄存器用量不必要的高

## 提示

- occupancy 定义：`(活跃 warp 数) / (每 SM 最大 warp 数)`，sm_90a 时每 SM 最多 128 warp
- `cudaOccupancyMaxPotentialBlockSize` 会考虑动态 smem，如果用了动态 smem 需在调用时指定大小
- 寄存器使用统计：`nvcc -Xptxas -v kernel.cu 2>&1 | grep registers`
- local memory 用量查看：`nvcc -Xptxas -v kernel.cu 2>&1 | grep "local memory"`

## 复盘问题

- sm_90a 上，如果 occupancy 100% 但性能反而下降，可能的原因是什么？
- `__launch_bounds__(256, 2)` 意味着什么？编译器会如何调整寄存器分配？
- occupancy 从 50% 提升到 100% 一定能使吞吐翻倍吗？
- 如何判断当前 kernel 是被 occupancy 限制还是被其他因素（memory/compute）限制？

## 对应官方参考

- CUDA C++ Programming Guide Section 4.2: "Hardware Multithreading"
- CUDA Runtime API: `cudaOccupancyMaxPotentialBlockSize`
- CUDA C++ Best Practices: "Execution Configuration Optimizations" section
- Nsight Compute: "Occupancy Analysis" section

## 输出对照（printf / std::puts 原文）

- `[D5-T26]` (main.cu:169) 原文：`[必做-1] cudaOccupancyMaxPotentialBlockSize:` -> 现：`[REQUIRED-1] cudaOccupancyMaxPotentialBlockSize:`
- `[D5-T27]` (main.cu:170) 原文：`(stub=0，TODO 后填入)` -> 现：`(stub=0, fill in after TODO)`
- `[D5-T29]` (main.cu:177) 原文：`[必做-3] 各 kernel 的 occupancy（blockDim=%d）：` -> 现：`[REQUIRED-3] occupancy per kernel (blockDim=%d):`
- `[D5-T31]` (main.cu:191) 原文：`[必做-4] 性能对比（REPS=%d，grid=%d block=%d）：` -> 现：`[REQUIRED-4] performance (REPS=%d, grid=%d block=%d):`
- `[D5-T33]` (main.cu:222) 原文：`(观察 PTX 是否有 st.local/ld.local)` -> 现：`(check PTX for st.local/ld.local)`
- `[D5-T39]` (main.cu:239) 原文：`[D5] 完成。用 nvcc -Xptxas -v 查看寄存器统计，ncu 查看 Achieved Occupancy。` -> 现：`[D5] done. Use nvcc -Xptxas -v for register stats, ncu for Achieved Occupancy.`
