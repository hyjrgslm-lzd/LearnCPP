# 模块 H · GEMM 优化阶梯与 CUTLASS

## 模块目标

通过一步步优化 GEMM kernel，从 naive O(N³) 实现一直到 warp-specialized Hopper GEMM，建立对 GEMM 优化阶梯的完整理解。同时深入学习 CUTLASS 3.x 的五层 hierarchy（Device / Kernel / Collective / Tiled MMA+Copy / Atom）、CollectiveBuilder 自动调度、cuTe 的 Layout/Tensor/Copy_Atom/MMA_Atom 四大概念，最终能用 CUTLASS 或 cuTe 手写对标库级性能的 GEMM kernel。

## 前置知识

- 完成模块 G（wmma / wgmma / TMA / warp specialization）
- 理解基础 GEMM 算法与分块策略
- 理解 Nsight Compute 的 roofline 分析、occupancy、内存带宽指标
- 理解 shared memory bank layout 与 conflict 避免

## 模块完成标准

- 能解释 GEMM naive → tiled → shared memory tile → warp tile → Tensor Core → pipelined 各阶段的性能特征与优化点
- 理解 CUTLASS 3.x 的五层 hierarchy 及各层的职责（Device 层封装整个 kernel 参数，Kernel 层定义 grid mapping，Collective 层定义数据流动与同步，Tiled MMA/Copy 层定义小块计算与搬运，Atom 层是最小操作单元）
- 掌握 CollectiveBuilder 的自动参数生成与 KernelSchedule（例 KernelTmaWarpSpecializedCooperative）的选择
- 理解 cuTe Layout 的 shape 与 stride 如何描述内存映射，以及如何用 Copy_Atom 与 MMA_Atom 实现数据搬运与计算
- 能手写一个 mini-GEMM kernel（不借助 Collective 层），用 cuTe 清晰表达索引与数据流
- 能用 CUTLASS 3.x Device API 搭建一个 Hopper warp-specialized GEMM，并用 Nsight Compute 验证性能

## 硬件与工具链要求

- Compute Capability：sm_80+ 基础（shared memory tile）；sm_90a 推荐（warp specialization / TMA）
- Blackwell sm_100a / sm_120a 用作进阶对比
- CUDA Toolkit：13.x+
- CUTLASS：3.x（从 GitHub main 或某个 3.x tag 通过 FetchContent）
- Nsight Compute：最新版，特别是 roofline 分析与 source-SASS 关联

---

## 练习 H1：gemm_naive_and_tiled

### 目标

从最简单的 naive GEMM（每个 thread 计算 C 的一个元素）开始，逐步优化到每个 block tile 32×32 的共享内存版本。通过对比两个版本的吞吐与 Nsight Compute roofline，深入理解为什么 naive O(N³) 实现即使在 GPU 上也会被内存瓶颈完全主导。

### 前置理解

- 理解 GEMM 的基本算法：C = A × B（M×K × K×N → M×N）
- 理解 "每个 thread 计算 C 的一个元素" 意味着每个 thread 需要从 global memory 加载 K 个 A 元素与 K 个 B 元素
- 理解 shared memory 作为 L1.5 cache 的角色：block 内所有 thread 共享，延迟较低

### 必做任务

1. // TODO [必做] 实现 naive GEMM：gridDim = (N/BLOCK_SIZE, M/BLOCK_SIZE), blockDim = (BLOCK_SIZE, BLOCK_SIZE)。每个 thread (tx, ty) 计算 C[blockIdx.y * blockDim.y + ty][blockIdx.x * blockDim.x + tx]。加载循环：for k in 0..K，读 A[...][k] 和 B[k][...]，累加到 C_local。

2. // TODO [必做] 用 `cudaEvent` 计时，记录 M=N=K=1024 或 2048 时的执行时间，计算 TFLOPS（2*M*N*K / time）。与理论峰值比较。

3. // TODO [必做] 实现 shared memory tile 版本：block 处理 TILE_SIZE×TILE_SIZE 的 C tile（例如 32×32），循环 K/TILE_K_SIZE 次。每次加载 TILE_SIZE×TILE_K_SIZE 的 A tile 和 TILE_K_SIZE×TILE_SIZE 的 B tile 到 smem，block 内所有 thread 计算该 tile 对 C tile 的贡献。加上 `__syncthreads()` 确保数据就位与计算完成。

4. // TODO [必做] 相同的矩阵大小，测量共享内存版本的吞吐。应该相比 naive 快 5-10 倍。

5. // TODO [必做] 用 CudaEventTimer 编写一个性能测试，对比两个版本的 TFLOPS、内存带宽利用率。

6. // TODO [必做] 用 Nsight Compute `--set full` 对两个版本抓 profile，记录 "Memory Bound" vs "Compute Bound" 判定、"Achieved Occupancy"、"L1/L2 Cache Hit Rate"，绘制或记录 roofline 图。

### 进阶任务

- 在共享内存版本基础上，用 bank conflict awareness 优化 smem layout（例如 padding）
- 尝试不同的 BLOCK_SIZE（128, 256）与 TILE_K_SIZE（8, 16, 32），找到性能最优点
- 手动展开最内层循环（K 维），减少循环开销

### 验收点

- naive 和 shared memory 两个版本编译通过，结果逐元素一致（FLT_EPSILON 容差）
- naive 吞吐远低于峰值（typically < 1% peak），shared memory 版本达到 roofline 上 memory-bound 线（通常 10-30% peak）
- Nsight Compute 报告 naive 为"严重内存访问未合并"，shared memory 版本 "coalescing" 提升明显
- 能在 roofline 图上指出两个版本的算术强度（FLOPs/Byte）与理论位置

### 观察点

- naive GEMM 的每个 thread 加载 2K 个字节（FP32：8K 字节），但仅执行 2K 个 FLOP，算术强度只有 0.25 FLOPs/Byte，远低于 GPU 的 roofline
- shared memory tile 通过让多个 thread 复用 smem 中的数据，大幅提升算术强度到 2-4 FLOPs/Byte（取决于 tile 大小）
- shared memory 版本的瓶颈可能转移到 shared memory 吞吐或 bank conflict，而非 global memory
- occupancy 不是决定性因素，naive 版本 occupancy 可能还不错，但吞吐仍极低

### 常见坑

- naive 版本中，多个 thread 访问同一行的连续元素，如果 thread 排列不当会导致 coalescing 失败
- shared memory 版本中，多个 thread 同时写 smem 的相邻地址可能产生 bank conflict（32 个 thread 访问 32 个 bank）
- smem 大小限制：96 KB per block，如果 TILE_SIZE×TILE_K_SIZE 的两个 tile 超过 48 KB，occupancy 会下降
- `__syncthreads()` 的开销不能忽视，如果 tile 过小会反复同步，开销相对增加
- 循环 unroll 过度会导致寄存器压力与 occupancy 下降
- 在 profiling 前没有做 warmup，第一次 kernel 执行受初始化影响

### 提示

- 计算 TFLOPS：`2 * M * N * K / (time_ms * 1e-3) / 1e12`
- 理论峰值查询：Hopper sm_90a FP32 = 1.5 TFLOP/s（单精，单 GPU）
- 检查 coalescing：相邻 thread 应访问相邻内存地址，且对齐到 128 byte（4 个 warp 的事务边界）
- 检查 bank conflict：shared memory 32 个 bank，跨度 4 byte；计算 address % 128 看是否多个 access 映射同一 bank
- warmup：kernel 执行至少 3 次，舍弃第一次数据，取后续的平均或最好值

### 复盘问题

- naive GEMM 为什么会被内存瓶颈支配，算术强度是多少？
- shared memory tile 如何提升算术强度，trade-off 是什么？
- 如果增加 TILE_SIZE 从 32 到 64，对 occupancy、寄存器压力、共享内存占用各有什么影响？
- roofline 图上，naive 与 optimized 版本分别对应什么位置？

### 对应官方参考

- CUDA C++ Best Practices Guide Chapter "Maximize Throughput" → "Maximize Memory Throughput"
- Nsight Compute documentation "Roofline Analysis"
- NVIDIA Blog "Optimizing CUDA Applications" series
- cuda-samples `simpleMultiGPU` / `matrixMul*` examples

---

## 练习 H2：gemm_warp_tile_and_mma

### 目标

在 H1 的 shared memory tile 基础上，进一步分解：每个 warp 负责一个 16×16 的 warp tile（C 的子 tile）。使用 `mma.sync m16n8k16` 指令（或 wmma），通过 `ldmatrix` 从 shared memory 加载矩阵片段到寄存器，然后用 Tensor Core 计算。这一步引入了 Tensor Core，是从"普通 GEMM 优化"向"Tensor Core 特化"的转折。

### 前置理解

- 完成 H1，理解 shared memory tile 版本的结构
- 完成模块 G1/G2，理解 wmma / mma.sync 的基础用法
- 理解寄存器的使用（fragment 就存在寄存器中）

### 必做任务

1. // TODO [必做] 改写 H1 的 shared memory 版本：blockDim 保持（例如 256），但现在 block 负责 32×32 的 C tile，分解为 2×2 的 warp tile（每个 16×16）。

2. // TODO [必做] 每个 warp 的逻辑：加载 16×16 的 A 片段与 16×16 的 B 片段到寄存器（通过 ldmatrix），执行 `mma.sync m16n8k16` 两次（第一次 16×8×16，第二次另外 16×8×16，合成 16×16×16）。

3. // TODO [必做] A/B 片段的 layout：A 是行主，B 是列主（或根据 ldmatrix 的要求调整）。确保 ldmatrix 从 smem 正确加载。

4. // TODO [必做] 结果 fragment 累加到 C（也是寄存器中的 fragment），最后 store 回 global memory。

5. // TODO [必做] 测量吞吐（TFLOPS），应明显高于 H1 的 pure ALU 版本（通常快 3-5 倍）。

6. // TODO [必做] 用 Nsight Compute 查看 Tensor Core 利用率指标（"Tensor Core Utilization"、"Tensor Pipe Efficiency"），应接近 100%（单 warp 不会 100%，但 block 内多个 warp 协同应很高）。

### 进阶任务

- 尝试 m16n8k32（一次加载两个 K 维的 tile），观察吞吐提升
- 尝试改变 warp tile 大小（例如 16×32 或 32×16），测量吞吐变化
- 对比手写 mma.sync PTX 与 wmma wrapper 的生成代码差异

### 验收点

- kernel 编译通过，结果与 H1 逐元素一致
- TFLOPS 相比 H1 提升 3-5 倍
- Tensor Core Utilization > 80%
- ldmatrix 指令在 PTX 视图中可见，且与 mma.sync 正确配对

### 观察点

- warp tile 分解使每个 warp 独立承载一部分计算，提升了并行性与寄存器利用率
- Tensor Core 的吞吐相比标量 ALU 快数倍，是现代 GPU 高性能的关键
- fragment 住在寄存器中，访问速度是 global/shared memory 的 1000 倍，减少了访存延迟

### 常见坑

- ldmatrix 与 mma.sync 的 shape 不对应，导致某些计算被忽略或数据错误
- 混淆 A/B 的 layout（行主 vs 列主），导致结果错误
- warp tile 尺寸与硬件 mma shape 不整除，某些 warp 处理不完整的 tile
- 寄存器压力：3 个 fragment（A、B、C）可能用掉 60-100 个寄存器/thread，导致 occupancy 下降
- 在不同硬件上运行（sm_70 vs sm_90），mma shape 支持不同，代码需要条件编译

### 提示

- 每个 fragment 的大小（寄存器数）取决于 M/N/K 与数据类型，ldmatrix 输出固定 128 byte（16×8 元素）
- 如果 warp tile 是 16×16 而单个 mma.sync 是 m16n8k16，需要两次 mma_sync（K 维各 16）或调整 shape
- 寄存器计算：N warps × M threads/warp × R registers/thread，需 < 256 KB/SM（通常 256 KB shared）
- ldmatrix 的指针对齐要求同 G1

### 复盘问题

- Tensor Core 相比标量 ALU，为什么能提升 3-5 倍吞吐？
- warp tile 16×16 与 mma.sync m16n8k16 的关系是什么，为什么需要两次 mma_sync？
- 如果 warp tile 改为 16×32，对 mma.sync 调用次数与寄存器占用各有什么影响？
- 寄存器压力如何影响 occupancy 与最终性能？

### 对应官方参考

- CUDA C++ Programming Guide Section B.30 "Warp Matrix Functions"
- CUDA Programming Guide Section 11: "Performance Tuning" → "Tensor Core"
- CUTLASS examples `gemm_*` with `mma_sync`

---

## 练习 H3：gemm_pipelined_double_buffer

### 目标

在 H2 的基础上，引入 producer-consumer pipeline：两个 stage 的 shared memory buffer（stage 0 与 stage 1）。一个 producer 线程 (或 warp) 负责提前加载下一个 tile 到 buffer，consumer warp 则对当前 buffer 执行 wgmma 或 mma.sync，实现计算与数据加载的重叠。这一步展示了如何消除 global memory 加载的延迟。

### 前置理解

- 完成 H2，理解 warp tile + Tensor Core 的结构
- 理解 double-buffering 的概念与同步机制
- 理解 `cuda::memcpy_async` 或 `cp.async`（sm_80+）

### 必做任务

1. // TODO [必做] 设计两个 shared memory buffer，每个能容纳一个 block tile（例如 32×32 FP32）。

2. // TODO [必做] 初始化阶段：加载第 0 个 tile 到 buffer 0。

3. // TODO [必做] pipeline 循环：
   - producer 用 `cuda::memcpy_async` 或 `__pipeline_*` 把下一个 tile 加载到 buffer 1（与 buffer 0 不冲突）
   - consumer warp 对 buffer 0 执行计算（通过 H2 的 warp tile + mma.sync）
   - `__pipeline_wait_prior(0)` / `__syncthreads()` 等待 producer 完成
   - swap buffer 指针

4. // TODO [必做] 对 stage 数 = 2 与 stage 数 = 3 分别实现，对比性能。

5. // TODO [必做] 测量吞吐（TFLOPS），应相比 H2 提升 20-40%（取决于 global memory latency 与计算强度平衡）。

6. // TODO [必做] 用 Nsight Compute 观察 "Smem/Dmem Pipeline" stall 或 "L1/L2 Hit Rate"，验证 pipeline 确实减少了内存等待。

### 进阶任务

- 实现三层 pipeline，进一步重叠
- 尝试用 NVIDIA pipeline library (`<cuda/pipeline>`) 而非手写 `__pipeline_*`
- 对比 `cp.async` vs `memcpy_async` 的性能差异

### 验收点

- kernel 编译通过，结果与 H2 逐元素一致
- TFLOPS 相比 H2 提升 15-40%
- Nsight Compute 显示 memory pipeline stall 相比 H2 显著下降
- stage 数 = 3 的版本比 stage 数 = 2 略优或性能相近

### 观察点

- double-buffering 让全局内存延迟被隐藏（latency hiding），因为计算与加载可以并行
- stage 数过多会导致 shared memory 占用增加，occupancy 下降，收益递减
- 在 memory-bound kernel 中，pipeline 的收益最大；在 compute-bound 中无效

### 常见坑

- buffer swap 错误，导致多个 stage 写同一 buffer
- `__pipeline_wait_prior()` 或 `__syncthreads()` 位置错误，导致数据竞争
- 没有处理最后 stage 的 flush（最后一个 tile 加载完但无下一 tile）
- `memcpy_async` 或 `cp.async` 的地址对齐不满足，导致异常

### 提示

- `cuda::memcpy_async(dst, src, size, stream)` 需要 CUDA 11.0+ 与 sm_80+
- `__pipeline_*` 是更底层的 API，需要 CUDA 12.0+ 与特定硬件支持
- double-buffering 的伪代码：`load(buf[0]); for i in 1..N: { load_async(buf[i % 2]); compute(buf[(i-1) % 2]); wait(); }`
- 在 profiling 前确保矩阵大小足够大（M=N=K >= 1024），小矩阵的 pipeline 效果不明显

### 复盘问题

- double-buffering 如何隐藏全局内存延迟？
- stage 数 = 2 vs 3，为什么后者不一定总是更快？
- 如果计算时间（H2 warp tile 执行时间）短于加载时间，pipeline 还有效吗？

### 对应官方参考

- CUDA C++ Programming Guide Section 3.2.5: "Asynchronous Warp-Level Primitives"
- CUDA C++ libcxx documentation: `<cuda/pipeline>` and `<cuda/barrier>`
- CUTLASS 3.x examples `hopper_gemm_*` with pipelining

---

## 练习 H4：gemm_hopper_warp_specialized

### 目标

集成模块 G 的所有技术（wgmma、TMA、mbarrier、warp specialization）到一个完整 GEMM kernel：producer warp 用 TMA 搬运 tiles，consumer warp 用 wgmma 计算。在 sm_90a 上达到接近库级性能。同时引入 cluster launch（`__cluster_dims__`），让多个 block 共享 L2 减少全局内存压力。

### 前置理解

- 完成模块 G1-G5（wmma、wgmma、TMA、warp specialization）
- 完成 H1-H3
- 理解 Hopper cluster 的分布式 shared memory

### 必做任务

1. // TODO [必做] 添加 `__cluster_dims__(2,1,1)` 声明（2×1×1 cluster 表示 2 个 block）。

2. // TODO [必做] kernel 参数包含：TMA tensor map（通过 `__grid_constant__`）、输入/输出指针、M/N/K。

3. // TODO [必做] blockDim = (256, 1, 1)：线程 0-31 为 producer，线程 32-255 为 7 个 consumer warp。

4. // TODO [必做] producer 逻辑（G5 回顾）：TMA 搬运 A tile 与 B tile 到 shared memory buffer，用 mbarrier 同步。

5. // TODO [必做] consumer 逻辑：等待 mbarrier，执行 wgmma 乘以 shared memory 中的数据，loop over blocks。

6. // TODO [必做] 用 cudaLaunchKernelEx 指定 cluster size，验证 Nsight Compute 显示 cluster 正确启动。

7. // TODO [必做] 测量 TFLOPS，对比 H3 与理论峰值。应达到 Hopper peak 的 70-85%（对于 FP32）。

### 进阶任务

- 集成 split-K（多个 block 分别计算 K 的不同切片，再 reduce），支持更大矩阵
- 尝试不同的 tile size（32×32, 64×64, 128×128），找到最优点
- 测试 Blackwell sm_100a，对比新的 MMA 指令形式

### 验收点

- kernel 编译通过，sm_90a 硬件上运行无错误
- GEMM 结果与 CPU 参考逐元素一致
- TFLOPS 达到 Hopper peak 的 70-85%（查询 `cudaDeviceGetAttribute` CU_DEVICE_ATTRIBUTE_MAX_THREADS_PER_MULTIPROCESSOR）
- Nsight Compute 显示 cluster 被正确启动（在"block distribution"或相关指标）
- warp specialization 的时间线在 Nsight Systems 中可见（producer 与 consumer 错开）

### 观察点

- TMA 的硬件加速使全局内存搬运从 SM 中卸载，SM 完全专注于计算
- warp specialization 让 producer 和 consumer 独立前进，最大化重叠
- cluster launch 让多个 block 共享 L2，减少 global memory 访问
- 组合这些技术后，GEMM 的吞吐可以逼近 GPU 的理论峰值

### 常见坑

- cluster 大小与 SM 数不匹配（某些 GPU 上 cluster 大小有限制）
- wgmma 需要完整的 128 个线程，blockDim 设置错误导致某些 warp 缺失
- TMA swizzle 与 shared memory layout 不匹配，数据乱序
- 没有处理矩阵大小不是 tile size 倍数的情况
- split-K reduction 时，不同 block 的结果未正确合并

### 提示

- Hopper peak FP32：查询 `cudaDeviceGetAttribute` 与手册
- cluster launch 语法：`cudaLaunchConfig_t config{}; config.gridDim = {grid_x, grid_y}; config.blockDim = {block_x, block_y}; cudaLaunchKernelEx(&config, kernel, ...)`
- 验证 TMA：Nsight Compute 的 "Memory Transactions" 中应看到 TMA 发出的批量读
- 调试 warp specialization：用 shared memory 计数器记录 producer/consumer 进度

### 复盘问题

- Hopper warp-specialized GEMM 相比 H3 pipeline 版本快多少倍，为什么？
- TMA 的卸载如何减少 SM 压力，整体吞吐如何改善？
- cluster 与 split-K 在什么场景下各自最优？

### 对应官方参考

- Hopper Tuning Guide Section "Warp Specialization"
- Hopper Tuning Guide Section "Tensor Memory Accelerator"
- CUTLASS 3.x examples `hopper_warp_specialized_gemm`
- NVIDIA Blog "Hopper GPU Performance" series

---

## 练习 H5：cutlass_collective_builder

### 目标

学会用 CUTLASS 3.x 的 Device API 与 CollectiveBuilder 自动生成高性能 GEMM kernel。理解 CUTLASS 的五层 hierarchy（Device / Kernel / Collective / Tiled MMA+Copy / Atom），以及如何通过改变高层参数自动生成不同的低层实现（例如 sm_80 用 mma.sync，sm_90a 用 wgmma）。

### 前置理解

- 完成 H1-H4，理解各个优化阶段
- 理解 CUTLASS 的基础概念（虽然可能没用过）
- 理解 template 参数与 type traits

### 必做任务

1. // TODO [必做] 在 main.cu 中 include CUTLASS 头文件（`cutlass/gemm/device/gemm_universal_adapter.hpp` 等），准备 CollectiveBuilder 环境。

2. // TODO [必做] 定义 GEMM 参数：M=1024, N=1024, K=1024（或更大），FP32 precision，sm_90a target。

3. // TODO [必做] 用 CollectiveBuilder 自动生成 Collective 类型：
   ```cpp
   using Collective = typename cutlass::gemm::collective::CollectiveBuilder<
     cutlass::arch::Sm90,
     cutlass::layout::RowMajor,
     cutlass::half_t,
     128,  // ct tile size A
     cutlass::half_t,
     128,  // ct tile size B
     float,
     typename cutlass::layout::RowMajor,
     ...other params...
   >::CollectiveOp;
   ```

4. // TODO [必做] 选择 KernelSchedule：`KernelTmaWarpSpecializedCooperative` 或 `KernelTmaWarpSpecializedPingpong`，观察两者的区别。

5. // TODO [必做] 用 `cutlass::gemm::device::GemmUniversalAdapter` 封装，调用 `initialize()` 与 `run()`。

6. // TODO [必做] 测量吞吐，对比手写的 H4。应相近（或 CUTLASS 因自动优化略快）。

### 进阶任务

- 尝试不同的 CollectiveBuilder 配置（不同 tile size、不同 schedule），对比性能
- 在 sm_80 与 sm_90a 上各运行一遍，观察自动生成的代码差异（PTX 视图）
- 尝试 split-K 模式，用 CUTLASS 的内置 support

### 验收点

- kernel 编译通过（CUTLASS 3.x headers only，无外部依赖）
- GEMM 结果与 H4 逐元素一致
- 吞吐与手写 H4 相近（±10%）
- 能在 CUTLASS 生成的 PTX 中识别 wgmma / TMA / mbarrier 指令

### 观察点

- CUTLASS 通过 template 参数自动生成完整的 kernel，避免了手写的复杂性
- 五层 hierarchy 让参数化与自动化成为可能：改变高层参数，低层实现自动适配
- 不同的 KernelSchedule 代表了设计者对 compute-memory overlap 的不同权衡

### 常见坑

- CollectiveBuilder 的参数组合并非所有都合法（某些 tile size / precision 组合不支持）
- 模板实例化失败时，错误信息极其冗长，需要耐心逐行查看
- 没有处理 M/N/K 不是 tile size 倍数的情况（CUTLASS 可能有 epilogue 但需配置）
- schedule 选择错误或不适用于硬件

### 提示

- CUTLASS 3.x FetchContent：确保 .gitignore 中包含 `third_party/`
- 编译命令参考：`cmake .. -DCMAKE_CUDA_ARCHITECTURES="90a" -DCUTLASS_ENABLE_TESTS=OFF`
- 调试编译错误：用 `--verbose` 查看完整编译命令，或 `--keep-dir` 保留 intermediate 文件
- 参考 CUTLASS 示例 `examples/48_hopper_warp_specialized_gemm/` 的结构

### 复盘问题

- CUTLASS 五层 hierarchy 各层分别负责什么，为什么这样分层有利？
- CollectiveBuilder 如何从高层参数自动生成低层实现？
- `KernelTmaWarpSpecializedCooperative` vs `KernelTmaWarpSpecializedPingpong` 的区别是什么？

### 对应官方参考

- CUTLASS GitHub: `media/docs/cpp/gemm_api_3x.md`
- CUTLASS examples: `examples/48_hopper_warp_specialized_gemm/`
- CUTLASS `include/cutlass/gemm/collective/` 源码
- CUTLASS paper / presentation on collective primitives

---

## 练习 H6：cute_layout_and_tensor

### 目标

学会 cuTe 的核心四大概念（Layout、Tensor、Copy_Atom、MMA_Atom），用它们手写一个 mini-GEMM kernel（不借助 Collective 层），体验如何用类型级的 layout 抽象避免复杂的手工索引。对比同样功能的手写 indexing，感受 cuTe 的清晰性与正确性。

### 前置理解

- 完成 H1-H5
- 理解 shape、stride、layout 作为数据结构的概念
- 对模板元编程有基础认识

### 必做任务

1. // TODO [必做] 学习 cuTe `Layout`：`make_layout(shape, stride)`，例如 `make_layout(Shape<16, 32>{}, Stride<32, 1>{})` 表示 16×32 矩阵，行主（stride 为 32）。

2. // TODO [必做] 学习 cuTe `Tensor`：`make_tensor(ptr, layout)`，把指针与 layout 组合成一个抽象张量对象。通过 `tensor(i, j)` 即可访问。

3. // TODO [必做] 写一个 mini-GEMM kernel：
   - 用 `make_tensor` 包装 A/B/C 的 global memory 指针
   - 在 shared memory 中创建 A/B tile 的 Tensor（layout 为 swizzle 以消除 bank conflict）
   - 用 loop 加载数据，计算，存储

4. // TODO [必做] 学习 cuTe `Copy_Atom`：例如 `SM90_TMA_LOAD<Copy_Atom_t>{}`，代表一个数据搬运最小单元。用它改写数据加载部分，使代码更声明式。

5. // TODO [必做] 学习 cuTe `MMA_Atom`：例如 `SM90_64x128x16_F16F16F32_SS{}`，代表一个 64×128×16 的 FP16→FP32 乘加单元。用它改写计算部分。

6. // TODO [必做] 对比：用 cuTe 写的版本 vs 手工索引版本的代码行数、可读性、正确性。

### 进阶任务

- 用 cuTe 实现一个 split-K GEMM
- 尝试动态 shape（虽然 Layout 通常是编译期确定的）
- 对比 cuTe 写的版本与 CUTLASS Collective 自动生成版本的 PTX/SASS 代码

### 验收点

- mini-GEMM kernel 编译通过（需 CUTLASS header 与 CUDA 13.x）
- 结果与参考实现逐元素一致
- 代码行数相比手工索引版少 30-50%（主要是索引计算部分）
- Tensor 对象的访问方式（`tensor(i,j)` 或 `tensor(make_coord(i,j))`）正确映射到全局内存地址

### 观察点

- cuTe Layout 把复杂的多维索引计算变成了类型级的描述，编译器可以完全消除抽象开销
- Copy_Atom 与 MMA_Atom 是可组合的原语，通过参数化可生成各种计算与搬运模式
- cuTe 的设计哲学是"零开销抽象"，生成的代码与手工索引等效

### 常见坑

- Layout 的 shape 与 stride 维度数必须相等（两个都是 rank-2 或 rank-3）
- swizzle layout 的参数需要与硬件 bank 布局匹配，参数错误导致 bank conflict 反而增加
- Copy_Atom / MMA_Atom 的类型参数必须与实际 tensor 的元素类型匹配
- Tensor 的生命周期与指针有关，如果指针失效，Tensor 操作导致 UB

### 提示

- cuTe 的 Coord 通常用 `make_coord(i, j, ...)` 创建，或直接用 tuple `{i, j}`
- swizzle 参数查看 `cutlass/cutlass.h` 或官方示例
- 调试 Layout：打印 `print_layout(layout)` 观察生成的布局
- 编译时 shape/stride 检查：用 `static_assert` 验证 rank

### 复盘问题

- cuTe Layout 相比手工索引计算的优势是什么？
- Copy_Atom 与 MMA_Atom 如何组合成完整的 kernel？
- swizzle 对 Layout 的影响是什么？

### 对应官方参考

- CUTLASS GitHub: `media/docs/cute/` 全系列文档
- CUTLASS `media/docs/cute/00_quickstart.md` → `0x_gemm_tutorial.md`
- CUTLASS `examples/cute_*` 系列示例
- NVIDIA Blog "CUTLASS 3.x and CuTe Deep Dive"

---

## 做完本模块后应达到的水平

**知识检查清单**

- 能清楚解释 GEMM naive → tiled → shared memory → warp tile → Tensor Core → pipelined → warp-specialized 各阶段的性能特征与优化点
- 理解 CUTLASS 3.x 的五层 hierarchy 及各层的职责与参数化方式
- 理解 CollectiveBuilder 如何从高层参数自动生成低层 kernel 实现
- 掌握 cuTe 的 Layout、Tensor、Copy_Atom、MMA_Atom 四大概念
- 能用 roofline 分析定位 GEMM 的瓶颈（compute-bound vs memory-bound）

**能做到**

- 实现一个 naive GEMM，用 roofline 分析证明其被内存完全主导
- 实现共享内存优化、warp tile、Tensor Core、pipelined 等多个版本，逐步提升性能
- 用 CUTLASS CollectiveBuilder 搭建一个 Hopper warp-specialized GEMM，达到 peak 的 70-85%
- 用 cuTe 手写一个 mini-GEMM，理解其中的 Layout 与 Atom 的含义
- 在 Nsight Compute 与 Nsight Systems 中定位性能瓶颈，并提出优化方向

**见模块 I** 关于如何基于这些 GEMM 技术实现 softmax、layernorm、fused attention 等 AI 算子。

