# 练习 D1：warp_shuffle_basics

## 目标

`[D1-T01]` (main.cu:2) 练习 D1：Warp Shuffle 五大原语。
`[D1-T02]` (main.cu:3) broadcast / butterfly / shift / reduce / scan。

学会五大 warp shuffle 函数，理解为什么 warp-local 操作比 shared memory 快，以及如何用 shuffle 实现 warp-32 级的 reduce 和 scan。

## 前置理解

- warp 是 32 个线程的集合，硬件调度的最小单位
- lane ID 是 warp 内的线程编号（0–31）
- shuffle 操作不需要 shared memory，直接从寄存器读数据

## 必做任务

`[D1-T03]` (main.cu:5) 学习目标。
`[D1-T04]` (main.cu:6) `__shfl_sync`：broadcast（lane 0 广播给全 warp）。
`[D1-T05]` (main.cu:7) `__shfl_xor_sync`：butterfly exchange。
`[D1-T06]` (main.cu:8) `__shfl_up_sync`：向高 lane 移位（用于 Kogge-Stone scan）。
`[D1-T07]` (main.cu:9) `__shfl_down_sync`：向低 lane 移位（用于 warp reduce）。
`[D1-T08]` (main.cu:10) 实现 warp-32 reduce 和前缀和（inclusive scan）。
`[D1-T11]` (main.cu:30) Kernel 1：warp broadcast — lane 0 广播给所有 lane。
`[D1-T12]` (main.cu:31) TODO [必做-1]。
`[D1-T13]` (main.cu:38) TODO [必做-1] 用 `__shfl_sync` 把 lane 0 的值广播给全部 lane。
`[D1-T15]` (main.cu:46) Kernel 2：butterfly exchange — lane i 与 lane (i^1) 交换。
`[D1-T16]` (main.cu:47) TODO [必做-2]。
`[D1-T17]` (main.cu:54) TODO [必做-2] `__shfl_xor_sync(mask, val, 1)` 实现相邻 lane 交换。
`[D1-T19]` (main.cu:62) Kernel 3：shift — 每个 lane 读上游 offset=1 的 lane 的值。
`[D1-T20]` (main.cu:63) TODO [必做-3]。
`[D1-T21]` (main.cu:70) TODO [必做-3] `__shfl_down_sync`：每个 lane 读 lane+1 的值。
`[D1-T23]` (main.cu:78) 设备函数：warp reduce（求和）。
`[D1-T24]` (main.cu:79) TODO [必做-4]。
`[D1-T25]` (main.cu:83) TODO [必做-4] log2(32) = 5 轮，每轮 offset 减半。
`[D1-T27]` (main.cu:91) Kernel 4：warp reduce kernel。
`[D1-T28]` (main.cu:92) TODO [必做-4]。
`[D1-T30]` (main.cu:105) 设备函数：Kogge-Stone inclusive scan。
`[D1-T31]` (main.cu:106) TODO [必做-5]。
`[D1-T34]` (main.cu:121) Kernel 5：warp inclusive scan kernel。
`[D1-T35]` (main.cu:122) TODO [必做-5]。
`[D1-T38]` (main.cu:138) Kernel 6：smem 版 reduce（对比用）。
`[D1-T39]` (main.cu:139) TODO [必做-6]。

1. 用 `__shfl_sync` 实现一个 warp 内的 broadcast：lane 0 的值广播给其他 31 个 lane，验证所有 lane 都读到相同值。
2. 用 `__shfl_xor_sync` 实现一个"蝶形"（butterfly exchange）：lane 0 和 1 交换，lane 2 和 3 交换，以此类推。验证交换结果正确。
3. 用 `__shfl_down_sync` 实现"移位"：每个 lane 读上游 offset 步的 lane 的值，逐步构建一个 warp 内的 reduce。
4. 用上述 shuffle 实现一个 warp-32 的求和 reduce：每轮迭代，offset 指数增大（1, 2, 4, 8, 16），逐步汇合结果到 lane 0。
5. 实现一个 warp-level scan（前缀和）：Kogge-Stone 算法，log(32) 轮，每轮用 `__shfl_up_sync` 读前驱。
6. 对比 shuffle 版和 shared memory 版本的性能（用 Nsight Compute 或简单计时）。shuffle 应该更快。

## 进阶任务

`[D1-T55]` (main.cu:280) TODO [进阶] Brent-Kung scan 实现。
`[D1-T56]` (main.cu:281) TODO [进阶] warp-16 partial reduce（mask=0x0000ffff）。

- 实现 Brent-Kung scan（不同的流水线策略），对比 Kogge-Stone
- 尝试 warp-16 或 warp-8 的 shuffle（用 mask 参数限制参与的 lane），观察部分 warp 参与时的行为

## 验收点

- warp reduce 和 scan 的结果与 CPU 参考实现一致
- 能清晰指出每个 `__shfl_*` 调用的 offset 含义
- Nsight Compute 显示 shuffle 版本的延迟或吞吐优于 smem 版本
- 代码没有寄存器溢出（register spill）

## 观察点

- warp shuffle 利用了"同一 warp 自动前向进度"的特性，无需显式屏障
- shuffle 的 mask 参数用于指定参与的 lane 集合（通常是 `0xffffffff`，表示全 warp）
- 同一个 warp 内的 shuffle 没有内存访问，全在寄存器和片上通路上，所以很快
- warp reduce / scan 是后续 block-level reduce / scan 的基础块

## 常见坑

- shuffle mask 错误（例如 mask 不包含某个 lane，导致该 lane 读到 undefined 值）
- 混淆 `__shfl_up` 和 `__shfl_down` 的方向
- offset 设置不对，导致某些数据被读多次或没被读
- 在非 warp 边界的地方结束 shuffle 操作，导致某些线程看到的数据不一致
- 假设 warp shuffle 可以跨 block，实际只在同一 warp 内有效
- 没有在 Kogge-Stone 或 Brent-Kung 的每一步加上所需的同步（warp 内无需显式 sync，但要理解数据依赖）

## 提示

- warp reduce 标准模式：`for(int offset = 16; offset > 0; offset >>= 1) { value += __shfl_down_sync(0xffffffff, value, offset); }`
- Kogge-Stone scan：`for(int offset = 1; offset < 32; offset <<= 1) { int prev = __shfl_up_sync(0xffffffff, value, offset); if(laneId >= offset) value += prev; }`
- mask 常用值：`0xffffffff`（全 warp）、`0x0000ffff`（低 16 lane）、根据需要构造
- 验证：shuffle 结果应与对应的 smem 版本一致

## 复盘问题

- warp reduce 需要多少轮 shuffle？为什么是这个数？
- Kogge-Stone 和 Brent-Kung 对于 scan 分别有什么优缺点？
- 为什么 warp shuffle 比 shared memory 快？（提示：内存层级）
- 如果只想操作 warp 的低 16 lane，mask 应该怎样设置？

## 对应官方参考

- CUDA C++ Programming Guide Section C.22: "Warp Shuffle Functions"
- CUDA C++ Best Practices: "Warp Shuffle" section
- NVIDIA/cccl CUB library: `warp_reduce` example
- CUDA Samples: `reduction` sample code

## 输出对照（printf / std::puts 原文）

- `[D1-T44]` (main.cu:201) 原文：`(期望两者均=%d, stub=0)` -> 现：`(expected both=%d, stub=0)`
- `[D1-T46]` (main.cu:215) 原文：`(期望 %d/%d, stub=0)` -> 现：`(expected %d/%d, stub=0)`
- `[D1-T48]` (main.cu:228) 原文：`(期望 %d, stub=0)` -> 现：`(expected %d, stub=0)`
- `[D1-T50]` (main.cu:251) 原文：`shuffle=%d (期望 %d, stub=0)` -> 现：`shuffle=%d (expected %d, stub=0)`
- `[D1-T51]` (main.cu:254) 原文：`smem   =%d (期望 %d)` -> 现：`smem   =%d (expected %d)`
- `[D1-T53]` (main.cu:271) 原文：`inclusive[31]=%d (期望 %d, stub=0)` -> 现：`inclusive[31]=%d (expected %d, stub=0)`
- `[D1-T54]` (main.cu:274) 原文：`exclusive[1] =%d (期望 %d, stub=0)` -> 现：`exclusive[1] =%d (expected %d, stub=0)`
- `[D1-T57]` (main.cu:289) 原文：`[D1] 完成。用 Nsight Compute 对比 shuffle 与 smem 版本的延迟。` -> 现：`[D1] done. Use Nsight Compute to compare shuffle vs smem latency.`
