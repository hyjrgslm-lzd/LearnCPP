# 练习 C3：cooperative_groups

## 目标

`[C3-T01]` (main.cu:1) 文件标识：`C3_cooperative_groups/main.cu`。
`[C3-T02]` (main.cu:2) 练习 C3：Cooperative Groups — `this_thread_block` + `tiled_partition`。
`[C3-T03]` (main.cu:4) 学习目标：用 `cooperative_groups::this_thread_block()` 替代 `__syncthreads()`；用 `tiled_partition<32>` / `tiled_partition<16>` 划分子组；对比 raw sync 版本与 CG 版本的可读性和性能。
`[C3-T04]` (main.cu:9) 编译与运行命令。

学会用 cooperative groups 提供的抽象（`this_thread_block`、`tiled_partition`）替代 raw `__syncthreads`，体验更清晰、更灵活的同步语义。

## 前置理解

- 完成练习 C2，掌握 `__syncthreads` 的基础用法
- 理解 warp 的概念和同一 warp 内线程间的自动"前向进度保证"
- 理解 tiling 的概念：把 block 分成多个 tile（例如 32-thread 子集）

## 必做任务

`[C3-T05]` (main.cu:25) 常量定义。
`[C3-T06]` (main.cu:30) Kernel 1：raw `__syncthreads` 版 reduction（参考基线）。
`[C3-T07]` (main.cu:31) TODO [必做-4] 对照组。
`[C3-T08]` (main.cu:52) Kernel 2：`cooperative_groups::this_thread_block()` 替代 `__syncthreads`。
`[C3-T09]` (main.cu:53) TODO [必做-1]。
`[C3-T10]` (main.cu:60) TODO [必做-1] 获取 block 级别的 cooperative group。
`[C3-T11]` (main.cu:66) TODO [必做-1] 用 `block.sync()` 替代 `__syncthreads()`。
`[C3-T12]` (main.cu:73) TODO [必做-1] `block.sync()` 而非 `__syncthreads()`。
`[C3-T13]` (main.cu:77) stub：修复后改为 `smem[0]`。
`[C3-T14]` (main.cu:81) 设备函数：warp tile reduce（供两层 reduce 使用）。
`[C3-T15]` (main.cu:82) TODO [必做-3]。
`[C3-T16]` (main.cu:86) TODO [必做-3] 用 `tile.shfl_down` 做 warp reduce。
`[C3-T17]` (main.cu:90) stub：修复后 lane 0 持有正确的 warp sum。
`[C3-T18]` (main.cu:93) Kernel 3：两层 reduce — `tiled_partition<32>` + `this_thread_block`。
`[C3-T19]` (main.cu:94) TODO [必做-3]。
`[C3-T20]` (main.cu:98) 每个 warp 写一个结果。
`[C3-T21]` (main.cu:102) TODO [必做-2] 创建 `tiled_partition<32>`。
`[C3-T22]` (main.cu:111) 第一层：warp 内 reduce。
`[C3-T23]` (main.cu:114) lane 0 写入 shared memory。
`[C3-T24]` (main.cu:120) 第二层：用第 0 个 warp 汇合所有 warp 的结果。
`[C3-T25]` (main.cu:125) stub：修复后改为 val。
`[C3-T26]` (main.cu:128) Kernel 4：`tiled_partition<16>` 示范。
`[C3-T27]` (main.cu:129) TODO [必做-5]。
`[C3-T28]` (main.cu:133) TODO [必做-5] 创建 `tiled_partition<16>`。
`[C3-T29]` (main.cu:140) TODO [必做-5] 在 `tile16` 内做 reduce（仅演示，不汇合到 block）。
`[C3-T30]` (main.cu:145) 每个 `tile<16>` 的 rank 0 输出。
`[C3-T31]` (main.cu:149) stub。
`[C3-T32]` (main.cu:152) CPU 参考。

1. 用 `#include <cooperative_groups.h>` 并写一个简单的 barrier：`auto g = cooperative_groups::this_thread_block(); g.sync();` 替代 `__syncthreads()`。编译并验证行为相同。
2. 创建一个 `tiled_partition<32>` 的子组，在其内部调用 `.sync()`，观察它只同步 32 个线程而不是整个 block。
3. 写一个两层 reduction：第一层用 `tiled_partition<32>` 在 warp 内做，第二层用 `this_thread_block()` 在 block 间汇合。清晰标记两层的同步边界。
4. 对比 raw `__syncthreads` 版本和 cooperative groups 版本的可读性；在注释中标记出"这个 tile 的职责"和"这个 block 的职责"。
5. 尝试 `tiled_partition<16>` 和 `tiled_partition<64>`（如果可用），观察不同 tile 大小对同步覆盖范围的影响。
6. 在 Nsight Compute 中对比两个版本的性能差异（应该相近，或 cooperative groups 略优于 raw sync）。

## 进阶任务

`[C3-T43]` (main.cu:230) TODO [必做-6] Nsight Compute 对比两版本性能。
`[C3-T44]` (main.cu:231) TODO [进阶] 用 cooperative_groups 实现 inclusive/exclusive scan。
`[C3-T45]` (main.cu:232) TODO [进阶] 使用 `cooperative_groups::partition` 动态分组。

- 实现一个 scan（prefix sum）用 cooperative groups，逐层展示 inclusive/exclusive scan 的同步需求
- 使用 `cooperative_groups::partition` 动态分组，体验更灵活的同步方式

## 验收点

`[C3-T33]` (main.cu:160) 主程序入口。
`[C3-T35]` (main.cu:182) 测试 raw sync baseline。
`[C3-T37]` (main.cu:198) 测试 CG block。
`[C3-T39]` (main.cu:214) 测试两层 reduce（warp tile）。
`[C3-T41]` (main.cu:226) 测试 `tile<16>`。

- 代码编译无误，运行结果与 C2 的 raw `__syncthreads` 版本一致
- 能指出 `tiled_partition<32>` 和 `this_thread_block()` 的作用范围差异
- Nsight Compute profiling 结果表明两个版本性能相近
- 代码易读性提升，同步意图更清晰

## 观察点

- cooperative groups 提供了"命名的同步范围"，比 raw `__syncthreads` 的"全 block 同步"更灵活
- `tiled_partition<32>` 利用 warp 的"前向进度保证"（内部无需显式 sync），但跨 tile 仍需屏障
- 用 cooperative groups 写的代码更易于维护和扩展，因为同步范围明确

## 常见坑

- 忘记 `#include <cooperative_groups.h>` 或 `using namespace cooperative_groups;`
- 混淆 `this_thread_block()` 和 `tiled_partition<W>(g)` 的范围
- 在不完整的 tile 上调用 sync（例如 blockDim.x = 40，用 `tiled_partition<32>` 会留下 8 个线程单独一个 tile）
- 假设 `tiled_partition<32>` 内的线程不需要同步（实际上如果有 smem 读写仍需考虑）
- 使用 cooperative groups 后完全忘记了底层 warp-level 的执行模型

## 提示

- cooperative groups 的核心：把线程分组，每个组有独立的 `.sync()` 方法
- `tiled_partition<32>` 对应一个 warp（auto-sync），但仍需显式 `.sync()` 确保读写可见性
- 如果 blockDim.x 不是 tile size 的倍数，最后一个 tile 会不完整；这时 `.sync()` 只等待该 tile 内的线程
- cooperative groups 在 Hopper cluster 上可以扩展到跨 block（`this_grid()` 等）

## 复盘问题

- 为什么 `tiled_partition<32>` 内的 `.sync()` 比 `__syncthreads()` 轻？
- 在 blockDim.x = 64 的情况下用 `tiled_partition<32>`，有几个 tile？每个 tile 何时同步？
- cooperative groups 与 raw sync 在硬件层面的区别是什么（从 PTX 角度）？
- 什么时候应该用 cooperative groups 而不是 raw `__syncthreads`？

## 对应官方参考

- CUDA C++ Programming Guide Section 3.2.8: "Cooperative Groups"
- NVIDIA/cccl repository: `cooperative_groups` header and examples
- Cooperative Groups documentation: https://docs.nvidia.com/cuda/cuda-c-programming-guide/#cooperative-groups
- CUDA C++ Best Practices: Synchronization and atomics section

## 输出对照（printf / std::puts 原文）

- `[C3-T34]` (main.cu:170) 原文：`CPU 参考结果 = %d` → 现：`CPU reference result = %d`
- `[C3-T36]` (main.cu:191) 原文：`[raw_sync]   结果=%d (期望 %d)  %.3f ms` → 现：`[raw_sync]   result=%d (expected %d)  %.3f ms`
- `[C3-T38]` (main.cu:207) 原文：`[cg_block]   结果=%d (期望 %d, stub=0)  %.3f ms` → 现：`[cg_block]   result=%d (expected %d, stub=0)  %.3f ms`
- `[C3-T40]` (main.cu:223) 原文：`[cg_tile32]  结果=%d (期望 %d, stub=0)  %.3f ms` → 现：`[cg_tile32]  result=%d (expected %d, stub=0)  %.3f ms`
- `[C3-T42]` (main.cu:235) 原文：`[tile16]     （演示，不汇合，stub=0）` → 现：`[tile16]     (demo only, no merge, stub=0)`
- `[C3-T46]` (main.cu:245) 原文：`[C3] 完成。TODO 完成后两个版本结果应与 CPU 参考一致。` → 现：`[C3] done. After completing TODOs, both versions should match the CPU reference.`
