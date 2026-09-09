# 练习 G5：warp_specialized_pipeline

## 目标

`[G5-T01]` (main.cu:2) 练习 G5：warp_specialized_pipeline — Hopper producer-consumer warp 专业化 pipeline。
`[G5-T02]` (main.cu:4) 学习目标。
`[G5-T03]` (main.cu:11) 编译命令。
`[G5-T04]` (main.cu:12) 运行命令。
`[G5-T05]` (main.cu:13) 硬件要求。

综合 G1-G4 的所有技术，实现 Hopper 的标志性模式：producer-consumer warp specialization。1 个 producer warp 专职发 TMA 指令搬运数据，7 个 consumer warp 专职执行 wgmma 计算，通过 mbarrier double-buffering 实现计算与搬运的完全重叠。这是后续模块 H（CUTLASS）的基础，也是理解 Hopper 硬件设计的关键。

## 硬件要求

- Compute Capability：**sm_90a 必需**（Hopper GPU，如 H100 / H200）
- CUDA Toolkit：13.x+
- 非 sm_90a 硬件将输出跳过提示并正常退出（`has_hopper_features()` 检测）

## 前置理解

- 完成 G1-G4，理解 wmma / wgmma / TMA / mbarrier 的各个环节
- 理解 warp specialization 的思想：不同线程负责不同职能（I/O vs 计算）
- 理解 double-buffering 的核心思路：buffer A 用于 stage N，buffer B 用于 stage N+1，通过屏障切换

## 必做任务

`[G5-T06]` (main.cu:21) libcu++ barrier。
`[G5-T07]` (main.cu:30) pipeline 参数。
`[G5-T08]` (main.cu:42) Hopper 专属设备代码（sm_90a）。
`[G5-T09]` (main.cu:46) TODO [必做] 步骤 3：producer warp TMA 搬运 stub。
`[G5-T10]` (main.cu:57) TODO [必做] 步骤 3：替换为真实 TMA cp.async.bulk 指令。
`[G5-T11]` (main.cu:69) TODO [必做] 步骤 4：consumer wgmma stub。
`[G5-T12]` (main.cu:79) TODO [必做] 步骤 4：替换为真实 wgmma.mma_async 调用。
`[G5-T13]` (main.cu:85) Kernel：warp-specialized pipeline。
`[G5-T14]` (main.cu:99) TODO [必做] 步骤 2：blockDim = 256；可选添加 `__cluster_dims__(1,1,1)`。
`[G5-T18]` (main.cu:122) TODO [必做] 步骤 5：mbarrier double-buffer 声明。
`[G5-T21]` (main.cu:138) TODO [必做] 步骤 3：producer 减少寄存器（不做计算，不需要多寄存器）。
`[G5-T22]` (main.cu:141) TODO [必做] 步骤 4：consumer 增加寄存器（wgmma 需要更多 accumulator 寄存器）。
`[G5-T24]` (main.cu:153) TODO [必做] 步骤 3 + 5：pipeline 主循环。
`[G5-T28]` (main.cu:185) TODO [必做] 步骤 5：barrier_arrive_tx 通知字节数。
`[G5-T29]` (main.cu:189) TODO [必做] 步骤 5：consumer 等待 barrier。
`[G5-T32]` (main.cu:202) TODO [必做] 步骤 5：切换 buffer（double-buffer 阶段切换）。
`[G5-T33]` (main.cu:206) TODO [必做] 步骤 3：fence.proxy.async 确保 wgmma 结果可见。
`[G5-T34]` (main.cu:212) TODO [必做] 步骤 4：按 wgmma 输出布局将 acc 写回正确地址。
`[G5-T45]` (main.cu:316) TODO [必做] 步骤 6：Nsight Compute 性能对比。

1. `// TODO [必做]` 设计 kernel 线程配置：blockDim = (256, 1, 1)，其中线程 0-31 为 producer warp，线程 32-255 为 7 个 consumer warp。
2. `// TODO [必做]` 在 shared memory 中分配两个 buffer（A 与 B），每个能容纳一个输入 tile（例如 64x64 FP16）。
3. `// TODO [必做]` producer warp 的逻辑：loop over blocks，每次发一个 TMA 搬运指令把下一个 block 搬到 buffer A 或 B（交替）。调用 `barrier_arrive_tx` 告知 consumer 有多少字节待到达。
4. `// TODO [必做]` consumer warp 的逻辑：等待 barrier 就绪（当前 buffer 数据已到），执行 wgmma 乘以当前 buffer 的数据，计算完成后 switch buffer。
5. `// TODO [必做]` 实现 double-buffering 的屏障切换机制：第一个 mbarrier 用于 buffer A 就绪，第二个用于 buffer B 就绪（或用同一 barrier 的多个 generation）。
6. `// TODO [必做]` 测量整个 kernel 的吞吐量（TFLOPS），对比 G3 的单纯 wgmma 版本。Nsight Compute 中观察 SM Occupancy、Memory Throughput、Tensor Core Utilization、warp scheduling 情况。

## 进阶任务

`[G5-T15]` (main.cu:107) 线程标识。
`[G5-T16]` (main.cu:112) 该 block 负责的输出 tile。
`[G5-T17]` (main.cu:117) shared memory 分配。
`[G5-T19]` (main.cu:131) （stub 用 __syncthreads 代替 barrier）。
`[G5-T20]` (main.cu:135) setmaxnreg 为 producer/consumer 分配不同寄存器。
`[G5-T23]` (main.cu:147) 每 consumer thread 持有的累加器（stub：2 个 FP32）。
`[G5-T25]` (main.cu:168) 注意：pipeline flush（最后几个 stage）需要特殊处理。
`[G5-T26]` (main.cu:174) producer：TMA 搬运 A tile。
`[G5-T27]` (main.cu:180) producer：TMA 搬运 B tile。
`[G5-T30]` (main.cu:194) stub 同步替代 barrier。
`[G5-T31]` (main.cu:198) consumer：wgmma 计算。
`[G5-T46]` (main.cu:324) TODO [进阶] 三缓冲/四缓冲，进一步重叠计算与搬运。
`[G5-T47]` (main.cu:325) TODO [进阶] 不同 tile 大小（32x32 / 64x64 / 128x128），观察 buffer/寄存器/吞吐权衡。
`[G5-T48]` (main.cu:326) TODO [进阶] 集成 Blackwell sm_100a MXFP8 MMA，验证 producer-consumer 模式通用性。

- 实现三缓冲或四缓冲，进一步重叠计算与搬运
- 尝试不同的 tile 大小（32x32 vs 64x64 vs 128x128），观察 buffer 大小、寄存器压力、整体吞吐的权衡
- 集成 Blackwell sm_100a 的新 MMA 形式（如 MXFP8），验证 producer-consumer 模式的通用性

## 验收点

`[G5-T35]` (main.cu:229) CPU 参考。
`[G5-T36]` (main.cu:248) 主程序入口。
`[G5-T37]` (main.cu:257) Hopper 特性检测。
`[G5-T38]` (main.cu:264) 矩阵尺寸：M=128, N=256, K=64（2x2 tile 网格）。
`[G5-T39]` (main.cu:288) `warp_specialized_pipeline_kernel` 启动。

- kernel 编译通过，sm_90a 硬件上运行
- GEMM 结果与 CPU 参考或前述单 block 版本逐元素一致
- warp specialization 版本的吞吐明显高于 non-specialized 版本（通常快 30-50%）
- Nsight Compute 显示 producer warp 与 consumer warp 的时间线错开（通过 Warp Occupancy 或 PTX timeline 可见）
- barrier 的 expected-tx count 与实际搬运字节数匹配

## 观察点

- producer warp 与 consumer warp 的分工完全隔离，producer 可以提前发 TMA 指令，consumer 可以安心计算
- double-buffering 的效果：当 consumer 使用 buffer A 计算时，producer 已经开始向 buffer B 搬数据，完美重叠
- 寄存器压力在 warp specialization 中可能更高（因为每个 warp 专注于一项任务，可能用更多寄存器）
- `setmaxnreg` 可用来为 producer/consumer 分配不同的寄存器配额

## 常见坑

- producer warp 与 consumer warp 的屏障同步不当，导致 consumer 过早开始计算或 producer 过度抢占带宽
- buffer 大小不足，无法容纳一个完整的 tile，导致溢出或 TMA 异常
- mbarrier 的 expected-tx 初始化错误，导致 consumer 永远等待或过早唤醒
- 没有正确处理 pipeline flush：最后几个 stage 需要特殊处理，确保所有数据都被消费完
- producer warp 的 TMA 指令发出后没有持续进行，导致搬运间隙出现，consumer 必须等待
- consumer warp 之间的数据竞争（如果不同 consumer 访问同一 buffer 的不同部分需要同步）
- 在 non-Hopper 硬件上运行导致 wgmma 或 `__cluster_dims__` 不支持

## 提示

- mbarrier 双缓冲：第 N 个 block 完成时，切换到第 N+1 个 barrier 对象或同一对象的不同 generation
- producer warp 调用 `cuda::device::barrier_arrive_tx(barrier, byte_count)`，consumer 调用 `barrier.wait()`
- warp specialization 中常用 `__syncthreads()` 在整个 block 层面做一次初始化同步，之后 producer/consumer 各自独立
- Nsight Compute 的 Warp Occupancy 或 Active Warps 指标可用来确认 warp specialization 的并发效果
- 调试技巧：用 shared memory 中的计数器记录 producer/consumer 的进度，kernel 结束后读出打印

## 复盘问题

- warp specialization 相比全部 warp 都做计算的模式，为什么能提升整体吞吐？
- double-buffering 何时最有效，buffer 数量过多（三缓冲+）还有收益吗？
- producer warp 与 consumer warp 之间的"契约"是什么，mbarrier 在其中扮演什么角色？
- 如果 producer warp 的 TMA 搬运速度跟不上 consumer 的计算速度，会发生什么？

## 对应官方参考

- Hopper Tuning Guide Section Warp Specialization
- Hopper Tuning Guide Section Double-Buffering with mbarrier
- NVIDIA Blog Warp Specialization in Hopper
- CUTLASS 3.x examples `hopper_warp_specialized_gemm`
- CUDA C++ Programming Guide Section 3.2.9: Synchronization Primitives

## 输出对照（printf / std::puts 原文）

- `[G5-T40]` (main.cu:296) 原文：`启动 warp_specialized_pipeline_kernel: ...` -> 现：`launch warp_specialized_pipeline_kernel: grid=(...) block=(...)`
- `[G5-T41]` (main.cu:298) 原文：`  producer warp: tid 0-31   (... warp)` -> 现：`  producer warp: tid 0-31   (%d warp)`
- `[G5-T42]` (main.cu:299) 原文：`  consumer warp: tid 32-255 (... warps)` -> 现：`  consumer warp: tid 32-255 (%d warps)`
- `[G5-T43]` (main.cu:300) 原文：`  double-buffer stages: ...` -> 现：`  double-buffer stages: %d`
- `[G5-T44]` (main.cu:310) 原文：`[warp_specialized_pipeline] ... ms (stub — TODO [必做] 步骤 3-5 完成后验证结果)` -> 现：`[warp_specialized_pipeline] %.3f ms (stub - verify result after TODO [REQUIRED] steps 3-5)`
- `[G5-T49]` (main.cu:331) 原文：`[G5] 完成。用 ncu --set full ./G5_warp_specialized_pipeline 观察 producer/consumer warp 调度。` -> 现：`[G5] done. Use ncu --set full ./G5_warp_specialized_pipeline to observe producer/consumer warp scheduling.`
