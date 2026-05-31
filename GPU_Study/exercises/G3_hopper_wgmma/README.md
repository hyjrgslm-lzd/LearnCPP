# 练习 G3：hopper_wgmma

## 目标

`[G3-T01]` (main.cu:2) 练习 G3：hopper_wgmma — Hopper 专属 warp-group MMA 异步指令。
`[G3-T02]` (main.cu:4) 学习目标。
`[G3-T03]` (main.cu:11) 编译命令。
`[G3-T04]` (main.cu:12) 运行命令。
`[G3-T05]` (main.cu:13) 硬件要求。

掌握 Hopper 独有的 `wgmma.mma_async` 指令，理解 128-thread warp group 概念（而非传统 32-thread warp），学会异步乘加的执行模型、FP8 E4M3/E5M2 精度选择、`setmaxnreg` 寄存器重新分配。通过对比 mma.sync（同步、warp-level）与 wgmma（异步、warp-group-level），建立对 Hopper 执行模型的深层理解。

## 硬件要求

- Compute Capability：**sm_90a 必需**（Hopper GPU，如 H100 / H200）
- CUDA Toolkit：13.x+
- 非 sm_90a 硬件将输出跳过提示并正常退出（`has_hopper_features()` 检测）

## 前置理解

- 完成 G1/G2，理解 wmma 与 mma.sync 的基础
- 理解 Hopper 的 warp group 概念：4 个 warp（128 个线程）协同操作
- 理解 FP8 格式：E4M3（8 bit，指数 4，尾数 3）和 E5M2（8 bit，指数 5，尾数 2）的范围与精度权衡

## 必做任务

`[G3-T06]` (main.cu:23) cuda::barrier 来自 libcu++。
`[G3-T07]` (main.cu:31) Hopper wgmma 设备代码：仅在 sm_90a 下编译。
`[G3-T08]` (main.cu:35) wgmma tile 尺寸（warp group 级：64x128x16，FP16 转 FP32）。
`[G3-T09]` (main.cu:40) block = 128 线程（1 个 warp group = 4 warp）。
`[G3-T10]` (main.cu:44) TODO [必做] 步骤 3：wgmma.mma_async PTX 包装（stub）。
`[G3-T11]` (main.cu:54) compile-safe stub：仅写零，不实际调用 wgmma。
`[G3-T12]` (main.cu:60) TODO [必做] 步骤 3：替换为真实 wgmma.mma_async inline PTX。
`[G3-T13]` (main.cu:73) TODO [必做] 步骤 3：setmaxnreg 演示。
`[G3-T14]` (main.cu:79) TODO [必做] 步骤 3：uncomment 后实际设置（inc）。
`[G3-T15]` (main.cu:86) TODO [必做] 步骤 3：uncomment 后实际设置（dec）。
`[G3-T16]` (main.cu:92) Kernel：wgmma 演示（blockDim = 128，1 warp group）。
`[G3-T17]` (main.cu:98) TODO [必做] 步骤 1：保留 `__cluster_dims__` 声明（为 G5 预热）。
`[G3-T19]` (main.cu:113) TODO [必做] 步骤 3：setmaxnreg 增加消费者寄存器。
`[G3-T20]` (main.cu:118) TODO [必做] 步骤 2：shared memory 用于暂存 A/B tile。
`[G3-T22]` (main.cu:129) TODO [必做] 步骤 4：mbarrier 初始化（协调 warp group 内 128 线程）。
`[G3-T27]` (main.cu:161) TODO [必做] 步骤 3：调用 wgmma.mma_async（stub）。
`[G3-T28]` (main.cu:167) TODO [必做] 步骤 3：fence.proxy.async 确保 wgmma 结果对全局可见。
`[G3-T29]` (main.cu:171) TODO [必做] 步骤 3：setmaxnreg 归还寄存器（演示）。
`[G3-T31]` (main.cu:175) TODO [必做] 步骤 3：根据 wgmma 输出布局将 acc 写回正确地址。
`[G3-T32]` (main.cu:184) TODO [必做] 步骤 5：FP8 E4M3 wgmma stub。
`[G3-T33]` (main.cu:194) TODO [必做] 步骤 5：FP8 wgmma 实现。
`[G3-T41]` (main.cu:286) TODO [必做] 步骤 5：FP8 wgmma stub 启动。
`[G3-T44]` (main.cu:303) TODO [必做] 步骤 6：Nsight Compute 性能测量。

1. `// TODO [必做]` 在 kernel 定义前加 `__cluster_dims__(1,1,1)` 声明（虽然这个练习不强制用 cluster，但为后续 G5 预热）。
2. `// TODO [必做]` 写一个 blockDim = (128, 1, 1) 的 kernel（整个 warp group），每个 warp group 负责一个更大的 tile（例如 64x64，分解为多个 wgmma 操作）。
3. `// TODO [必做]` 使用 `ptx::wgmma::mma_async` 或 inline PTX `wgmma.mma_async` 执行 FP16 转 FP32 的乘加（形如 `wgmma.mma_async.sync.aligned.m64n128k16.f32.f16`，实际 shape 取决于 warp group 协调方式）。
4. `// TODO [必做]` 加上 `cuda::barrier<cuda::thread_scope_block>` 机制，让 warp group 内所有 128 个线程协调完成。与 mbarrier 配对做 transaction count 计数。
5. `// TODO [必做]` 尝试 FP8 E4M3 输入精度（`wgmma.mma_async.sync.aligned.m64n128k32.f32.e4m3`）。设置每行或每列的 scale factor（per-tensor vs per-row scaling）。
6. `// TODO [必做]` 测量吞吐量（TFLOPS）并对比 G2 的 mma.sync 版本。记录 Nsight Compute 中的 Warp Occupancy、Tensor Core Clock Throughput。

## 进阶任务

`[G3-T18]` (main.cu:106) 每个 block 负责 tile (tile_row, tile_col) 的 WGMMA_M x WGMMA_N 输出。
`[G3-T21]` (main.cu:124) 每 thread 持有的累加器（stub：2 个 FP32 对应 wgmma m64n128k16 的 1/128 分配）。
`[G3-T23]` (main.cu:134) （stub 直接用 __syncthreads 代替）。
`[G3-T24]` (main.cu:137) K 维 tile 循环。
`[G3-T25]` (main.cu:140) 协作加载 A tile 到 smem。
`[G3-T26]` (main.cu:150) 协作加载 B tile 到 smem。
`[G3-T30]` (main.cu:174) 写回 C（stub：写零）。
`[G3-T45]` (main.cu:308) TODO [进阶] 双缓冲：warp group 0 加载 tile0，warp group 1 计算 tile-1。
`[G3-T46]` (main.cu:309) TODO [进阶] FP8 E5M2 精度对比 E4M3 精度/吞吐权衡。
`[G3-T47]` (main.cu:310) TODO [进阶] Blackwell sm_100a 上预览新 MMA 形式（MXFP8）。

- 实现一个双缓冲版本：warp group 0 加载 tile 0，warp group 1 计算 tile -1，以实现计算与数据移动的重叠
- 尝试 FP8 E5M2 精度，对比 E4M3 的精度/吞吐权衡
- 在 Blackwell sm_100a 上预览新 MMA 形式（如 MXFP8），验证指令兼容性

## 验收点

`[G3-T35]` (main.cu:219) 主程序入口。
`[G3-T36]` (main.cu:228) Hopper 特性检测：非 sm_90a 硬件跳过 kernel 启动。
`[G3-T37]` (main.cu:236) 矩阵尺寸：M=128, N=256, K=64（4 tile x 2 tile 网格）。
`[G3-T38]` (main.cu:260) `wgmma_demo_kernel` 启动。

- wgmma kernel 编译通过，sm_90a 或更高硬件上运行无错误
- 结果与 CPU 参考或 G2 的结果在数值精度范围内一致
- Nsight Compute 显示 warp group 内所有 128 个线程均参与计算（Warp Execution Efficiency 应接近 100%）
- 能指出 wgmma 的吞吐相比 mma.sync 的改进（通常因 warp group 更大而更高利用率）

## 观察点

- wgmma 是 async 指令，发出后 warp 可以立即离开，不需等待结果
- 128-thread warp group 相比 32-thread warp 对内存数据的吞吐与计算都有更强的协调能力
- FP8 的 per-row scaling 相比 per-tensor scaling 能提供更好的精度，但需要硬件支持（Hopper 支持）
- 单个 `wgmma.mma_async` 完成的计算量相比 `mma.sync` 通常大 4-8 倍

## 常见坑

- wgmma 需要完整的 128 个线程启动，任何缺失导致行为未定义（不像 wmma/mma.sync 是 warp 级的）
- FP8 scale factor 设置错误，导致数值溢出或精度丧失
- 混淆 E4M3 与 E5M2 的范围（E4M3: 约 -496 到 496；E5M2: 约 -57344 到 57344）
- 没有使用 mbarrier 或其他同步机制，导致后续 warp group 看到旧数据
- `setmaxnreg` 不当调节，导致寄存器不足或浪费
- wgmma 发出的异步任务在 kernel 结束前未完成，导致结果未写回 global memory
- 在非 sm_90a 硬件上编译或运行，导致指令不支持

## 提示

- wgmma 的 async 性质：`wgmma.mma_async` 启动计算后，该指令不会 block 当前 warp，其他指令可继续发出（需要确保依赖正确）
- 128-thread warp group = 4 个 warp（warp 0-3），线程 `threadIdx.x % 32` 对应 lane ID，`threadIdx.x / 32` 对应 warp within group ID
- FP8 scaling：per-tensor 时全矩阵共享一个 scale；per-row/per-column 时各行/列各有一个 scale
- 使用 `ptx::wgmma` 时确保包含了正确的 libcu++ 头文件（CUDA Toolkit 13.0+）
- Nsight Compute PTX 视图中搜索 `wgmma` 来验证指令生成

## 复盘问题

- wgmma 的 128-thread warp group 相比 mma.sync 的 32-thread warp，在硬件执行模型上有什么本质区别？
- FP8 E4M3 vs E5M2 分别在什么场景下更适合，以及精度权衡的考虑？
- `setmaxnreg` 的作用是什么，过多或过少分配寄存器会导致什么后果？
- async wgmma 何时真正完成计算，如何用 mbarrier 确保数据可见性？

## 对应官方参考

- CUDA C++ Programming Guide Section 3.2.6: Warp Group Matrix Multiply (wgmma)
- CUDA PTX ISA Section wgmma instructions（所有 variants）
- Hopper Tuning Guide Section Warp Group MMA
- CUDA Toolkit 13.0 Release Notes: FP8 Tensor Core support
- NVIDIA Blog Hopper GPU Architecture Deep Dive

## 输出对照（printf / std::puts 原文）

- `[G3-T39]` (main.cu:269) 原文：`启动 wgmma_demo_kernel: ...` -> 现：`launch wgmma_demo_kernel: grid=(...) block=(...)`
- `[G3-T40]` (main.cu:280) 原文：`[wgmma_demo_kernel] ... ms (stub — 结果全零，TODO [必做] 步骤 3 完成后验证)` -> 现：`[wgmma_demo_kernel] %.3f ms (stub - result is all zeros, verify after TODO [REQUIRED] step 3)`
- `[G3-T42]` (main.cu:293) 原文：`启动 wgmma_fp8_stub_kernel: ...` -> 现：`launch wgmma_fp8_stub_kernel: grid=(...) block=(128,1,1)`
- `[G3-T43]` (main.cu:298) 原文：`[wgmma_fp8_stub_kernel] done (stub — TODO [必做] 步骤 5)` -> 现：`[wgmma_fp8_stub_kernel] done (stub - TODO [REQUIRED] step 5)`
- `[G3-T48]` (main.cu:315) 原文：`[G3] 完成。用 ncu --set full ./G3_hopper_wgmma 查看 wgmma 吞吐与 warp group 占用率。` -> 现：`[G3] done. Use ncu --set full ./G3_hopper_wgmma to inspect wgmma throughput and warp-group occupancy.`
