# 练习 D2：vote_ballot

## 目标

`[D2-T01]` (main.cu:2) 练习 D2：Warp Vote & Ballot。
`[D2-T02]` (main.cu:3) `__any_sync / __all_sync / __ballot_sync`。

学会 vote (`__any_sync`、`__all_sync`) 和 ballot (`__ballot_sync`) 函数，用 ballot 实现 stream compaction（条件过滤），以及计算 warp-level 的条件统计。

## 前置理解

- 理解 warp 内的 divergence（同一 warp 的线程进入不同分支）
- 理解 ballot 的结果是一个 32-bit 整数，每一 bit 对应一个 lane 的条件结果

## 必做任务

`[D2-T03]` (main.cu:5) 学习目标。
`[D2-T04]` (main.cu:6) `__any_sync` / `__all_sync`：warp 级 OR / AND 归约。
`[D2-T05]` (main.cu:7) `__ballot_sync`：每个 lane 的条件编码为 32-bit 掩码。
`[D2-T06]` (main.cu:8) ballot + `__popc` 实现 stream compaction（过滤满足条件的元素）。
`[D2-T07]` (main.cu:9) 对比 ballot 版 vs shared memory gather 版的正确性与性能。
`[D2-T09]` (main.cu:28) 条件：值 > THRESHOLD（偶数下标的一半满足）。
`[D2-T10]` (main.cu:31) Kernel 1：`__any_sync` 演示。
`[D2-T11]` (main.cu:32) TODO [必做-1]。
`[D2-T12]` (main.cu:39) 条件：任何 lane 的值 > THRESHOLD。
`[D2-T13]` (main.cu:40) TODO [必做-1]。
`[D2-T14]` (main.cu:48) Kernel 2：`__all_sync` 演示。
`[D2-T15]` (main.cu:49) TODO [必做-2]。
`[D2-T16]` (main.cu:56) 条件：所有 lane 的值 > 0（in[] = 1..32，应全为真）。
`[D2-T17]` (main.cu:57) TODO [必做-2]。
`[D2-T18]` (main.cu:65) Kernel 3：`__ballot_sync` 生成掩码。
`[D2-T19]` (main.cu:66) TODO [必做-3]。
`[D2-T20]` (main.cu:73) TODO [必做-3] 每个 lane 的条件（值 > THRESHOLD）编码到 32-bit mask。
`[D2-T21]` (main.cu:81) Kernel 4：stream compaction — ballot 版。
`[D2-T22]` (main.cu:82) 输入：in[]（N 个元素）。
`[D2-T23]` (main.cu:83) 输出：out[]（满足条件的元素，紧凑排列）；out_count：满足条件的数量。
`[D2-T24]` (main.cu:84) TODO [必做-4]。
`[D2-T25]` (main.cu:91) 条件：偶数。
`[D2-T26]` (main.cu:93) Step 1：ballot 得到掩码。
`[D2-T28]` (main.cu:97) Step 2：popcount 得到满足条件的 lane 数。
`[D2-T30]` (main.cu:101) Step 3：计算每个 lane 的输出位置。
`[D2-T31]` (main.cu:102) 掩码中比当前 lane 低的 1-bit 数 = exclusive scan。
`[D2-T33]` (main.cu:106) Step 4：满足条件的 lane 写输出。
`[D2-T35]` (main.cu:114) Kernel 5：stream compaction — smem gather 版（对照）。
`[D2-T36]` (main.cu:115) TODO [必做-5]。
`[D2-T37]` (main.cu:128) TODO [必做-5] 用 atomicAdd 写到 smem（非 warp 级，仅演示）。
`[D2-T38]` (main.cu:135) 拷贝到 global。
`[D2-T53]` (main.cu:259) TODO [必做-6] Nsight Compute PTX 视图：确认 vote.any / ballot 指令。

1. 实现 `__any_sync`：检查同一 warp 的任意 lane 的条件是否为真。例如，任何 lane 的值 > 100。
2. 实现 `__all_sync`：检查同一 warp 的全部 lane 的条件是否全为真。例如，所有 lane 的值都 > 0。
3. 用 `__ballot_sync` 构造一个 32-bit 掩码，其中第 i 位表示 lane i 的条件是否为真。
4. 用 ballot 实现 stream compaction：输入数组，过滤出满足条件的元素（例如偶数），输出到紧凑数组。用 popcount 计算输出大小。
5. 对比 ballot 版本和 shared memory gather 版本的正确性和性能。
6. 在 Nsight Compute 中查看两个版本的 PTX，观察 `vote.any` / `ballot` 指令的分布。

## 进阶任务

`[D2-T54]` (main.cu:260) TODO [进阶] warp 级直方图（多条件 ballot）。
`[D2-T55]` (main.cu:261) TODO [进阶] lane 活跃度可视化（打印每个 warp 有多少 active lane）。

- 实现一个 warp-level 的直方图：统计满足不同条件的 lane 个数，输出为 vector
- 用 ballot 构造一个"lane 活跃度"的可视化（打印每个 warp 有多少 active lane）

## 验收点

- vote 和 ballot 的结果与预期相符
- stream compaction 的输出完全而无重复
- 性能对比显示 ballot 版本相较 smem gather 不差（或更优）
- Nsight Compute PTX 视图确认生成了 vote / ballot 指令

## 观察点

- `__any_sync` 和 `__all_sync` 是归约操作，返回单一布尔值
- `__ballot_sync` 返回 32-bit 掩码，编码了每个 lane 的条件
- ballot + popcount 可以快速计算满足条件的 lane 个数
- divergence 内的 vote/ballot 结果反映实际的 lane 状态分布

## 常见坑

- vote mask 错误，导致某些 lane 被忽略
- 在 divergent branch 外调用 `__activemask()` 而非明确的 sync mask，导致不准确
- 混淆 `__any_sync` 和 `__all_sync` 的语义（any = OR，all = AND）
- ballot 结果理解错误（例如不知道哪个 bit 对应哪个 lane）
- 没有考虑到 popcount 的成本（实际上很快，但仍不是零代价）
- stream compaction 时忘记原子操作或同步，导致输出数据混乱

## 提示

- vote 函数返回 0（false）或 1（true）
- ballot 返回一个 32-bit unsigned int，第 i 位对应 lane i 的条件结果
- popcount（population count）是硬件支持的操作，用于计数 bit 为 1 的个数；CUDA 提供 `__popc`
- stream compaction：先 ballot 得到掩码，再用 exclusive scan 计算每个 lane 的输出位置

## 复盘问题

- 如果同一 warp 的 16 个 lane 满足条件、16 个不满足，`__ballot_sync` 返回多少？
- stream compaction 中，popcount 的结果表示什么？
- 为什么说 `__activemask()` 在某些场景下不如显式 mask 准确？
- vote 和 ballot 相比 shared memory gather 的优势是什么？

## 对应官方参考

- CUDA C++ Programming Guide Section C.21: "Warp Vote Functions"
- CUDA C++ Programming Guide Section C.22: "Warp Shuffle Functions"（ballot 属于 shuffle 家族）
- CUDA C++ Best Practices: "Warp Vote Functions" section
- NVIDIA/cccl: `warp_scan` / `warp_ballot` examples

## 输出对照（printf / std::puts 原文）

- `[D2-T41]` (main.cu:163) 原文：`CPU compact: %d 个偶数，前两个: %d %d` -> 现：`CPU compact: %d evens, first two: %d %d`
- `[D2-T43]` (main.cu:189) 原文：`(期望 1, stub=0)` -> 现：`(expected 1, stub=0)`
- `[D2-T45]` (main.cu:202) 原文：`(期望 1, stub=0)` -> 现：`(expected 1, stub=0)`
- `[D2-T47]` (main.cu:217) 原文：`(期望 %d lanes > %d, stub=0)` -> 现：`(expected %d lanes > %d, stub=0)`
- `[D2-T49]` (main.cu:233) 原文：`count=%d (期望 %d, stub=0)` -> 现：`count=%d (expected %d, stub=0)`
- `[D2-T50]` (main.cu:236) 原文：`前两个: %d %d (期望 %d %d)` -> 现：`first two: %d %d (expected %d %d)`
- `[D2-T52]` (main.cu:254) 原文：`count=%d (期望 %d, stub=0)` -> 现：`count=%d (expected %d, stub=0)`
- `[D2-T56]` (main.cu:271) 原文：`[D2] 完成。TODO 填写后 ballot compact 应与 CPU 参考一致。` -> 现：`[D2] done. After filling TODOs, ballot compact should match CPU reference.`
