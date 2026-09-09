# 练习 D3：warp_reduce_scan

## 目标

`[D3-T01]` (main.cu:2) 练习 D3：Block-level Reduce & Scan。
`[D3-T02]` (main.cu:3) 两层结构（warp shuffle + smem）。

综合 D1 和 D2，用 warp shuffle 和 ballot 高效实现 block-level reduce 和 scan，理解两层结构：warp-local 用 shuffle，block-level 用 shared memory。

## 前置理解

- 完成 D1（warp shuffle reduce/scan）和 D2（vote/ballot）
- 理解 block 由多个 warp 组成，需要两层操作才能完成 block-level 的 reduce/scan

## 必做任务

`[D3-T03]` (main.cu:5) 学习目标。
`[D3-T04]` (main.cu:6) 两层 reduce：warp shuffle 内聚，smem 跨 warp 汇合。
`[D3-T05]` (main.cu:7) 两层 scan：warp Kogge-Stone + warp 间 offset。
`[D3-T06]` (main.cu:8) 对比全 smem / 全 shuffle+atomic / 两层混合三个版本。
`[D3-T07]` (main.cu:9) Nsight Compute 瓶颈分类（memory-bound vs compute-bound）。
`[D3-T09]` (main.cu:31) 设备函数：warp reduce（D1 中的核心原语）。
`[D3-T10]` (main.cu:42) 设备函数：warp inclusive scan (Kogge-Stone)。
`[D3-T11]` (main.cu:53) Kernel 1：全 shared memory reduce（基线）。
`[D3-T12]` (main.cu:54) TODO [必做-1 对照组]。
`[D3-T13]` (main.cu:71) Kernel 2：两层混合 reduce（warp shuffle + smem second level）。
`[D3-T14]` (main.cu:72) TODO [必做-1]。
`[D3-T15]` (main.cu:84) 第一层：warp reduce。
`[D3-T16]` (main.cu:87) lane 0 写 warp 结果到 smem。
`[D3-T17]` (main.cu:91) 第二层：第 0 个 warp 汇合所有 warp 的和。
`[D3-T19]` (main.cu:101) Kernel 3：全 shuffle + atomicAdd（跨 warp 用 atomic）。
`[D3-T20]` (main.cu:102) TODO [必做-3 对比]。
`[D3-T22]` (main.cu:118) Kernel 4：两层 block inclusive scan。
`[D3-T23]` (main.cu:119) TODO [必做-2]。
`[D3-T24]` (main.cu:124) 每个 warp 的 inclusive scan 末值。
`[D3-T25]` (main.cu:125) warp 间 prefix offset。
`[D3-T26]` (main.cu:133) 第一层：warp 内 Kogge-Stone inclusive scan。
`[D3-T27]` (main.cu:136) lane 31 持有本 warp 的 total。
`[D3-T28]` (main.cu:140) 第 0 个 warp 做 warp_totals 的 scan，得到 warp 间 offset。
`[D3-T29]` (main.cu:149) 加上 warp 间 offset。
`[D3-T41]` (main.cu:262) TODO [必做-5] 不同 blockDim（128/256/512）下的吞吐对比。
`[D3-T42]` (main.cu:263) TODO [必做-6] Nsight Compute 瓶颈分类。

1. 实现一个两层 reduce：第一层用 warp shuffle 把每个 warp 的结果归到 lane 0，第二层用 shared memory + `__syncthreads` 汇合所有 warp 的结果。
2. 实现一个两层 scan：第一层在每个 warp 内用 Kogge-Stone 得到 inclusive scan，第二层在 warp 间用 smem 做"offset scan"，最后再加上 warp 间的 offset。
3. 对比三个版本的性能：(a) 全 shared memory，(b) 全 warp shuffle + atomic，(c) 两层混合。用 Nsight Compute 记录。
4. 验证 scan 结果（inclusive 和 exclusive）的正确性。
5. 测量不同 blockDim（128、256、512）下的吞吐差异。
6. 在 Nsight Compute 中查看瓶颈分类（是 memory-bound 还是 compute-bound）。

## 进阶任务

`[D3-T43]` (main.cu:264) TODO [进阶] block-level histogram（多 bin ballot）。
`[D3-T44]` (main.cu:265) TODO [进阶] split-K reduce（多 block + global atomic）。

- 实现一个 block-level histogram（多个 reduce，每个对应一个 bin），用 ballot 快速判定 lane 的 bin 归属
- 尝试"split-K" reduce：多个 block 的结果用 global memory atomic 汇合

## 验收点

- 两层 reduce 和 scan 的结果与单层 smem 版本一致
- 性能对比显示两层混合方案在不同条件下的权衡
- Nsight Compute 瓶颈分类清晰可见
- 代码无寄存器溢出

## 观察点

- 两层结构的优势：warp-local shuffle 避免了 smem 访问的延迟与同步开销
- block-level reduce/scan 的最后汇合仍需 shared memory 和 `__syncthreads`
- 瓶颈往往从 smem 访问延迟（memory-bound）转向计算吞吐（compute-bound）

## 常见坑

- warp reduce 后没有 `__syncthreads` 就读 smem，导致 race
- 混淆 inclusive scan 和 exclusive scan 的含义（inclusive：包括自己；exclusive：不包括自己）
- block 内 warp 个数计算错误（blockDim.x / 32，向上取整）
- 忘记在最后一个 warp 的结果写入 smem 时做 sync

## 提示

- 两层 reduce 标准模式：`value = warp_reduce(value); if(laneId == 0) smem[warpId] = value; __syncthreads(); value = warp_reduce(smem[laneId] if laneId < warpCount else 0);`
- warp 内 scan 用 Kogge-Stone，warp 间 scan 需要额外的 offset（前面 warp 的 scan 输出值）
- 瓶颈判定：看 Nsight Compute 的 "SM Throughput" 是否接近 peak（接近 = compute-bound，否则 memory-bound）

## 复盘问题

- block-level reduce 为什么需要两层，而不是全用 smem？
- block-level scan 的"offset"是什么，为什么需要它？
- 如果 blockDim.x 不是 32 的倍数（例如 96），warp 间 reduce 如何处理最后一个不完整的 warp？
- 两层混合方案在什么情况下比全 smem 更优？

## 对应官方参考

- CUDA C++ Best Practices: "Reduction" and "Scan" sections
- NVIDIA/cccl: CUB library `BlockReduce` and `BlockScan`
- CUDA Samples: `reduction` sample (多个优化版本)

## 输出对照（printf / std::puts 原文）

- `[D3-T18]` (main.cu:97) 原文：`stub：修复后改为 val` -> 现：`stub: replace with val after fix`
- `[D3-T21]` (main.cu:115) 原文：`stub（初始化）` -> 现：`stub (initialization)`
- `[D3-T30]` (main.cu:151) 原文：`修复后为正确值（stub 是 inc + 0）` -> 现：`correct after fix (stub = inc + 0)`
- `[D3-T31]` (main.cu:154) 原文：`stub：修复后改为 inclusive` -> 现：`stub: replace with inclusive after fix`
- `[D3-T32]` (main.cu:155) 原文：`stub：修复后改为 inclusive - val` -> 现：`stub: replace with inclusive - val after fix`
- `[D3-T36]` (main.cu:217) 原文：`(期望 %d)` -> 现：`(expected %d)`
- `[D3-T37]` (main.cu:228) 原文：`(期望 %d, stub=0)` -> 现：`(expected %d, stub=0)`
- `[D3-T38]` (main.cu:239) 原文：`(期望 %d, stub=0)` -> 现：`(expected %d, stub=0)`
- `[D3-T40]` (main.cu:257) 原文：`inc[255]=%d (期望 %d, stub=0)  exc[1]=%d (期望 %d)` -> 现：`inc[255]=%d (expected %d, stub=0)  exc[1]=%d (expected %d)`
- `[D3-T45]` (main.cu:272) 原文：`[D3] 完成。用 Nsight Compute --set full 查看瓶颈分类。` -> 现：`[D3] done. Use Nsight Compute --set full to view bottleneck classification.`
