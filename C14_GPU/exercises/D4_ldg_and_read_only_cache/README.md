# 练习 D4：ldg_and_read_only_cache

## 目标

`[D4-T01]` (main.cu:2) 练习 D4：`__ldg` 与 read-only cache。
`[D4-T02]` (main.cu:3) 访存优化与 L1TEX 命中率。

学会用 `__ldg` 指令和 read-only cache 优化只读数据的访问性能，理解什么时候 `__ldg` 有效、什么时候无效，以及如何用 Nsight Compute 验证。

## 前置理解

- 理解 GPU 内存层级（global → L2 → L1）
- 理解 `const __restrict__` 指针修饰的含义
- 理解 coalescing 和 cache 行为

## 必做任务

`[D4-T03]` (main.cu:5) 学习目标。
`[D4-T04]` (main.cu:6) `__ldg` 生成 `ld.global.nc` 指令，走 read-only cache（绕过 L1 data cache）。
`[D4-T05]` (main.cu:7) 对比普通指针读、`__ldg` 读、`const __restrict__` 指针读的 PTX 差异。
`[D4-T06]` (main.cu:8) memory-bound 场景下 `__ldg` 能提升 L2 / DRAM 命中率。
`[D4-T07]` (main.cu:9) compute-bound 场景下 `__ldg` 效果有限。
`[D4-T08]` (main.cu:10) Nsight Compute：查看 L1TEX 命中率与 Memory Throughput。
`[D4-T10]` (main.cu:33) Kernel 1：普通指针读（无优化）。
`[D4-T11]` (main.cu:34) TODO [必做-1] 对照组 A。
`[D4-T12]` (main.cu:40) TODO [必做-1] 普通读：生成 `ld.global.ca`（with L1 cache）。
`[D4-T13]` (main.cu:41) 普通读。
`[D4-T14]` (main.cu:46) Kernel 2：`__ldg` 读（走 read-only / texture cache）。
`[D4-T15]` (main.cu:47) TODO [必做-1] 对照组 B。
`[D4-T16]` (main.cu:53) TODO [必做-1] 用 `__ldg` 读，生成 `ld.global.nc`。
`[D4-T18]` (main.cu:62) Kernel 3：`const __restrict__` 指针（编译器可能自动生成 `__ldg`）。
`[D4-T19]` (main.cu:63) TODO [必做-5]。
`[D4-T20]` (main.cu:68) TODO [必做-5] 观察 `const __restrict__` 是否自动产生 `ld.global.nc`。
`[D4-T21]` (main.cu:74) Kernel 4：memory-bound 场景（每线程读 1 float，做极少计算）。
`[D4-T22]` (main.cu:75) TODO [必做-4] — 这里 `__ldg` 效果明显。
`[D4-T23]` (main.cu:82) 极少计算，瓶颈在内存。
`[D4-T25]` (main.cu:99) Kernel 5：compute-bound 场景（大量 float 计算，内存访问不是瓶颈）。
`[D4-T26]` (main.cu:100) TODO [必做-4] — 这里 `__ldg` 效果有限。
`[D4-T27]` (main.cu:107) 大量计算（约 64 次 float 运算）。
`[D4-T41]` (main.cu:254) TODO [必做-3] PTX 查看：`nvcc -arch sm_90a -ptx main.cu`。
`[D4-T42]` (main.cu:255) 搜索 `ld.global.ca` vs `ld.global.nc`。
`[D4-T43]` (main.cu:256) TODO [必做-6] Nsight Compute：查看 L1TEX hit rate 与 Memory Throughput。

1. 写一个简单的 kernel：从 global memory 读只读数据（例如权重矩阵），进行计算。第一个版本用普通指针读，第二个版本用 `__ldg` 读。
2. 对两个版本进行性能对比测量（用 Nsight Compute 或计时）。
3. 在 PTX 代码中查看两个版本的指令差异：普通读是否是 `ld.global.ca`（with cache），`__ldg` 是否是 `ld.global.nc`（no cache）。
4. 尝试一个"memory-bound"场景（例如每个 thread 读 1 个 float，做最小计算）和"compute-bound"场景（例如每个 thread 读 1 个 float，做大量计算），观察 `__ldg` 效果的差异。
5. 用 `const __restrict__` 修饰指针，观察编译器是否能自动生成 `__ldg`（某些情况下可以）。
6. 在 Nsight Compute 中查看 L1 和 L2 cache 的命中率（hit ratio），对比优化前后。

## 进阶任务

`[D4-T44]` (main.cu:257) TODO [进阶] texture cache 版本对比。
`[D4-T45]` (main.cu:258) TODO [进阶] compute-bound kernel 中使用 `__ldg` 观察无明显变化。

- 实现一个 texture 读取版本（使用 texture cache），对比 `__ldg` 和 texture 的性能
- 尝试在 compute-bound kernel 中使用 `__ldg`，观察对性能的影响

## 验收点

- PTX 代码清晰显示 `__ldg` 生成了 `ld.global.nc` 指令
- 在 memory-bound 场景中，`__ldg` 相比普通读有明显加速
- Nsight Compute L1/L2 cache 指标显示差异
- compute-bound 场景中 `__ldg` 效果不明显（符合预期）

## 观察点

- `__ldg` 绕过 L1 cache，直接访问 L2 和 global memory，在某些模式下更优
- `__ldg` 最优场景是"稀疏、非临时"的只读访问（例如权重矩阵的不规则访问）
- `const __restrict__` 给编译器优化的机会，但不保证生成 `__ldg`
- read-only cache 是一种特殊的片上缓存，行为与通用 L1 不同

## 常见坑

- 在 compute-bound kernel 中期望 `__ldg` 有巨大性能提升（实际效果有限）
- 混淆 `__ldg` 和 texture：`__ldg` 用硬件的 read-only cache，texture 有专门的 texture cache（二者独立）
- 忘记 `#include <cuda_runtime_helpers.h>` 或类似头文件，导致 `__ldg` 不可用
- `__ldg` 仅对指针参数有效，如果数据已在寄存器中则无效
- 没有实际测量性能，只是猜测 `__ldg` 是否有效

## 提示

- `__ldg` 原型：`template<class T> T __ldg(const T* ptr);` 任何标量类型都可
- 查看 PTX：`nvcc -arch sm_90a -ptx kernel.cu` 生成 .ptx 文件，搜索 `ld.global`
- memory-bound 验证：Nsight Compute 中看 "Memory Throughput / Peak Throughput" 的比值
- `const __restrict__` 写法：`const float * __restrict__ weights` 表示只读且无别名

## 复盘问题

- `__ldg` 为什么在 memory-bound 场景下有效，在 compute-bound 场景下无效？
- read-only cache 和 L1 cache 的主要区别是什么？
- 如果一个指针既读又写，`__ldg` 还能用吗？
- `const __restrict__` 修饰的指针一定会生成 `__ldg` 吗？

## 对应官方参考

- CUDA C++ Programming Guide Section 3.2.2: "Device Memory"
- CUDA Runtime API: `__ldg` function
- CUDA C++ Best Practices: "Read-Only Data Cache" section
- PTX ISA: `ld.global.*` instruction variants

## 输出对照（printf / std::puts 原文）

- `[D4-T17]` (main.cu:56) 原文：`stub：取消注释上一行后删除此行` -> 现：`stub: remove this line after uncommenting above`
- `[D4-T24]` (main.cu:90) 原文（注释）：`TODO [必做-4]` -> 现：`TODO [REQUIRED-4]`
- `[D4-T28]` (main.cu:117) 原文（注释）：`TODO [必做-4]` -> 现：`TODO [REQUIRED-4]`
- `[D4-T30]` (main.cu:133) 原文：`数组大小: N=%d (%.1f MB)` -> 现：`Array size: N=%d (%.1f MB)`
- `[D4-T34]` (main.cu:176) 原文：`(stub=plain，TODO 后差异可见)` -> 现：`(stub=plain, difference visible after TODO)`
- `[D4-T36]` (main.cu:196) 原文：`--- memory-bound 场景（极少计算）---` -> 现：`--- memory-bound scenario (minimal compute) ---`
- `[D4-T37]` (main.cu:219) 原文：`(TODO 完成后应略快于 plain)` -> 现：`(after TODO should be slightly faster than plain)`
- `[D4-T39]` (main.cu:225) 原文：`--- compute-bound 场景（大量 float 计算）---` -> 现：`--- compute-bound scenario (heavy float compute) ---`
- `[D4-T40]` (main.cu:248) 原文：`(期望与 plain 相近 — compute-bound)` -> 现：`(expected close to plain - compute-bound)`
- `[D4-T46]` (main.cu:264) 原文：`[D4] 完成。用 ncu --metrics l1tex__t_sector_hit_rate.pct 查看 L1TEX 命中率。` -> 现：`[D4] done. Use ncu --metrics l1tex__t_sector_hit_rate.pct to view L1TEX hit rate.`
