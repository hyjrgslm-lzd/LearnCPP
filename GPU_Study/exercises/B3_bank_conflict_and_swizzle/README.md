# 练习 B3：bank_conflict_and_swizzle

## 目标

`[B3-T01]` (main.cu:2) 练习 B3：bank_conflict_and_swizzle。
`[B3-T02]` (main.cu:3) 深入理解 shared memory 的 32 bank 模型：观察 bank conflict 如何导致串行化，学会用 padding 消除。
`[B3-T03]` (main.cu:7) 验收：stride-32 版吞吐 << stride-1 版（~30 倍差距）；Nsight Compute 命令 `ncu --set full -o b3.ncu-rep ./B3_bank_conflict_and_swizzle.exe` 查看 `shared_ld_bank_conflict` / `shared_st_bank_conflict` 指标。

深入理解 shared memory 的 32 bank 模型。观察 bank conflict 如何导致序列化访问。学会用 padding 和 swizzle 消除 conflict。

## 前置理解

- 完成 B2。
- 你知道 shared memory 被分成 32 个 bank。
- 接受"同一 warp 内多个线程同时访问同一 bank 会被串行化"。

## 必做任务

`[B3-T08]` (main.cu:30) Kernel 1：stride-1 访问（无 bank conflict）。
`[B3-T09]` (main.cu:32) TODO [必做] 步骤 1：理解为何此 kernel 无 conflict。bank_id = i % 32，32 线程对应 32 不同 bank。
`[B3-T10]` (main.cu:39) TODO [必做] 在 INNER_LOOP 次循环内做 stride-1 写读和 sync。
`[B3-T11]` (main.cu:46) TODO [必做] 使用 sdata[tid]（stride-1）。
`[B3-T12]` (main.cu:54) Kernel 2：stride-32 访问（最坏 bank conflict）。
`[B3-T13]` (main.cu:56) TODO [必做] 步骤 2：理解为何此 kernel 全部冲突。所有 32 线程访问 bank 0。
`[B3-T15]` (main.cu:65) TODO [必做] `int idx = tid * 32;` 步长 32，全线程命中 bank 0。
`[B3-T16]` (main.cu:77) Kernel 3：padding 消除 conflict。
`[B3-T17]` (main.cu:79) TODO [必做] 步骤 5：理解 padding 原理。
`[B3-T19]` (main.cu:90) TODO [必做] 写入和读取，与 stride-1 相同但加了 padding。
`[B3-T40]` (main.cu:217) TODO [必做] 步骤 4/6：运行 Nsight Compute。

1. 写一个 kernel `access_stride_1`，stride-1 访问 shared memory（无 conflict）：

```cpp
// TODO [必做] sdata[threadIdx.x] = ...;  // thread i 访问 bank i，完全无冲突
```

2. 写一个 kernel `access_stride_32`，stride-32 访问（最坏情况，全部冲突）：

```cpp
// TODO [必做] int idx = threadIdx.x * 32;
// TODO [必做] sdata[idx] = ...;  // 所有线程访问同一 bank
```

3. 在两个 kernel 上运行大量操作（INNER_LOOP 次循环），计时。stride-32 版本应该慢 30+ 倍。
4. 用 Nsight Compute 查看两个版本的 `shared_ld_bank_conflict` / `shared_st_bank_conflict` 指标：

```
// TODO [必做] ncu --set full -o b3.ncu-rep ./B3_bank_conflict_and_swizzle.exe
```

5. 写一个 kernel `access_with_padding`，用 padding 消除冲突。对于 32 个 float 的数组，加 padding 变成 33 个 float：

```cpp
// TODO [必做] __shared__ float sdata[32 + 1];  // 多一个 float 作 padding
// TODO [必做] sdata[threadIdx.x] = ...;  // 现在地址偏移变化，thread i 访问 bank i（无冲突）
```

6. 用 padding 版本重新运行计时，性能应该接近 stride-1 版本。

## 进阶任务

`[B3-T20]` (main.cu:101) 进阶：32x32 矩阵转置 — naive vs padded。
`[B3-T21]` (main.cu:103) TODO [进阶] 实现三个版本：naive、padded、Nsight Compute 对比。
`[B3-T22]` (main.cu:121) 转置后写出（naive 版）。
`[B3-T23]` (main.cu:131) +1 padding 消除 conflict（padded 版）。

- 在 Hopper (sm_90a) 上试用 `cp.async.bulk` 的 swizzle 寄存器，或用 libcu++ 的 swizzle 模板。
- 实现一个"对角线冲突"的访问模式（如 thread i 访问位置 i + i*32），观察冲突的复杂性。
- 实现 32x32 矩阵转置：naive 版（有 bank conflict）+ padded 版（`[32][33]`，无 conflict），用 Nsight Compute 对比。

## 验收点

`[B3-T04]` (main.cu:21) 常量段。
`[B3-T05]` (main.cu:23) BLOCK_SZ = 32（一个 warp）。
`[B3-T06]` (main.cu:24) INNER_LOOP = 10000。
`[B3-T07]` (main.cu:25) GRID_SZ = 1024。
`[B3-T14]` (main.cu:60) sdata 大小足够容纳步长。
`[B3-T18]` (main.cu:85) +1 padding。
`[B3-T24]` (main.cu:144) 计时辅助。
`[B3-T25]` (main.cu:147) 预热。
`[B3-T26]` (main.cu:160) main 入口。
`[B3-T27]` (main.cu:165) Bank ID 公式说明。
`[B3-T31]` (main.cu:181) 测试 stride-1 段。
`[B3-T33]` (main.cu:191) 测试 stride-32 段。
`[B3-T35]` (main.cu:201) 测试 padding 段。
`[B3-T39]` (main.cu:215) Nsight Compute 提示段。

- stride-1 版本和 padding 版本的吞吐相近（比值 ~1x）。
- stride-32 版本的吞吐显著低于 stride-1（比值 ~30x）。
- Nsight Compute 的 `shared_ld_bank_conflict` 和 `shared_st_bank_conflict` 指标能证明。
- 程序运行无错。

## 观察点

- 32 bank，每 bank 4 字节。thread i 访问地址 `addr` 时，bank ID = `(addr / 4) % 32`。
- 同一 warp 的线程同时访问同一 bank 时，硬件串行化这些访问，每次 4 字节。
- padding 通过改变地址偏移，使得原本冲突的访问落在不同 bank。
- Hopper 引入了 swizzle 寄存器，可以在硬件级动态处理冲突（见 B5 和模块 G）。

## 常见坑

1. **误以为 bank conflict 只影响读**：write conflict 也会导致串行化。
2. **padding 大小计算错误**：不同数据类型有不同的 bank mapping。float 是 4 字节，double 是 8 字节。
3. **以为 L1 缓存能避免 conflict**：shared memory 是 on-chip 但不经过 L1，conflict 是直接的。
4. **测量时没有足够的操作量**：如果只做一次访问，计时噪声很大。需要循环许多次（本例 INNER_LOOP=10000）。
5. **没有考虑多个 warp**：一个 block 可能有多个 warp。每个 warp 独立产生的 bank conflict 会相加。

## 提示

- Bank ID 公式：`bank_id = (address_in_bytes / 4) % 32`（假设 int/float）。
- padding 一般就是加 1 个元素（4 字节），就能消除绝大多数 32-thread warp 的冲突。
- 要手算 bank conflict，先算出每个线程访问的地址，再算 bank_id，看是否有多个线程落在同一 bank。
- Nsight Compute 的 `shared_ld_bank_conflict` 是累计冲突数，不是百分比。用它和总 load 数对比。

## 复盘问题

1. shared memory 的 bank ID 是如何计算的？
2. stride-1 访问为什么无 conflict，stride-32 为什么全冲突？
3. padding 消除冲突的原理是什么？
4. 如果 block 有两个 warp，每个 warp 独立有 conflict，最终会怎样？
5. Nsight Compute 中的 `shared_ld_bank_conflict` 数值如何解读？

## 对应官方参考

- CUDA C++ Best Practices Guide / Shared Memory: https://docs.nvidia.com/cuda/cuda-c-best-practices-guide/
- Nsight Compute / Shared Memory Analysis: https://docs.nvidia.com/nsight-compute/

## 输出对照（printf / std::puts 原文）

- `[B3-T28]` (main.cu:170) 原文：`stride-1:  thread i -> bank i        (无冲突)` -> 现：`stride-1:  thread i -> bank i           (no conflict)`
- `[B3-T29]` (main.cu:171) 原文：`stride-32: thread i -> bank 0        (32-way 冲突)` -> 现：`stride-32: thread i -> bank 0           (32-way conflict)`
- `[B3-T30]` (main.cu:172) 原文：`padding:   改变偏移，thread i -> bank i+1 (无冲突)` -> 现：`padding:   shifts offsets, thread i -> bank i+1 (no conflict)`
- `[B3-T32]` (main.cu:189) 原文：`[B3] stride-1  耗时: %.3f ms/run` -> 现：`[B3] stride-1  time: %.3f ms/run`
- `[B3-T34]` (main.cu:199) 原文：`[B3] stride-32 耗时: %.3f ms/run` -> 现：`[B3] stride-32 time: %.3f ms/run`
- `[B3-T36]` (main.cu:209) 原文：`[B3] padding   耗时: %.3f ms/run` -> 现：`[B3] padding   time: %.3f ms/run`
- `[B3-T37]` (main.cu:212) 原文：`stride-32 / stride-1 比值 (期望 ~30x)` -> 现：`stride-32 / stride-1 ratio (expected ~30x)`
- `[B3-T38]` (main.cu:215) 原文：`padding / stride-1 比值 (期望 ~1x)` -> 现：`padding / stride-1 ratio (expected ~1x)`
- `[B3-T41]` (main.cu:220) 原文：`查看 shared_ld_bank_conflict / shared_st_bank_conflict 指标。` -> 现：`Inspect shared_ld_bank_conflict / shared_st_bank_conflict metrics.`
