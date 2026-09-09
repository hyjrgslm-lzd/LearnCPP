# 练习 G1：wmma_fp16_basic

## 目标

`[G1-T01]` (main.cu:2) 练习 G1：wmma_fp16_basic — Volta+ `nvcuda::wmma` API 基础用法。
`[G1-T02]` (main.cu:4) 学习目标。
`[G1-T03]` (main.cu:11) 编译命令。
`[G1-T04]` (main.cu:12) 运行命令。
`[G1-T05]` (main.cu:13) 硬件要求。

学会用 Volta+ 的 `nvcuda::wmma` API 构造和执行最简单的 Tensor Core 操作：`fragment<matrix_a/b, M,N,K>` 的构造、`load_matrix_sync` 加载、`mma_sync` 乘加、`store_matrix_sync` 存储。通过 M16N16K16 的 FP16 转 FP32 tile，建立对 Tensor Core 基础流水的理解。

## 硬件要求

- Compute Capability：sm_70+（Volta / Turing / Ampere / Ada / Hopper 均可）
- CUDA Toolkit：13.x+

## 前置理解

- 理解 GEMM 的分块策略：把 C 分成多个 16x16 的小 tile
- 理解 warp 作为 32-thread 执行单位，`nvcuda::wmma` 是 warp-level 的操作
- 理解 fragment 作为"寄存器上的数据视图"，不是真实内存对象

## 必做任务

`[G1-T06]` (main.cu:32) 常量定义。
`[G1-T07]` (main.cu:37) 矩阵尺寸 128x128x128，分解为 8x8x8 个 16x16x16 tile。
`[G1-T08]` (main.cu:43) CPU 参考实现（FP16 输入到 FP32 累加）。
`[G1-T09]` (main.cu:48) `C[i,j] = sum_l A[i,l] * B[l,j]`。
`[G1-T10]` (main.cu:62) 验证：逐元素比较（允许相对误差 1e-2）。
`[G1-T11]` (main.cu:84) Kernel 1：单 warp wmma GEMM（每个 warp 处理一个 16x16x16 tile）。
`[G1-T12]` (main.cu:91) TODO [必做] 步骤 1：声明三个 fragment。
`[G1-T13]` (main.cu:92) TODO [必做] 步骤 3：用 `load_matrix_sync` 加载 fA / fB。
`[G1-T14]` (main.cu:93) TODO [必做] 步骤 4：`mma_sync` 执行乘加。
`[G1-T15]` (main.cu:94) TODO [必做] 步骤 5：`store_matrix_sync` 写回结果。
`[G1-T16]` (main.cu:101) 每个 block（即 1 个 warp）负责输出 tile (tile_row, tile_col)。
`[G1-T17]` (main.cu:106) TODO [必做] 步骤 1：声明 fragment 模板。
`[G1-T18]` (main.cu:115) 累加器清零。
`[G1-T19]` (main.cu:117) TODO [必做] 步骤 3：逐 K-tile 循环加载 + mma。
`[G1-T20]` (main.cu:124) 计算 A、B tile 的起始指针。
`[G1-T21]` (main.cu:128) TODO [必做] 步骤 3：`load_matrix_sync`（注意对齐）。
`[G1-T22]` (main.cu:132) TODO [必做] 步骤 4：`mma_sync`。
`[G1-T23]` (main.cu:136) TODO [必做] 步骤 5：`store_matrix_sync` 写回。
`[G1-T31]` (main.cu:198) TODO [必做] 步骤 2：在 host 端构造 FP16 矩阵并复制到 device。
`[G1-T40]` (main.cu:281) TODO [必做] 步骤 6：Nsight Compute 性能测量。

1. `// TODO [必做]` 声明三个 fragment：`fragment<matrix_a, 16, 16, 16, half>` 类型的 fA，`fragment<matrix_b, 16, 16, 16, half>` 类型的 fB，`fragment<accumulator, 16, 16, 16, float>` 类型的 fC。初始化 fC 为全 0。
2. `// TODO [必做]` 在 host 端构造两个 16x16 的 half 矩阵（行主），复制到 device global memory。
3. `// TODO [必做]` 写一个 kernel，blockDim = (32, 1, 1)（单个 warp），每个 warp 使用 `load_matrix_sync` 从 global memory 加载 fA 和 fB（注意 ldmatrix 需要 16-byte 对齐的指针）。
4. `// TODO [必做]` 调用 `nvcuda::wmma::mma_sync(fC, fA, fB, fC)` 执行单次 16x16x16 乘加。
5. `// TODO [必做]` 用 `store_matrix_sync` 把结果 fC 写回 global memory，再在 host 端用 CPU 简单循环验证正确性。
6. `// TODO [必做]` 用 Nsight Compute 测量吞吐量（TFLOPS），对比理论 peak TFLOPS。记录 SM Occupancy、Warp Execution Efficiency、Tensor Core Utilization。

## 进阶任务

`[G1-T24]` (main.cu:146) Kernel 2（进阶）：tiled GEMM，block 内多 warp 分担 tile。
`[G1-T25]` (main.cu:158) warp 在 block 内的位置。
`[G1-T26]` (main.cu:163) 该 warp 负责的全局 tile。
`[G1-T27]` (main.cu:169) TODO [必做] 步骤 1：声明 fragment（同 Kernel 1）。
`[G1-T28]` (main.cu:175) TODO [必做] 步骤 3-4：K 维循环。
`[G1-T29]` (main.cu:185) TODO [必做] 步骤 5：写回。
`[G1-T41]` (main.cu:287) TODO [进阶] 尝试 M16N16K32 的 FP16 转 FP32（K=32 版本），观察 ldmatrix 加载次数变化。
`[G1-T42]` (main.cu:288) TODO [进阶] 对比 sm_70 Volta 与 sm_90a Hopper 的 SASS 差异（Nsight PTX 视图）。
`[G1-T43]` (main.cu:289) TODO [进阶] 用 half 作为累加器（`fragment<accumulator, 16,16,16, half>`），观察精度与吞吐变化。

- 尝试 M16N16K32 的 FP16 转 FP32，观察 ldmatrix 加载次数的变化
- 对比 sm_70 Volta 与 sm_90a Hopper 上的 SASS 指令差异（用 Nsight Compute PTX 视图）
- 尝试用 half 作为累加器（`fragment<accumulator, 16, 16, 16, half>`），观察精度变化与吞吐差异

## 验收点

`[G1-T30]` (main.cu:191) 主程序入口。
`[G1-T32]` (main.cu:213) 填充小范围随机值，避免 FP16 溢出。
`[G1-T33]` (main.cu:219) CPU 参考（在复制之前计算）。
`[G1-T34]` (main.cu:233) Kernel 1 启动：每 block = 1 warp，grid = (N/N_TILE, M/M_TILE)。
`[G1-T37]` (main.cu:256) Kernel 2 启动：tiled，block = WARPS_M*WARPS_N warp。

- wmma kernel 编译通过，无 warnings
- 单个 warp 执行的 GEMM 结果与 CPU 参考实现逐元素一致（可用 ULP tolerance）
- Nsight Compute 显示 Tensor Core Utilization >= 80%（单 warp 情况下通常 100%）
- 能指出 fragment load/store 中 ldmatrix 对齐要求的硬件原因

## 观察点

- wmma 操作在 PTX 中对应 `mma.sync` 与 `ldmatrix` 的组合
- 单个 warp 执行 wmma 可达 Hopper 上的理论 peak（考虑 warp 数量 32）
- fragment 是一种"虚拟"数据结构，编译器把它映射到物理寄存器上
- 16x16x16 FP16 转 FP32 对应 2x16x16x32 = 16384 FLOPs，在 2 个 4-clock cycle 内完成（理想情况）

## 常见坑

- fragment 声明时模板参数顺序错误（M/N/K 的顺序），导致编译失败或错误结果
- `load_matrix_sync` 的指针未按 16-byte 对齐，导致地址计算错误或 ldmatrix 异常
- 混淆 fragment 的 layout（row-major vs column-major），导致结果错误
- 在非 warp 对齐的线程上调用 wmma（例如 threadIdx.x >= 32），导致行为未定义
- 使用了 `__syncthreads` 在 wmma 调用前后，导致不必要的性能下降
- fragment 的 M*N*K 尺寸与硬件支持的尺寸不匹配（Volta 支持 16x16x16/16，Ada 更灵活）
- 从 global memory 加载时没有考虑 warp 内线程的分工方式，某些线程看不到数据
- 累加器 fragment 的元素数量计算错误，导致存储越界

## 提示

- fragment 初始化：`fill_fragment(fC, 0.0f)` 或通过构造函数
- 16-byte 对齐检查：`ptr % 16 == 0`，或用 `__align__(16)` 修饰符
- 单个 16x16 FP16 矩阵占 512 字节，必须连续存储在 global memory 中
- ldmatrix 在 PTX 中是 `ldmatrix.sync.aligned.m8n8` 指令，自动拆分为多个加载
- Nsight Compute 的 Tensor Pipe Efficiency 比 Warp Execution Efficiency 更反映 Tensor Core 真实利用率

## 复盘问题

- wmma 中 M、N、K 各自代表什么，以及它们如何映射到硬件 Tensor Core？
- 为什么 ldmatrix 需要 16-byte 对齐，而普通 global memory 访问只需 4-byte 对齐？
- 单个 warp 执行 mma_sync 需要多少个 clock cycle，以及如何通过 Nsight 验证？
- fragment 是寄存器还是共享内存，编译器如何决定其实际位置？
- 如果把 load_matrix_sync 的 layout 改为列主，结果会怎样？

## 对应官方参考

- CUDA C++ Programming Guide Section B.30: Warp Matrix Functions
- CUDA PTX ISA Section mma / ldmatrix instructions
- Hopper Tuning Guide Section Tensor Core
- NVIDIA Blog Tensor Cores in Hopper GPU
- cuda-samples `wmma_*` examples

## 输出对照（printf / std::puts 原文）

- `[G1-T35]` (main.cu:240) 原文：`启动 wmma_gemm_kernel: ...` -> 现：`launch wmma_gemm_kernel: grid=(...) block=(32,1,1)`
- `[G1-T36]` (main.cu:252) 原文：`[wmma_gemm_kernel] ... ms 验证: PASS/FAIL` -> 现：`[wmma_gemm_kernel] %.3f ms verify: PASS/FAIL`
- `[G1-T38]` (main.cu:264) 原文：`启动 wmma_gemm_tiled_kernel: ...` -> 现：`launch wmma_gemm_tiled_kernel: grid=(...) block=(...)`
- `[G1-T39]` (main.cu:276) 原文：`[wmma_gemm_tiled_kernel] ... ms 验证: PASS/FAIL` -> 现：`[wmma_gemm_tiled_kernel] %.3f ms verify: PASS/FAIL`
- `[G1-T44]` (main.cu:299) 原文：`[G1] 完成。用 ncu --set full ./G1_wmma_fp16_basic 查看 Tensor Core 利用率。` -> 现：`[G1] done. Use ncu --set full ./G1_wmma_fp16_basic to inspect Tensor Core utilization.`
