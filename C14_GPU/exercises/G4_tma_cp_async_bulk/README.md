# 练习 G4：tma_cp_async_bulk

## 目标

`[G4-T01]` (main.cu:2) 练习 G4：tma_cp_async_bulk — Hopper TMA 异步大粒度数据搬运。
`[G4-T02]` (main.cu:4) 学习目标。
`[G4-T03]` (main.cu:11) 编译命令。
`[G4-T04]` (main.cu:12) 运行命令。
`[G4-T05]` (main.cu:13) 硬件要求。

学会使用 Tensor Memory Accelerator（TMA）进行异步大粒度数据搬运，从 global memory 到 shared memory（或反向）。通过 host 端的 `CUtensorMap` 构造与 `cuTensorMapEncodeTiled` 编码、kernel 端的 `__grid_constant__` 参数传递、`ptx::cp_async_bulk_tensor` 指令、`cuda::barrier` 同步，你会看到如何卸载 kernel 内的数据搬运工作到硬件单元。

## 硬件要求

- Compute Capability：**sm_90a 必需**（Hopper GPU，如 H100 / H200）
- CUDA Toolkit：13.x+
- 非 sm_90a 硬件将输出跳过提示并正常退出（`has_hopper_features()` 检测）

## 前置理解

- 完成 C4（Hopper cluster）与 C5（mbarrier）
- 理解 TMA 是一个独立硬件单元，不占用 SM 计算资源
- 理解 shared memory 的分布式访问（distributed smem）在 cluster 上的地址空间

## 必做任务

`[G4-T06]` (main.cu:21) 驱动 API（CUtensorMap / cuTensorMapEncodeTiled）。
`[G4-T07]` (main.cu:23) libcu++ barrier。
`[G4-T08]` (main.cu:32) 矩阵尺寸与 tile 参数。
`[G4-T09]` (main.cu:42) `__grid_constant__` tensor map（全局 scope，kernel 通过常量指针访问）。
`[G4-T10]` (main.cu:46) 编译期声明（实际值在 host 端填写后传入 kernel 参数）。
`[G4-T11]` (main.cu:50) Hopper TMA kernel（sm_90a 专属设备代码）。
`[G4-T12]` (main.cu:54) TODO [必做] 步骤 3：TMA cp_async_bulk 包装（stub）。
`[G4-T13]` (main.cu:62) compile-safe stub：直接走 element-wise 复制。
`[G4-T14]` (main.cu:71) TODO [必做] 步骤 3：替换为真实 TMA cp.async.bulk 指令。
`[G4-T15]` (main.cu:82) Kernel 1：TMA 搬运演示（无 swizzle，stub）。
`[G4-T17]` (main.cu:102) TODO [必做] 步骤 4：mbarrier 初始化。
`[G4-T19]` (main.cu:114) TODO [必做] 步骤 3：发起 TMA 搬运（leader thread，tid == 0）。
`[G4-T22]` (main.cu:126) TODO [必做] 步骤 4：consumer 等待 barrier。
`[G4-T24]` (main.cu:145) Kernel 2：TMA swizzle 128B 演示（stub）。
`[G4-T25]` (main.cu:154) TODO [必做] 步骤 5：使用 CU_TENSOR_MAP_SWIZZLE_128B 构造的 tensor map。
`[G4-T26]` (main.cu:183) Host：CUtensorMap 构造（stub 包装）。
`[G4-T27]` (main.cu:208) TODO [必做] 步骤 1：调用 cuTensorMapEncodeTiled。
`[G4-T32]` (main.cu:265) TODO [必做] 步骤 1+2：构造 CUtensorMap 并传递到 kernel。
`[G4-T40]` (main.cu:331) TODO [必做] 步骤 6：Nsight Compute 验证。

1. `// TODO [必做]` 在 host 代码中构造一个 `CUtensorMapDataType::CU_TENSOR_MAP_DATA_TYPE_FLOAT16` 的 tensor map，用 `cuTensorMapEncodeTiled` 编码一个 FP16 矩阵的布局（shape、stride、tile shape 等）。
2. `// TODO [必做]` 用 `cudaMemcpyToSymbol` 或 `cudaGetSymbolAddress` 把 tensor map 复制到 device 的 `__grid_constant__` symbol 中（kernel 端声明为 `__grid_constant__ CUtensorMap g_tensorMap`）。
3. `// TODO [必做]` 在 kernel 中用 `ptx::cp_async_bulk_tensor` 从 global memory 异步搬运一个 tile 到 shared memory。指定 source tensor map、目标 smem 地址、coordinate。
4. `// TODO [必做]` 使用 `cuda::barrier` 与 `cuda::device::barrier_arrive_tx` 追踪 TMA 搬运的字节数。consumer 线程调用 `.wait()` 等待数据到达。
5. `// TODO [必做]` 尝试 TMA swizzle mode（`CU_TENSOR_MAP_SWIZZLE_128B`），观察如何消除 shared memory bank conflict。对比有无 swizzle 的性能差异。
6. `// TODO [必做]` 在 Nsight Compute 中查看 Memory Transactions、L2 Hit Rate，验证 TMA 的搬运确实绕过了 L1 并减少了事务数。

## 进阶任务

`[G4-T16]` (main.cu:97) shared memory：容纳一个 tile。
`[G4-T18]` (main.cu:111) （stub 用 __syncthreads 代替 barrier.wait）。
`[G4-T20]` (main.cu:120) stub：直接搬运。
`[G4-T21]` (main.cu:123) stub 代替 bar.wait(...)。
`[G4-T23]` (main.cu:131) 验证：将 smem 内容写回 global memory。
`[G4-T41]` (main.cu:337) TODO [进阶] 三层 pipeline：stage0 TMA 搬运 A；stage1 TMA B + compute A；stage2 compute B + TMA 写回。
`[G4-T42]` (main.cu:338) TODO [进阶] 多个 tensor map（A/B/C 各一个），验证 grid_constant 容量。
`[G4-T43]` (main.cu:339) TODO [进阶] 测量 TMA 搬运延迟与理论带宽接近程度。

- 实现一个三层 pipeline：stage 0 TMA 搬运 tile A；stage 1 TMA 搬运 tile B、compute 用 tile A；stage 2 compute 用 tile B、TMA 写回结果
- 尝试多个 tensor map（分别用于 A、B、C），验证 grid_constant 能否容纳多个 map
- 测量 TMA 搬运的延迟（到达 smem 的时间）与理论峰值带宽的接近程度

## 验收点

`[G4-T29]` (main.cu:231) 主程序入口。
`[G4-T30]` (main.cu:240) Hopper 特性检测。
`[G4-T31]` (main.cu:248) 分配矩阵。
`[G4-T33]` (main.cu:276) Kernel 1：TMA 无 swizzle（stub）。
`[G4-T35]` (main.cu:296) 逐元素验证。
`[G4-T37]` (main.cu:305) Kernel 2：TMA swizzle 128B（stub）。

- kernel 编译通过，需要 sm_90+ 编译支持
- TMA 搬运的数据与预期的 tile 内容完全一致（可逐字节验证）
- swizzle 模式应该消除 shared memory bank conflict（Nsight 报告应显示 `shared_ld_bank_conflict` 为 0）
- barrier transaction count 与实际搬运字节数一致

## 观察点

- TMA 的搬运是硬件加速，不消耗 SM 资源，只消耗 global memory 带宽与 L2 cache
- swizzle 通过改变数据在 smem 中的布局来打破 bank conflict，对于某些 tile 大小极其有效
- `__grid_constant__` 参数只能在 kernel launch 时通过 `cudaLaunchKernelEx` 修改，不能像普通参数一样动态变化
- TMA 搬运通常比 kernel 内手动的 shared memory 加载快 2-3 倍（对于大 tile）

## 常见坑

- CUtensorMap 构造时的 shape 与 stride 参数错误，导致 TMA 读取越界或地址计算错误
- `cuTensorMapEncodeTiled` 的 tile shape 必须与实际使用的 shared memory tile 大小一致，否则数据对不齐
- `__grid_constant__` 声明必须在 global scope，不能在 kernel 内部或 namespace 内
- TMA swizzle mode 与 shared memory layout 不匹配，导致数据乱序
- barrier 的 transaction count 指定错误，导致 consumer 等待超时或过早唤醒
- 没有考虑 TMA 搬运的延迟，立即从 smem 读数据，导致读到垃圾
- TMA 搬运 16-byte 对齐之外的数据，导致硬件异常

## 提示

- CUtensorMap 初始化：先用 `memset` 清零，再调用 `cuTensorMapEncodeTiled` 填充各字段
- swizzle mode 常用值：`CU_TENSOR_MAP_SWIZZLE_NONE`（0）、`CU_TENSOR_MAP_SWIZZLE_128B`（1）、`CU_TENSOR_MAP_SWIZZLE_64B`（2）
- TMA 搬运地址必须 16-byte 对齐；shared memory 基址通常是 0，offset 需自己计算
- `ptx::cp_async_bulk_tensor` 对应 PTX 指令 `cp.async.bulk.tensor` 或 `cp.async.bulk.tensor.prefetch`
- 验证 TMA 指令：Nsight Compute PTX 视图中搜索 `cp.async.bulk.tensor`

## 复盘问题

- TMA 的搬运与传统的 kernel 内 shared memory 加载相比，优势在哪里，局限性是什么？
- swizzle 如何消除 bank conflict，原理是什么？
- `__grid_constant__` 与普通 kernel 参数的区别是什么，为什么 TMA tensor map 必须是 grid constant？
- barrier transaction count 的作用是什么，为什么需要精确指定搬运字节数？

## 对应官方参考

- CUDA C++ Programming Guide Chapter 11.8: Asynchronous Data Copies and TMA
- CUDA Driver API: `cuTensorMapEncodeTiled` documentation
- CUDA PTX ISA Section cp.async.bulk.tensor instructions
- Hopper Tuning Guide Section Tensor Memory Accelerator
- NVIDIA Blog Tensor Memory Accelerator in Hopper

## 输出对照（printf / std::puts 原文）

- `[G4-T28]` (main.cu:226) 原文：`[stub] build_tensor_map_stub: cuTensorMapEncodeTiled 未调用（TODO [必做] 步骤 1）` -> 现：`[stub] build_tensor_map_stub: cuTensorMapEncodeTiled not invoked (TODO [REQUIRED] step 1)`
- `[G4-T34]` (main.cu:283) 原文：`启动 tma_load_kernel: ...` -> 现：`launch tma_load_kernel: grid=(...) block=(...)`
- `[G4-T36]` (main.cu:300) 原文：`[tma_load_kernel] ... ms 验证: PASS/FAIL (... errors)` -> 现：`[tma_load_kernel] %.3f ms verify: PASS/FAIL (%d errors)`
- `[G4-T38]` (main.cu:314) 原文：`启动 tma_swizzle_kernel: ...` -> 现：`launch tma_swizzle_kernel: grid=(...) block=(...)`
- `[G4-T39]` (main.cu:325) 原文：`[tma_swizzle_kernel] ... ms (stub — TODO [必做] 步骤 5 完成后对比 bank conflict)` -> 现：`[tma_swizzle_kernel] %.3f ms (stub - compare bank conflicts after TODO [REQUIRED] step 5)`
- `[G4-T44]` (main.cu:346) 原文：`[G4] 完成。用 ncu --set full ./G4_tma_cp_async_bulk 查看 TMA 搬运事务数与 L2 命中率。` -> 现：`[G4] done. Use ncu --set full ./G4_tma_cp_async_bulk to inspect TMA transactions and L2 hit rate.`
