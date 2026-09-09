# 模块 G · TensorCore、wgmma、TMA

## 模块目标

掌握 GPU Tensor Core 三代编程接口的演进（wmma → mma.sync → wgmma）、理解 Hopper 的异步数据移动加速器（TMA）如何打破 kernel 内数据移动的瓶颈、学会 producer-consumer warp specialization 模式如何在 Hopper 上实现高效的 pipeline。你会从最基础的 wmma API 一路深入到 Hopper 的 warp-specialized GEMM，建立对 Tensor Core 执行模型与硬件异步机制的直观理解。

## 前置知识

- 完成模块 C（线程层级、cooperative groups、cluster）
- 完成模块 D（warp 原语、occupancy）
- 完成模块 E（stream、CUDA Graph）
- 理解 block/warp/lane 映射关系
- 理解 shared memory、global memory 的读写语义
- 理解 warp 的"前向进度保证"与同步原语

## 模块完成标准

- 能清楚说出 wmma / mma.sync / wgmma 三代 API 的硬件对应关系与指令形式
- 理解 `ldmatrix` 为什么必须与 `mma.sync` 配对，以及 global-to-register 的搬运路径
- 掌握 Hopper wgmma 的异步执行模型，包括 128-thread warp group 概念
- 理解 CUtensorMap 的主机端构造、`__grid_constant__` 参数传递、`ptx::cp_async_bulk_tensor` 搬运
- 理解 mbarrier 与 TMA 的交互（transaction count、arrive_tx、wait）
- 能实现并测量一个 warp-specialized GEMM kernel 的性能，并用 Nsight Compute 定位 Tensor Core 利用率

## 硬件与工具链要求

- Compute Capability：sm_70+ 基础（wmma）；sm_75+ 建议（mma.sync）；sm_90a 必需（wgmma / TMA）
- Blackwell sm_100a / sm_120a 用作进阶对比（MXFP8、新 MMA 形式）
- CUDA Toolkit：13.x+
- Nsight Compute：最新版（Tensor Core 利用率、warp execution patterns）
- 工具：`--lineinfo` 生成行号，PTX 查看 wgmma / mma.sync 指令

---

## 练习 G1：wmma_fp16_basic

### 目标

学会用 Volta+ 的 `nvcuda::wmma` API 构造和执行最简单的 Tensor Core 操作：`fragment<matrix_a/b, M,N,K>` 的构造、`load_matrix_sync` 加载、`mma_sync` 乘加、`store_matrix_sync` 存储。通过 M16N16K16 的 FP16→FP32 tile，建立对 Tensor Core 基础流水的理解。

### 前置理解

- 理解 GEMM 的分块策略：把 C 分成多个 16×16 的小 tile
- 理解 warp 作为 32-thread 执行单位，`nvcuda::wmma` 是 warp-level 的操作
- 理解 fragment 作为"寄存器上的数据视图"，不是真实内存对象

### 必做任务

1. // TODO [必做] 声明三个 fragment：`fragment<matrix_a, 16, 16, 16, half>` 类型的 fA，`fragment<matrix_b, 16, 16, 16, half>` 类型的 fB，`fragment<accumulator, 16, 16, 16, float>` 类型的 fC。初始化 fC 为全 0。

2. // TODO [必做] 在 host 端构造两个 16×16 的 half 矩阵（行主），复制到 device global memory。

3. // TODO [必做] 写一个 kernel，blockDim = (32, 1, 1)（单个 warp），每个 warp 使用 `load_matrix_sync` 从 global memory 加载 fA 和 fB（注意 ldmatrix 需要 16-byte 对齐的指针）。

4. // TODO [必做] 调用 `nvcuda::wmma::mma_sync(fC, fA, fB, fC)` 执行单次 16×16×16 乘加。

5. // TODO [必做] 用 `store_matrix_sync` 把结果 fC 写回 global memory，再在 host 端用 CPU BLAS（例如 Eigen 或简单循环）验证正确性。

6. // TODO [必做] 用 Nsight Compute 测量吞吐量（TFLOPS），对比理论 peak TFLOPS。记录 "SM Occupancy"、"Warp Execution Efficiency"、"Tensor Core Utilization"。

### 进阶任务

- 尝试 M16N16K32 的 FP16→FP32，观察 ldmatrix 加载次数的变化
- 对比 sm_70 Volta 与 sm_90a Hopper 上的 SASS 指令差异（用 Nsight Compute PTX 视图）
- 尝试用 half 作为累加器（`fragment<accumulator, 16, 16, 16, half>`），观察精度变化与吞吐差异

### 验收点

- wmma kernel 编译通过，无 warnings
- 单个 warp 执行的 GEMM 结果与 CPU 参考实现逐元素一致（可用 ULP tolerance）
- Nsight Compute 显示 "Tensor Core Utilization" >= 80%（单 warp 情况下通常 100%）
- 能指出 fragment load/store 中 ldmatrix 对齐要求的硬件原因

### 观察点

- wmma 操作在 PTX 中对应 `mma.sync` 与 `ldmatrix` 的组合
- 单个 warp 执行 wmma 可达 Hopper 上的理论 peak（考虑 warp 数量 32）
- fragment 是一种"虚拟"数据结构，编译器把它映射到物理寄存器上
- 16×16×16 FP16→FP32 对应 2×16×16×32 = 16384 FLOPs，在 2 个 4-clock cycle 内完成（理想情况）

### 常见坑

- fragment 声明时模板参数顺序错误（M/N/K 的顺序），导致编译失败或错误结果
- `load_matrix_sync` 的指针未按 16-byte 对齐，导致地址计算错误或 ldmatrix 异常
- 混淆 fragment 的 layout（row-major vs column-major），导致结果错误
- 在非 warp 对齐的线程上调用 wmma（例如 threadIdx.x >= 32），导致行为未定义
- 使用了 `__syncthreads` 在 wmma 调用前后，导致不必要的性能下降
- fragment 的 M×N×K 尺寸与硬件支持的尺寸不匹配（Volta 支持 16×16×16/16，Ada 更灵活）
- 从 global memory 加载时没有考虑 warp 内线程的分工方式，某些线程看不到数据
- 累加器 fragment 的元素数量计算错误，导致存储越界

### 提示

- fragment 初始化：`fill_fragment(fC, 0.0f)` 或通过构造函数
- 16-byte 对齐检查：`ptr % 16 == 0`，或用 `__align__(16)` 修饰符
- 单个 16×16 FP16 矩阵占 512 字节，必须连续存储在 global memory 中
- ldmatrix 在 PTX 中是 `ldmatrix.sync.aligned.m8n8` 指令，自动拆分为多个加载
- Nsight Compute 的 "Tensor Pipe Efficiency" 比 "Warp Execution Efficiency" 更反映 Tensor Core 真实利用率

### 复盘问题

- wmma 中 M、N、K 各自代表什么，以及它们如何映射到硬件 Tensor Core？
- 为什么 ldmatrix 需要 16-byte 对齐，而普通 global memory 访问只需 4-byte 对齐？
- 单个 warp 执行 mma_sync 需要多少个 clock cycle，以及如何通过 Nsight 验证？
- fragment 是寄存器还是共享内存，编译器如何决定其实际位置？
- 如果把 load_matrix_sync 的 layout 改为列主，结果会怎样？

### 对应官方参考

- CUDA C++ Programming Guide Section B.30: "Warp Matrix Functions"
- CUDA PTX ISA Section "mma / ldmatrix" instructions
- Hopper Tuning Guide Section "Tensor Core"
- NVIDIA Blog "Tensor Cores in Hopper GPU"
- cuda-samples `wmma_*` examples

---

## 练习 G2：mma_sync_ptx

### 目标

学会直接使用 Ampere+ 的 PTX `mma.sync` 指令（而非通过 `nvcuda::wmma` wrapper），理解不同输入精度与输出精度组合（FP16/BF16/INT8 → FP32/INT32），以及 `ldmatrix` 与 `mma.sync` 的配对关系。通过 inline PTX，你会看到更底层的硬件行为与指令编码。

### 前置理解

- 完成 G1，理解 wmma 的基础概念
- 理解 PTX inline assembly 的基础语法（operand constraints、clobber list）
- 理解不同数据类型在 PTX 中的表示（`.f16`、`.f32`、`.s8` 等）

### 必做任务

1. // TODO [必做] 在 main.cu 头部 include `<cuda/ptx>` 或准备 inline PTX，声明一个 wrapper function 执行 `mma.sync.aligned.m16n8k16.row.col.f32.f16.f16.f32`（参数格式：M16N8K16，行主 A，列主 B，FP32 输出，FP16 inputs）。

2. // TODO [必做] 写两个 16×16（FP16）与 8×16（FP16）的矩阵，检验 m16n8k16 的数据排列与 ldmatrix 加载方式。

3. // TODO [必做] 使用 `ptx::mma` 命名空间下的绑定（如果 libcu++ 支持）或手写 inline PTX 调用 `mma.sync` 完成 16×8 tile 乘加。

4. // TODO [必做] 尝试改变输入精度为 BF16（`mma.sync.m16n8k16.f32.bf16.bf16.f32`），测量性能差异（BF16 通常与 FP16 相近）。

5. // TODO [必做] 尝试改变输入精度为 INT8 signed（`mma.sync.m16n8k16.s32.s8.s8.s32`），测量 INT8 GEMM 吞吐。

6. // TODO [必做] 在 Nsight Compute 中用 PTX 视图验证生成的 `mma.sync` 指令形式与输入中的 intent 一致。

### 进阶任务

- 实现 m16n8k32（一次 load 两个 K 维的 tile），观察 ldmatrix 加载次数的变化
- 对比 `mma.sync` 与 wmma 的 PTX 代码差异，理解 wrapper 的开销
- 尝试混合精度：BF16 input + FP32 output，验证转换的自动性

### 验收点

- 三个版本（FP16、BF16、INT8）均编译通过，无 PTX 语法错误
- 结果与 CPU 参考实现逐元素一致（INT8 允许较大精度误差）
- Nsight Compute PTX 视图中能看到 `mma.sync` 与 `ldmatrix` 交错
- 能指出 m16n8k16 vs m16n16k16 在 throughput 上的区别

### 观察点

- `ldmatrix` 每次加载 16 个元素（8×2 layout），与 mma.sync 的 matrix shape 紧密配合
- BF16 与 FP16 虽然精度不同，但 PTX `mma.sync` 指令的吞吐相同
- INT8 乘加在 FP32 中进行，输出是 32-bit 整数，避免溢出
- mma.sync 的 row/col layout 参数对应矩阵的 leading dimension 方向

### 常见坑

- PTX inline 中寄存器约束（`=r`、`r`）与实际寄存器宽度（32-bit vs 64-bit）不匹配，导致值丢失
- 混淆 `mma.sync` 的 layout specifier（row.col vs col.row），导致结果错误
- ldmatrix 加载的元素数与 mma.sync 期望的矩阵形状不对应，某些计算被忽略
- 没有考虑 mma.sync 的 pipeline 延迟（多个 clock cycle），导致依赖链过长
- INT8 overflow：如果输入范围超过 [-128, 127]，结果会环绕
- 混淆 `.s8` signed 与 `.u8` unsigned，导致符号扩展错误
- 在 sm_75 以下硬件上尝试 mma.sync，导致编译或执行失败

### 提示

- libcu++ 提供了 `ptx::mma` namespace，定义了类型安全的 mma.sync 包装
- 如果编译器不支持，fallback 到 inline PTX：`asm("mma.sync.aligned.m16n8k16.row.col.f32.f16.f16.f32 ..."`
- ldmatrix 加载的数据需要处于 global memory 或 shared memory，且对齐到 16 byte
- 验证 PTX：用 `nvcc --keep --ptx` 生成 .ptx 文件，grep `mma.sync` 查看生成的指令
- Nsight Compute 的 "SM Speed of Light" 表格中可查看 FP16/BF16/INT8 GEMM 的理论峰值

### 复盘问题

- m16n8k16 vs m16n16k16 分别用多少个 warp 执行，以及为什么 Hopper 引入了新的 shape？
- ldmatrix 与 mma.sync 之间的数据依赖是什么，Pipeline 延迟的关键路径在哪？
- INT8 GEMM 的吞吐为什么通常是 FP16 GEMM 的 2 倍（理论上），硬件是否提供了特殊支持？
- BF16 vs FP16 在精度与吞吐的权衡上各有何优缺点？

### 对应官方参考

- CUDA PTX ISA Section "mma" instruction（所有 variants）
- CUDA PTX ISA Section "ldmatrix" instruction
- CUDA libcu++ `<cuda/ptx>` header documentation
- Hopper Tuning Guide Section "Mixed-Precision GEMM"
- Ada Compatibility Guide Section "Tensor Core in Ada"

---

## 练习 G3：hopper_wgmma

### 目标

掌握 Hopper 独有的 `wgmma.mma_async` 指令，理解 128-thread warp group 概念（而非传统 32-thread warp），学会异步乘加的执行模型、FP8 E4M3/E5M2 精度选择、`setmaxnreg` 寄存器重新分配。通过对比 mma.sync（同步、warp-level）与 wgmma（异步、warp-group-level），建立对 Hopper 执行模型的深层理解。

### 前置理解

- 完成 G1/G2，理解 wmma 与 mma.sync 的基础
- 理解 Hopper 的 warp group 概念：4 个 warp（128 个线程）协同操作
- 理解 FP8 格式：E4M3（8 bit，指数 4，尾数 3）和 E5M2（8 bit，指数 5，尾数 2）的范围与精度权衡

### 必做任务

1. // TODO [必做] 在 kernel 定义前加 `__cluster_dims__(1,1,1)` 声明（虽然这个练习不强制用 cluster，但为后续 G5 预热）。

2. // TODO [必做] 写一个 blockDim = (128, 1, 1) 的 kernel（整个 warp group），每个 warp group 负责一个更大的 tile（例如 64×64，分解为多个 wgmma 操作）。

3. // TODO [必做] 使用 `ptx::wgmma::mma_async` 或 inline PTX `wgmma.mma_async` 执行 FP16→FP32 的乘加（形如 `wgmma.mma_async.sync.aligned.m64n128k16.f32.f16`，实际 shape 取决于 warp group 协调方式）。

4. // TODO [必做] 加上 `cuda::barrier<cuda::thread_scope_block>` 机制，让 warp group 内所有 128 个线程协调完成。与 mbarrier 配对做 transaction count 计数。

5. // TODO [必做] 尝试 FP8 E4M3 输入精度（`wgmma.mma_async.sync.aligned.m64n128k32.f32.e4m3`）。设置每行或每列的 scale factor（per-tensor vs per-row scaling）。

6. // TODO [必做] 测量吞吐量（TFLOPS）并对比 G2 的 mma.sync 版本。记录 Nsight Compute 中的 "Warp Occupancy"、"Tensor Core Clock Throughput"。

### 进阶任务

- 实现一个双缓冲版本：warp group 0 加载 tile 0，warp group 1 计算 tile -1，以实现计算与数据移动的重叠
- 尝试 FP8 E5M2 精度，对比 E4M3 的精度/吞吐权衡
- 在 Blackwell sm_100a 上预览新 MMA 形式（如 MXFP8），验证指令兼容性

### 验收点

- wgmma kernel 编译通过，sm_90a 或更高硬件上运行无错误
- 结果与 CPU 参考或 G2 的结果在数值精度范围内一致
- Nsight Compute 显示 warp group 内所有 128 个线程均参与计算（"Warp Execution Efficiency"应接近 100%）
- 能指出 wgmma 的吞吐相比 mma.sync 的改进（通常因 warp group 更大而更高利用率）

### 观察点

- wgmma 是 async 指令，发出后 warp 可以立即离开，不需等待结果
- 128-thread warp group 相比 32-thread warp 对内存数据的吞吐与计算都有更强的协调能力
- FP8 的 per-row scaling 相比 per-tensor scaling 能提供更好的精度，但需要硬件支持（Hopper 支持）
- 单个 `wgmma.mma_async` 完成的计算量相比 `mma.sync` 通常大 4-8 倍

### 常见坑

- wgmma 需要完整的 128 个线程启动，任何缺失导致行为未定义（不像 wmma/mma.sync 是 warp 级的）
- FP8 scale factor 设置错误，导致数值溢出或精度丧失
- 混淆 E4M3 与 E5M2 的范围（E4M3: ~-496 to 496；E5M2: ~-57344 to 57344）
- 没有使用 mbarrier 或其他同步机制，导致后续 warp group 看到旧数据
- `setmaxnreg` 不当调节，导致寄存器不足或浪费
- wgmma 发出的异步任务在 kernel 结束前未完成，导致结果未写回 global memory
- 在非 sm_90a 硬件上编译或运行，导致指令不支持

### 提示

- wgmma 的 async 性质：`wgmma.mma_async` 启动计算后，该指令不会 block 当前 warp，其他指令可继续发出（需要确保依赖正确）
- 128-thread warp group = 4 个 warp（warp 0-3），线程 threadIdx.x % 32 对应 lane ID，threadIdx.x / 32 对应 warp within group ID
- FP8 scaling：per-tensor 时全矩阵共享一个 scale；per-row/per-column 时各行/列各有一个 scale
- 使用 `ptx::wgmma` 时确保包含了正确的 libcu++ 头文件（CUDA Toolkit 13.0+）
- Nsight Compute PTX 视图中搜索 `wgmma` 来验证指令生成

### 复盘问题

- wgmma 的 128-thread warp group 相比 mma.sync 的 32-thread warp，在硬件执行模型上有什么本质区别？
- FP8 E4M3 vs E5M2 分别在什么场景下更适合，以及精度权衡的考虑？
- `setmaxnreg` 的作用是什么，过多或过少分配寄存器会导致什么后果？
- async wgmma 何时真正完成计算，如何用 mbarrier 确保数据可见性？

### 对应官方参考

- CUDA C++ Programming Guide Section 3.2.6: "Warp Group Matrix Multiply (wgmma)"
- CUDA PTX ISA Section "wgmma" instructions（所有 variants）
- Hopper Tuning Guide Section "Warp Group MMA"
- CUDA Toolkit 13.0 Release Notes: "FP8 Tensor Core support"
- NVIDIA Blog "Hopper GPU Architecture Deep Dive"

---

## 练习 G4：tma_cp_async_bulk

### 目标

学会使用 Tensor Memory Accelerator（TMA）进行异步大粒度数据搬运，从 global memory 到 shared memory（或反向）。通过 host 端的 `CUtensorMap` 构造与 `cuTensorMapEncodeTiled` 编码、kernel 端的 `__grid_constant__` 参数传递、`ptx::cp_async_bulk_tensor` 指令、`cuda::barrier` 同步，你会看到如何卸载 kernel 内的数据搬运工作到硬件单元。

### 前置理解

- 完成 C4（Hopper cluster）与 C5（mbarrier）
- 理解 TMA 是一个独立硬件单元，不占用 SM 计算资源
- 理解 shared memory 的分布式访问（distributed smem）在 cluster 上的地址空间

### 必做任务

1. // TODO [必做] 在 host 代码中构造一个 `CUtensorMapDataType::CU_TENSOR_MAP_DATA_TYPE_FLOAT16` 的 tensor map，用 `cuTensorMapEncodeTiled` 编码一个 FP16 矩阵的布局（shape、stride、tile shape 等）。

2. // TODO [必做] 用 `cudaMemcpyToSymbol` 或 `cudaGetSymbolAddress` 把 tensor map 复制到 device 的 `__grid_constant__` symbol 中（kernel 端声明为 `__grid_constant__ CUtensorMap g_tensorMap`）。

3. // TODO [必做] 在 kernel 中用 `ptx::cp_async_bulk_tensor` 从 global memory 异步搬运一个 tile 到 shared memory。指定 source tensor map、目标 smem 地址、 coordinate。

4. // TODO [必做] 使用 `cuda::barrier` 与 `cuda::device::barrier_arrive_tx` 追踪 TMA 搬运的字节数。consumer 线程调用 `.wait()` 等待数据到达。

5. // TODO [必做] 尝试 TMA swizzle mode（`CU_TENSOR_MAP_SWIZZLE_128B`），观察如何消除 shared memory bank conflict。对比有无 swizzle 的性能差异。

6. // TODO [必做] 在 Nsight Compute 中查看 "Memory Transactions"、"L2 Hit Rate"，验证 TMA 的搬运确实绕过了 L1 并减少了事务数。

### 进阶任务

- 实现一个三层 pipeline：stage 0 TMA 搬运 tile A；stage 1 TMA 搬运 tile B、compute 用 tile A；stage 2 compute 用 tile B、TMA 写回结果
- 尝试多个 tensor map（分别用于 A、B、C），验证 grid_constant 能否容纳多个 map
- 测量 TMA 搬运的延迟（到达 smem 的时间）与理论峰值带宽的接近程度

### 验收点

- kernel 编译通过，需要 sm_90+ 编译支持
- TMA 搬运的数据与预期的 tile 内容完全一致（可逐字节验证）
- swizzle 模式应该消除 shared memory bank conflict（Nsight 报告应显示 `shared_ld_bank_conflict` 为 0）
- barrier transaction count 与实际搬运字节数一致

### 观察点

- TMA 的搬运是硬件加速，不消耗 SM 资源，只消耗 global memory 带宽与 L2 cache
- swizzle 通过改变数据在 smem 中的布局来打破 bank conflict，对于某些 tile 大小极其有效
- `__grid_constant__` 参数只能在 kernel launch 时通过 `cudaLaunchKernelEx` 修改，不能像普通参数一样动态变化
- TMA 搬运通常比 kernel 内手动的 shared memory 加载快 2-3 倍（对于大 tile）

### 常见坑

- CUtensorMap 构造时的 shape 与 stride 参数错误，导致 TMA 读取越界或地址计算错误
- `cuTensorMapEncodeTiled` 的 tile shape 必须与实际使用的 shared memory tile 大小一致，否则数据对不齐
- `__grid_constant__` 声明必须在 global scope，不能在 kernel 内部或 namespace 内
- TMA swizzle mode 与 shared memory layout 不匹配，导致数据乱序
- barrier 的 transaction count 指定错误，导致 consumer 等待超时或过早唤醒
- 没有考虑 TMA 搬运的延迟，立即从 smem 读数据，导致读到垃圾
- TMA 搬运 16-byte 对齐之外的数据，导致硬件异常

### 提示

- CUtensorMap 初始化：先用 `memset` 清零，再调用 `cuTensorMapEncodeTiled` 填充各字段
- swizzle mode 常用值：`CU_TENSOR_MAP_SWIZZLE_NONE`（0）、`CU_TENSOR_MAP_SWIZZLE_128B`（1）、`CU_TENSOR_MAP_SWIZZLE_64B`（2）
- TMA 搬运地址必须 16-byte 对齐；shared memory 基址通常是 0，offset 需自己计算
- `ptx::cp_async_bulk_tensor` 对应 PTX 指令 `cp.async.bulk.tensor` 或 `cp.async.bulk.tensor.prefetch`
- 验证 TMA 指令：Nsight Compute PTX 视图中搜索 `cp.async.bulk.tensor`

### 复盘问题

- TMA 的搬运与传统的 kernel 内 shared memory 加载相比，优势在哪里，局限性是什么？
- swizzle 如何消除 bank conflict，原理是什么？
- `__grid_constant__` 与普通 kernel 参数的区别是什么，为什么 TMA tensor map 必须是 grid constant？
- barrier transaction count 的作用是什么，为什么需要精确指定搬运字节数？

### 对应官方参考

- CUDA C++ Programming Guide Chapter 11.8: "Asynchronous Data Copies and TMA"
- CUDA Driver API: `cuTensorMapEncodeTiled` documentation
- CUDA PTX ISA Section "cp.async.bulk.tensor" instructions
- Hopper Tuning Guide Section "Tensor Memory Accelerator"
- NVIDIA Blog "Tensor Memory Accelerator in Hopper"

---

## 练习 G5：warp_specialized_pipeline

### 目标

综合 G1-G4 的所有技术，实现 Hopper 的标志性模式：producer-consumer warp specialization。1 个 producer warp 专职发 TMA 指令搬运数据，7 个 consumer warp 专职执行 wgmma 计算，通过 mbarrier double-buffering 实现计算与搬运的完全重叠。这是后续模块 H（CUTLASS）的基础，也是理解 Hopper 硬件设计的关键。

### 前置理解

- 完成 G1-G4，理解 wmma / wgmma / TMA / mbarrier 的各个环节
- 理解 warp specialization 的思想：不同线程负责不同职能（I/O vs 计算）
- 理解 double-buffering 的核心思路：buffer A 用于 stage N，buffer B 用于 stage N+1，通过屏障切换

### 必做任务

1. // TODO [必做] 设计 kernel 线程配置：blockDim = (256, 1, 1)，其中线程 0-31 为 producer warp，线程 32-255 为 7 个 consumer warp。

2. // TODO [必做] 在 shared memory 中分配两个 buffer（A 与 B），每个能容纳一个输入 tile（例如 64×64 FP16）。

3. // TODO [必做] producer warp 的逻辑：loop over blocks，每次发一个 TMA 搬运指令把下一个 block 搬到 buffer A 或 B（交替）。调用 `barrier_arrive_tx` 告知 consumer 有多少字节待到达。

4. // TODO [必做] consumer warp 的逻辑：等待 barrier 就绪（当前 buffer 数据已到），执行 wgmma 乘以当前 buffer 的数据，计算完成后 switch buffer。

5. // TODO [必做] 实现 double-buffering 的屏障切换机制：第一个 mbarrier 用于 "buffer A 就绪"，第二个用于 "buffer B 就绪"（或用同一 barrier 的多个 generation）。

6. // TODO [必做] 测量整个 kernel 的吞吐量（TFLOPS），对比 G3 的单纯 wgmma 版本。Nsight Compute 中观察 "SM Occupancy"、"Memory Throughput"、"Tensor Core Utilization"、warp scheduling 情况。

### 进阶任务

- 实现三缓冲或四缓冲，进一步重叠计算与搬运
- 尝试不同的 tile 大小（32×32 vs 64×64 vs 128×128），观察 buffer 大小、寄存器压力、整体吞吐的权衡
- 集成 Blackwell sm_100a 的新 MMA 形式（如 MXFP8），验证 producer-consumer 模式的通用性

### 验收点

- kernel 编译通过，sm_90a 硬件上运行
- GEMM 结果与 CPU 参考或前述单 block 版本逐元素一致
- warp specialization 版本的吞吐明显高于 non-specialized 版本（通常快 30-50%）
- Nsight Compute 显示 producer warp 与 consumer warp 的时间线错开（通过 "Warp Occupancy" 或 PTX timeline 可见）
- barrier 的 expected-tx count 与实际搬运字节数匹配

### 观察点

- producer warp 与 consumer warp 的分工完全隔离，producer 可以提前发 TMA 指令，consumer 可以安心计算
- double-buffering 的效果：当 consumer 使用 buffer A 计算时，producer 已经开始向 buffer B 搬数据，完美重叠
- 寄存器压力在 warp specialization 中可能更高（因为每个 warp 专注于一项任务，可能用更多寄存器）
- `setmaxnreg` 可用来为 producer/consumer 分配不同的寄存器配额

### 常见坑

- producer warp 与 consumer warp 的屏障同步不当，导致 consumer 过早开始计算或 producer 过度抢占带宽
- buffer 大小不足，无法容纳一个完整的 tile，导致溢出或 TMA 异常
- mbarrier 的 expected-tx 初始化错误，导致 consumer 永远等待或过早唤醒
- 没有正确处理 pipeline flush：最后几个 stage 需要特殊处理，确保所有数据都被消费完
- producer warp 的 TMA 指令发出后没有持续进行，导致搬运间隙出现，consumer 必须等待
- consumer warp 之间的数据竞争（如果不同 consumer 访问同一 buffer 的不同部分需要同步）
- 在 non-Hopper 硬件上运行导致 wgmma 或 `__cluster_dims__` 不支持

### 提示

- mbarrier 双缓冲：第 N 个 block 完成时，切换到第 N+1 个 barrier 对象或同一对象的不同 generation
- producer warp 调用 `cuda::device::barrier_arrive_tx(barrier, byte_count)`，consumer 调用 `barrier.wait()`
- warp specialization 中常用 `__syncthreads()` 在整个 block 层面做一次初始化同步，之后 producer/consumer 各自独立
- Nsight Compute 的 "Warp Occupancy" 或 "Active Warps" 指标可用来确认 warp specialization 的并发效果
- 调试技巧：用 shared memory 中的计数器记录 producer/consumer 的进度，kernel 结束后读出打印

### 复盘问题

- warp specialization 相比全部 warp 都做计算的模式，为什么能提升整体吞吐？
- double-buffering 何时最有效，buffer 数量过多（三缓冲+）还有收益吗？
- producer warp 与 consumer warp 之间的"契约"是什么，mbarrier 在其中扮演什么角色？
- 如果 producer warp 的 TMA 搬运速度跟不上 consumer 的计算速度，会发生什么？

### 对应官方参考

- Hopper Tuning Guide Section "Warp Specialization"
- Hopper Tuning Guide Section "Double-Buffering with mbarrier"
- NVIDIA Blog "Warp Specialization in Hopper"
- CUTLASS 3.x examples `hopper_warp_specialized_gemm`
- CUDA C++ Programming Guide Section 3.2.9: "Synchronization Primitives"

---

## 做完本模块后应达到的水平

**知识检查清单**

- 能清楚解释 wmma / mma.sync / wgmma 三代指令的硬件对应关系与 API 差异
- 理解 ldmatrix 与 mma.sync 的配对，以及为什么需要 16-byte 对齐
- 掌握 Hopper wgmma 的异步执行模型，包括 128-thread warp group 的协调
- 理解 TMA（Tensor Memory Accelerator）的工作原理与 CUtensorMap 的参数编码
- 能用 mbarrier 实现 producer-consumer pipeline 的同步与事务计数
- 理解 FP8 E4M3/E5M2 精度选择与 per-tensor/per-row scaling 的权衡

**能做到**

- 用 wmma 实现单 warp 的 GEMM 并用 Nsight Compute 验证 Tensor Core 利用率
- 用 mma.sync PTX 实现多精度 GEMM（FP16/BF16/INT8）
- 用 wgmma 实现 128-thread warp group 的异步乘加
- 用 TMA 实现高效的异步数据搬运，消除 bank conflict
- 设计并实现 warp-specialized producer-consumer pipeline，验证搬运与计算的完全重叠

**见模块 H** 关于如何用 CUTLASS CollectiveBuilder 自动生成这些优化模式，以及 cuTe 的 Layout/Tensor 抽象如何简化复杂的索引计算。

