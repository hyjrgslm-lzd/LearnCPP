# 练习 C5：mbarrier_async_barrier

## 目标

`[C5-T01]` (main.cu:1) 文件标识：`C5_mbarrier_async_barrier/main.cu`。
`[C5-T02]` (main.cu:2) 练习 C5：`cuda::barrier` (libcu++) — producer/consumer 异步同步模式。
`[C5-T03]` (main.cu:4) 学习目标：在 smem 中声明并初始化 `cuda::barrier<cuda::thread_scope_block>`；producer warp 用 `arrive_and_drop` / `arrive`，consumer 用 wait；`barrier_arrive_tx` 传递预期字节数（为 TMA 交互预热）；对比 `__syncthreads` 全量等待 vs mbarrier 部分线程等待。
`[C5-T04]` (main.cu:11) 编译与运行命令；注意 sm_80+ 支持 `cuda::barrier`，Hopper sm_90a 还支持 `barrier_arrive_tx`。

学会用 `cuda::barrier` 和 mbarrier 原语构建异步同步模式：producer 和 consumer warp 可以独立前进，只在需要时同步。这是后续 TMA（模块 G）和 warp specialization pipeline 的关键基础。

## 前置理解

- 完成前面四个练习，掌握 `__syncthreads` 和 cooperative groups
- 理解异步编程的概念：不是所有线程都需要卡在屏障上
- 理解 libcu++ 的 `<cuda/barrier.hpp>` 头文件（随 CUDA Toolkit 附带）

## 必做任务

`[C5-T05]` (main.cu:18) libcu++ barrier（随 CUDA Toolkit 附带）。
`[C5-T06]` (main.cu:25) 常量定义。
`[C5-T07]` (main.cu:29) `SMEM_WORDS`：smem 数组大小。
`[C5-T08]` (main.cu:30) `PRODUCER_WARPS`：前 1 个 warp 为 producer。
`[C5-T09]` (main.cu:33) Kernel 1：`__syncthreads` 版（对照组）。
`[C5-T10]` (main.cu:42) 所有线程写（模拟 producer）。
`[C5-T11]` (main.cu:43) 全量等待。
`[C5-T12]` (main.cu:45) 所有线程读（模拟 consumer）。
`[C5-T13]` (main.cu:49) Kernel 2：`cuda::barrier` 版；前 `PRODUCER_WARPS` 个 warp 写 smem，然后 `arrive_and_drop`；其余 warp（consumer）在 `bar.arrive_and_wait` 处等待。
`[C5-T14]` (main.cu:54) TODO [必做-1] 声明 smem barrier。
`[C5-T15]` (main.cu:55) TODO [必做-2] producer `arrive_and_drop` / consumer `arrive_and_wait`。
`[C5-T16]` (main.cu:56) TODO [必做-3] `barrier_arrive_tx`（预期字节数）。
`[C5-T17]` (main.cu:60) TODO [必做-1] 在 smem 中声明 barrier，初始化为 `blockDim.x`，且必须由单个线程（通常 tid == 0）完成。
`[C5-T18]` (main.cu:73) producer warp：写数据，然后 arrive。
`[C5-T19]` (main.cu:76) TODO [必做-2] producer 完成后 `arrive_and_drop`（不等待）。
`[C5-T20]` (main.cu:78) consumer warp：arrive 并等待 producer 完成。
`[C5-T21]` (main.cu:80) TODO [必做-2] consumer `arrive_and_wait`。
`[C5-T22]` (main.cu:83) 读取 producer 写入的数据。
`[C5-T23]` (main.cu:87) Stub：写 0，学生完成 TODO 后应有正确值。
`[C5-T24]` (main.cu:91) Kernel 3：`barrier_arrive_tx` 演示（Hopper 特性）。
`[C5-T25]` (main.cu:92) TODO [必做-3] 为 TMA-like 场景设置预期字节数。
`[C5-T26]` (main.cu:96) TODO [必做-3] 声明支持 `arrive_tx` 的 barrier。
`[C5-T27]` (main.cu:106) 模拟 TMA 搬运：预告将写入 N 字节。
`[C5-T28]` (main.cu:114) consumer 等待（TODO [必做-3]）。
`[C5-T29]` (main.cu:117) fallback：用 `__syncthreads` 代替，TODO 完成后替换。
`[C5-T30]` (main.cu:119) TODO: 完成后验证值为 `tid * 5`。
`[C5-T31]` (main.cu:121) Stub。

1. 在 kernel 中包含 `#include <cuda/barrier.hpp>` 并声明一个 `cuda::barrier<cuda::thread_scope_block>` 作为 shared memory 对象。初始化为 block 内线程总数。
2. 实现一个简单的 producer-consumer 模式：某些线程（producer）往 smem 写数据，其他线程（consumer）读数据。producer 写完后调用 `.arrive_and_drop()`，consumer 在 `.arrive_and_wait()` 处等待。
3. 用 `barrier_arrive_tx` 传递"预期字节数"给 TMA（虽然这个练习不用真正的 TMA，但要传入合理的值）。观察 mbarrier 如何追踪 transaction 计数。
4. 对比 `__syncthreads` 版本和 mbarrier 版本：前者要求所有线程都到达屏障，后者允许某些线程提前完成并离开。测量两个版本的性能差异。
5. 在 Nsight Compute 中用 PTX 视图查看 mbarrier 对应的 `barrier.arrive` / `barrier.wait` 指令。
6. 验证 mbarrier transaction count 的正确性：如果 producer 声明了 N 字节但实际没写完，consumer 会怎样？

## 进阶任务

`[C5-T39]` (main.cu:178) TODO [必做-4] 对比性能差异（sync vs mbarrier）。
`[C5-T40]` (main.cu:179) TODO [必做-5] Nsight Compute PTX 视图查看 `barrier.arrive` / `barrier.wait`。
`[C5-T41]` (main.cu:180) TODO [必做-6] 验证 transaction count 错误时的行为。
`[C5-T42]` (main.cu:181) TODO [进阶] 多轮 pipeline：双 mbarrier 交替 phase。
`[C5-T43]` (main.cu:182) TODO [进阶] `barrier_arrive_tx` 准确预测 TMA 字节数。

- 实现一个多轮 pipeline：wave 1 的 producer 和 wave 2 的 consumer 用不同的 mbarrier 实现重叠计算
- 尝试用 `barrier_arrive_tx` 准确预测 TMA 的搬运字节数

## 验收点

`[C5-T32]` (main.cu:124) 主程序入口。
`[C5-T33]` (main.cu:138) 对照组：`__syncthreads` 版。
`[C5-T35]` (main.cu:152) mbarrier 版。
`[C5-T37]` (main.cu:166) `barrier_arrive_tx` 演示。

- kernel 编译通过，mbarrier 相关代码无编译错误
- producer 和 consumer 的执行顺序正确，数据完整传输
- Nsight Compute PTX 视图中能看到 `barrier.arrive_tx` 和 `barrier.wait` 指令
- transaction count 的计数与预期一致

## 观察点

- mbarrier 是 Hopper 引入的高级同步原语，允许异步的 arrive/wait 分离
- `barrier_arrive_tx` 用于告知 TMA 预期的数据量，TMA 会自动递减 transaction count
- 相比 `__syncthreads` 要求全部线程同步，mbarrier 允许部分线程提前完成
- 这为 warp specialization（某些 warp 专职 TMA 搬运）奠定基础

## 常见坑

- 初始化 mbarrier 时没有设置正确的 thread count（应等于 block 内的线程总数）
- 混淆 `arrive_and_drop`（表示该线程不再参与后续同步）和 `arrive_and_wait`（表示该线程等待）
- `barrier_arrive_tx` 的字节数计算错误，导致 consumer 等待超时或过早唤醒
- 没有考虑到 mbarrier 是 libcu++ 的抽象，底层仍是 PTX 指令，跨平台兼容性有限
- 在非 Hopper 硬件上编译，可能无法生成正确的 PTX

## 提示

- mbarrier 初始化：`cuda::barrier<cuda::thread_scope_block> bar(blockDim.x);` 在 shared memory 中声明
- arrive_and_drop：该线程完成工作，不再参与此屏障
- arrive_and_wait：该线程完成工作，等待其他线程（需 `__syncthreads()` 样式的同步）
- barrier_arrive_tx(bar, tx_count) 用于 TMA 搬运计数（下个练习会真正用上）
- PTX 对应：mbarrier.arrive_tx 对应 `barrier.arrive.acquire.shared::cta.b64 ...`

## 复盘问题

- 为什么说 mbarrier 比 `__syncthreads` 更"异步"？
- producer 调用 `arrive_and_drop` 后，consumer 的 `arrive_and_wait` 会立刻返回吗？
- `barrier_arrive_tx` 的字节数应该等于多少？是 TMA 搬运的实际字节数还是预期字节数？
- 如果 transaction count 计数错误，会导致什么样的 bug（hang、错误结果）？

## 对应官方参考

- CUDA C++ Programming Guide Section 3.2.9: "Synchronization Primitives"
- libcu++ `<cuda/barrier.hpp>` header (libcu++ documentation)
- Hopper Tuning Guide: "Async Barriers and mbarrier" section
- CUDA PTX ISA: `barrier.*` instructions

## 输出对照（printf / std::puts 原文）

- `[C5-T34]` (main.cu:148) 原文：`[sync]  out[0]=%d out[1]=%d  %.3f ms` → 现：保持英文不变
- `[C5-T36]` (main.cu:163) 原文：`[mbar]  out[0]=%d out[32]=%d  %.3f ms  (stub=0)` → 现：保持英文不变
- `[C5-T38]` (main.cu:175) 原文：`[tx]    out[0]=%d out[1]=%d  (期望 0, 5 — stub=0)` → 现：`[tx]    out[0]=%d out[1]=%d  (expected 0, 5 - stub=0)`
- `[C5-T44]` (main.cu:191) 原文：`[C5] 完成。参考 libcu++ <cuda/barrier> 文档完成 TODO。` → 现：`[C5] done. Refer to libcu++ <cuda/barrier> docs to complete the TODOs.`
