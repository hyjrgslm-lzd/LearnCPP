# 练习 C2：syncthreads_and_divergence

## 目标

`[C2-T01]` (main.cu:1) 文件标识：`C2_syncthreads_and_divergence/main.cu`。
`[C2-T02]` (main.cu:2) 练习 C2：`__syncthreads` 正确用法与 divergent branch 陷阱。
`[C2-T03]` (main.cu:4) 学习目标：理解 `__syncthreads` 是 block 级屏障；divergent branch 内调用会产生未定义行为；共享内存 reduction 每一步都必须 sync；使用 `compute-sanitizer --tool synccheck` 发现 sync 问题。
`[C2-T04]` (main.cu:10) 编译、运行与 sanitizer 命令。

学会 `__syncthreads` 的正确用法，理解为什么 divergent branch 内调用 sync 会导致 undefined behavior，以及如何用 shared memory reduction 来体验"每一步都需要 sync"的必要性。

## 前置理解

- 理解 shared memory 的所有权与生命周期（per-block，block 内所有线程共享）
- 理解"divergence"：同一 warp 内的线程进入不同的 if/else 分支
- 理解 `__syncthreads()` 的含义：block 内全体线程必须到达这一点，才能继续

## 必做任务

`[C2-T05]` (main.cu:24) 常量定义。
`[C2-T06]` (main.cu:27) `N == BLOCK_SIZE`：单 block 演示。
`[C2-T07]` (main.cu:30) Kernel 1：shared memory 写入后读取（基础验证）。
`[C2-T08]` (main.cu:31) TODO [必做-1]。
`[C2-T09]` (main.cu:38) 每个线程写入自己的 `threadIdx`。
`[C2-T10]` (main.cu:42) TODO [必做-1] 加 `__syncthreads()`，再从其他位置读。
`[C2-T11]` (main.cu:45) 读取相邻线程的值（循环），此处在 sync 前读会得到错误值。
`[C2-T12]` (main.cu:50) Kernel 2：divergent branch 内的 `__syncthreads`（故意错误）。
`[C2-T13]` (main.cu:51) TODO [必做-2] 用 `compute-sanitizer --tool synccheck` 检测此 kernel。
`[C2-T14]` (main.cu:53) 警告：在真实 GPU 上可能 hang；调试时用 synccheck 的 `--report-api-errors`。
`[C2-T15]` (main.cu:60) TODO [必做-2] 奇数 tid 调用了 `__syncthreads`，偶数没有 → UB / hang。
`[C2-T16]` (main.cu:69) 暂时写 0 以便代码能编译通过。
`[C2-T17]` (main.cu:73) Kernel 3：错误的 reduction（缺少某个 sync）。
`[C2-T18]` (main.cu:74) TODO [必做-3/4] `bad_reduce`：学生需要找出缺失的 `__syncthreads`。
`[C2-T19]` (main.cu:84) TODO [必做-3] reduction 缺少 `__syncthreads`，结果不正确。
`[C2-T20]` (main.cu:90) TODO [必做-3] 在这里加 `__syncthreads()`。
`[C2-T21]` (main.cu:93) stub：写 0，学生修复后应写 `smem[0]`。
`[C2-T22]` (main.cu:96) Kernel 4：正确的 reduction（每步都 sync）。
`[C2-T23]` (main.cu:97) TODO [必做-3] `good_reduce`：参考实现。
`[C2-T24]` (main.cu:108) TODO [必做-3] 完成正确的 strided reduction。
`[C2-T25]` (main.cu:113) 每步都必须 sync。
`[C2-T26]` (main.cu:116) stub：修复后改为 `smem[0]`。
`[C2-T27]` (main.cu:119) Kernel 5：bank conflict aware reduction。
`[C2-T28]` (main.cu:120) TODO [必做-5]。
`[C2-T29]` (main.cu:131) TODO [必做-5] 使用偏移访问减少 bank conflict。
`[C2-T30]` (main.cu:141) stub。
`[C2-T31]` (main.cu:144) CPU 参考 reduction。

1. 写一个简单的 shared memory 写入场景：每个线程往 smem 数组对应位置写入自己的 threadIdx，然后调用 `__syncthreads()`，再从其他位置读。验证读写顺序正确。
2. 在 if 分支内调用 `__syncthreads()`，使得某些线程进入 if、某些不进入。观察编译是否报错，运行时是否 hang 或得到错误结果。
3. 写一个共享内存的 reduction kernel：每个线程贡献一个值到共享数组，第一层 reduce 用 strided 方式写回，每一步都加 `__syncthreads()`。
4. 在 reduction 的每一层中删除一个 `__syncthreads()`，观察 Nsight 或 compute-sanitizer --tool synccheck 的诊断结果。
5. 实现一个带 bank conflict awareness 的 reduction（错开访问以减少 bank conflict），对比有无 sync 的差异。
6. 用 compute-sanitizer --tool synccheck 扫描上述 kernel，记录找到的 sync 问题。

## 进阶任务

`[C2-T46]` (main.cu:226) TODO [进阶] 三层 reduction：block 内 → block 间 + atomic。
`[C2-T47]` (main.cu:227) TODO [进阶] 用 warp shuffle 替代某一层 reduction。

- 写一个三层 reduction：block 内 → block 间 + atomic，在每一层清晰标记 sync 点
- 尝试用 warp shuffle 替代某一层 reduction，对比 smem sync 的需求变化

## 验收点

`[C2-T32]` (main.cu:152) 主程序入口。
`[C2-T33]` (main.cu:158) 准备数据。
`[C2-T34]` (main.cu:163) 输入数据填 1..256。
`[C2-T36]` (main.cu:171) 测试 1：smem 写入读取。
`[C2-T38]` (main.cu:185) 测试 2：bad divergent sync（不要在 sanitizer 外直接跑）。
`[C2-T39]` (main.cu:188) TODO [必做-2] 取消注释后用 `compute-sanitizer --tool synccheck` 运行。
`[C2-T41]` (main.cu:198) 测试 3：bad reduce vs good reduce。
`[C2-T44]` (main.cu:222) 测试 4：bank conflict aware reduce。

- 代码能清楚地运行、无 race condition、synccheck 无 error
- 能指出"如果删除某个 `__syncthreads()`，会在哪一步产生 data race"
- shared memory reduction 的每一层结果与 CPU 参考实现一致
- Nsight Compute 中能看到 shared_ld_bank_conflict 的报告（如适用）

## 观察点

- `__syncthreads` 是 block 级的屏障，不是 warp 级的
- divergence 内的 sync 会导致某些线程永远卡住（subset of warp 到达 sync 点，subset 没到达）
- shared memory reduction 需要在每一层都 sync，否则下一层的读会看到旧数据
- bank conflict 影响的是 smem 访问延迟，不是正确性，但会大幅降低性能

## 常见坑

- 在条件分支内调用 `__syncthreads`，即使是"所有线程最终都会到达"的逻辑也会产生 UB
- 写 shared memory 后立刻读，忘记 `__syncthreads()`，导致读到旧值或不一致
- reduction 的每一层没有相应的 sync，使得后续层看到的是混乱的数据
- 假设同一 warp 内的线程"自动同步"，结果在 smem 上依然产生 race
- 在 loop 里写 smem，loop 前只有一个 sync，导致不同迭代的数据混乱
- 过度同步：在根本不需要同步的地方加 sync，造成性能下降
- 混淆 `__syncthreads()` 和 `__syncwarp()` 的作用范围
- 没有考虑到有些线程可能不参与后续计算（例如 threadIdx >= N），导致这些线程卡在 sync 上

## 提示

- divergence 判断规则：如果同一 warp 的两个线程执行路径不同，即为 divergence
- shared memory reduction 的标准模式：`for(int s = blockDim.x/2; s > 0; s >>= 1) { if(threadIdx.x < s) smem[threadIdx.x] += smem[threadIdx.x + s]; __syncthreads(); }`
- bank conflict：shared memory 按 32 字节宽的 bank 划分，连续 4 字节地址分别进入 bank 0/1/2/...；同一 bank 的并发访问串行化
- 用 `compute-sanitizer --tool synccheck ./executable` 来检测 sync 问题

## 复盘问题

- 为什么 divergence 内的 `__syncthreads` 会导致 hang？（提示：warp 不会分割等待）
- shared memory reduction 的每一层为什么都需要 sync？删除其中某个会怎样？
- 如果 blockDim.x = 1024，第一层 reduction 有多少个 warp 参与？为什么这很重要？
- bank conflict 和 race condition 哪个更容易在 synccheck 中被检出？

## 对应官方参考

- CUDA C++ Programming Guide Section 3.2.3: "Synchronization Functions"
- CUDA C++ Best Practices Section 4.1.3: "Synchronize to Ensure Data Visibility"
- compute-sanitizer User Guide: synccheck tool
- CUDA C++ Programming Guide Appendix B.3: "Shared Memory"

## 输出对照（printf / std::puts 原文）

- `[C2-T35]` (main.cu:165) 原文：`CPU reduce 参考结果 = %d` → 现：`CPU reduce reference result = %d`
- `[C2-T37]` (main.cu:181) 原文：`[必做-1] smem_write_read_kernel 完成（TODO: 加 sync 后验证结果）` → 现：`[REQUIRED-1] smem_write_read_kernel done (TODO: validate after adding sync)`
- `[C2-T40]` (main.cu:194) 原文：`[必做-2] bad_divergent_sync_kernel: 用 synccheck 观察 UB（kernel 已注释）` → 现：`[REQUIRED-2] bad_divergent_sync_kernel: observe UB with synccheck (kernel commented out)`
- `[C2-T42]` (main.cu:208) 原文：`[必做-3] bad_reduce  结果 = %d (期望 %d, 此处为 stub=0)` → 现：`[REQUIRED-3] bad_reduce  result = %d (expected %d, currently stub=0)`
- `[C2-T43]` (main.cu:216) 原文：`[必做-3] good_reduce 结果 = %d (期望 %d, 此处为 stub=0)` → 现：`[REQUIRED-3] good_reduce result = %d (expected %d, currently stub=0)`
- `[C2-T45]` (main.cu:233) 原文：`[必做-5] bc_aware_reduce 结果 = %d (期望 %d, 此处为 stub=0)` → 现：`[REQUIRED-5] bc_aware_reduce result = %d (expected %d, currently stub=0)`
- `[C2-T48]` (main.cu:243) 原文：`[C2] 完成。提示：compute-sanitizer --tool synccheck ./C2_syncthreads_and_divergence` → 现：`[C2] done. Tip: compute-sanitizer --tool synccheck ./C2_syncthreads_and_divergence`
