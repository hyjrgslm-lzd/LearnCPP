# 练习 G2：mma_sync_ptx

## 目标

`[G2-T01]` (main.cu:2) 练习 G2：mma_sync_ptx — Ampere+ PTX `mma.sync` 指令直接使用。
`[G2-T02]` (main.cu:4) 学习目标。
`[G2-T03]` (main.cu:11) 编译命令。
`[G2-T04]` (main.cu:12) 运行命令。
`[G2-T05]` (main.cu:13) 硬件要求。

学会直接使用 Ampere+ 的 PTX `mma.sync` 指令（而非通过 `nvcuda::wmma` wrapper），理解不同输入精度与输出精度组合（FP16/BF16/INT8 -> FP32/INT32），以及 `ldmatrix` 与 `mma.sync` 的配对关系。通过 inline PTX，你会看到更底层的硬件行为与指令编码。

## 硬件要求

- Compute Capability：sm_80+（Ampere / Ada / Hopper）
- CUDA Toolkit：13.x+

## 前置理解

- 完成 G1，理解 wmma 的基础概念
- 理解 PTX inline assembly 的基础语法（operand constraints、clobber list）
- 理解不同数据类型在 PTX 中的表示（`.f16`、`.f32`、`.s8` 等）

## 必做任务

`[G2-T06]` (main.cu:29) 常量：m16n8k16 的矩阵尺寸。
`[G2-T07]` (main.cu:39) 辅助：FP16 CPU GEMM（行主 A x 列主 B）。
`[G2-T08]` (main.cu:77) TODO [必做] 步骤 1：PTX `mma.sync` 包装。
`[G2-T09]` (main.cu:81) 寄存器布局（warp 内，每 thread 持有的片段）。
`[G2-T10]` (main.cu:87) 输出 C（FP32，4 寄存器/thread）。
`[G2-T11]` (main.cu:89) 输入 A（FP16，8 个 unsigned 代表 8 个 .f16x2 寄存器）。
`[G2-T12]` (main.cu:92) 输入 B（FP16，4 个 unsigned 代表 4 个 .f16x2 寄存器）。
`[G2-T13]` (main.cu:94) 输入 C（FP32 累加器初始值）。
`[G2-T14]` (main.cu:97) TODO [必做] 步骤 1：inline PTX 调用。
`[G2-T15]` (main.cu:119) TODO [必做] 步骤 4：BF16 版本包装。
`[G2-T16]` (main.cu:129) TODO [必做] 步骤 4：将 .f16 替换为 .bf16。
`[G2-T17]` (main.cu:145) TODO [必做] 步骤 5：INT8 版本包装。
`[G2-T18]` (main.cu:154) TODO [必做] 步骤 5：INT8 mma.sync。
`[G2-T19]` (main.cu:170) Kernel：FP16 mma.sync（m16n8k16，单 warp，单 tile）。
`[G2-T20]` (main.cu:181) TODO [必做] 步骤 2：将 A/B 搬到 shared memory（ldmatrix 需要 smem）。
`[G2-T21]` (main.cu:187) 协作加载到 smem（每线程搬运 16/32x2 = 1 个 half4 块）。
`[G2-T22]` (main.cu:188) TODO [必做] 步骤 2：ldmatrix 需要将数据放入 shared memory 后再加载。
`[G2-T23]` (main.cu:194) TODO [必做] 步骤 3：ldmatrix 加载 A/B。
`[G2-T28]` (main.cu:239) TODO [必做] 步骤 3：调用 FP16 mma.sync。
`[G2-T30]` (main.cu:246) TODO [必做] 步骤 3：按 mma.sync 输出寄存器布局写回 global memory。
`[G2-T31]` (main.cu:262) Kernel：INT8 mma.sync（stub）。
`[G2-T32]` (main.cu:270) TODO [必做] 步骤 5：加载 A/B（s8），调用 mma_m16n8k16_s8，写回 C。
`[G2-T34]` (main.cu:291) TODO [必做] 步骤 2：构造 16x16 (FP16) A 和 16x8 (FP16) B。
`[G2-T43]` (main.cu:378) TODO [必做] 步骤 6：Nsight Compute PTX 视图验证。

1. `// TODO [必做]` 在 main.cu 头部 include `<cuda/ptx>` 或准备 inline PTX，声明一个 wrapper function 执行 `mma.sync.aligned.m16n8k16.row.col.f32.f16.f16.f32`（参数格式：M16N8K16，行主 A，列主 B，FP32 输出，FP16 inputs）。
2. `// TODO [必做]` 写两个 16x16（FP16）与 8x16（FP16）的矩阵，检验 m16n8k16 的数据排列与 ldmatrix 加载方式。
3. `// TODO [必做]` 使用 `ptx::mma` 命名空间下的绑定（如果 libcu++ 支持）或手写 inline PTX 调用 `mma.sync` 完成 16x8 tile 乘加。
4. `// TODO [必做]` 尝试改变输入精度为 BF16（`mma.sync.m16n8k16.f32.bf16.bf16.f32`），测量性能差异（BF16 通常与 FP16 相近）。
5. `// TODO [必做]` 尝试改变输入精度为 INT8 signed（`mma.sync.m16n8k16.s32.s8.s8.s32`），测量 INT8 GEMM 吞吐。
6. `// TODO [必做]` 在 Nsight Compute 中用 PTX 视图验证生成的 `mma.sync` 指令形式与输入中的 intent 一致。

## 进阶任务

`[G2-T24]` (main.cu:202) A: m16n8k16 -> 每线程 8 个 half，用 ldmatrix x4 加载。
`[G2-T25]` (main.cu:212) 高半部（K = 8..15）。
`[G2-T26]` (main.cu:221) B: m16n8k16 -> 每线程 4 个 half，用 ldmatrix x2 加载。
`[G2-T27]` (main.cu:236) 累加器清零。
`[G2-T29]` (main.cu:245) 写回 C（每 thread 写 2 个元素，行 = tid/4，列 = (tid%4)*2 + 0/1）。
`[G2-T44]` (main.cu:382) TODO [进阶] 实现 m16n8k32（K=32），观察 ldmatrix 加载次数变化。
`[G2-T45]` (main.cu:383) TODO [进阶] 对比 mma.sync 与 nvcuda::wmma 的 PTX 差异。
`[G2-T46]` (main.cu:384) TODO [进阶] 混合精度：BF16 input + FP32 output，验证转换自动性。

- 实现 m16n8k32（一次 load 两个 K 维的 tile），观察 ldmatrix 加载次数的变化
- 对比 `mma.sync` 与 wmma 的 PTX 代码差异，理解 wrapper 的开销
- 尝试混合精度：BF16 input + FP32 output，验证转换的自动性

## 验收点

`[G2-T33]` (main.cu:282) 主程序入口。
`[G2-T35]` (main.cu:309) CPU 参考。
`[G2-T36]` (main.cu:331) 启动 FP16 mma.sync kernel（单 warp，单 tile）。
`[G2-T38]` (main.cu:349) 简单验证首行。
`[G2-T40]` (main.cu:357) 启动 INT8 mma.sync kernel（stub）。

- 三个版本（FP16、BF16、INT8）均编译通过，无 PTX 语法错误
- 结果与 CPU 参考实现逐元素一致（INT8 允许较大精度误差）
- Nsight Compute PTX 视图中能看到 `mma.sync` 与 `ldmatrix` 交错
- 能指出 m16n8k16 vs m16n16k16 在 throughput 上的区别

## 观察点

- `ldmatrix` 每次加载 16 个元素（8x2 layout），与 mma.sync 的 matrix shape 紧密配合
- BF16 与 FP16 虽然精度不同，但 PTX `mma.sync` 指令的吞吐相同
- INT8 乘加在 FP32 中进行，输出是 32-bit 整数，避免溢出
- mma.sync 的 row/col layout 参数对应矩阵的 leading dimension 方向

## 常见坑

- PTX inline 中寄存器约束（`=r`、`r`）与实际寄存器宽度（32-bit vs 64-bit）不匹配，导致值丢失
- 混淆 `mma.sync` 的 layout specifier（row.col vs col.row），导致结果错误
- ldmatrix 加载的元素数与 mma.sync 期望的矩阵形状不对应，某些计算被忽略
- 没有考虑 mma.sync 的 pipeline 延迟（多个 clock cycle），导致依赖链过长
- INT8 overflow：如果输入范围超过 [-128, 127]，结果会环绕
- 混淆 `.s8` signed 与 `.u8` unsigned，导致符号扩展错误
- 在 sm_75 以下硬件上尝试 mma.sync，导致编译或执行失败

## 提示

- libcu++ 提供了 `ptx::mma` namespace，定义了类型安全的 mma.sync 包装
- 如果编译器不支持，fallback 到 inline PTX：`asm("mma.sync.aligned.m16n8k16.row.col.f32.f16.f16.f32 ...")`
- ldmatrix 加载的数据需要处于 global memory 或 shared memory，且对齐到 16 byte
- 验证 PTX：用 `nvcc --keep --ptx` 生成 .ptx 文件，grep `mma.sync` 查看生成的指令
- Nsight Compute 的 SM Speed of Light 表格中可查看 FP16/BF16/INT8 GEMM 的理论峰值

## 复盘问题

- m16n8k16 vs m16n16k16 分别用多少个 warp 执行，以及为什么 Hopper 引入了新的 shape？
- ldmatrix 与 mma.sync 之间的数据依赖是什么，Pipeline 延迟的关键路径在哪？
- INT8 GEMM 的吞吐为什么通常是 FP16 GEMM 的 2 倍（理论上），硬件是否提供了特殊支持？
- BF16 vs FP16 在精度与吞吐的权衡上各有何优缺点？

## 对应官方参考

- CUDA PTX ISA Section mma instruction（所有 variants）
- CUDA PTX ISA Section ldmatrix instruction
- CUDA libcu++ `<cuda/ptx>` header documentation
- Hopper Tuning Guide Section Mixed-Precision GEMM
- Ada Compatibility Guide Section Tensor Core in Ada

## 输出对照（printf / std::puts 原文）

- `[G2-T37]` (main.cu:338) 原文：`启动 mma_fp16_kernel: ...` -> 现：`launch mma_fp16_kernel: grid=(1,1,1) block=(32,1,1)`
- `[G2-T39]` (main.cu:353) 原文：`[mma_fp16_kernel] ... ms 首行验证: ...` -> 现：`[mma_fp16_kernel] %.3f ms first-row verify: PASS/FAIL(stub or layout pending)`
- `[G2-T41]` (main.cu:364) 原文：`启动 mma_s8_kernel: ...` -> 现：`launch mma_s8_kernel: grid=(1,1,1) block=(32,1,1)`
- `[G2-T42]` (main.cu:373) 原文：`[mma_s8_kernel] ... ms (stub — TODO [必做] 步骤 5 完成后验证)` -> 现：`[mma_s8_kernel] %.3f ms (stub - verify after TODO [REQUIRED] step 5)`
- `[G2-T47]` (main.cu:396) 原文：`[G2] 完成。用 nvcc --keep --ptx + grep 'mma.sync' 验证 PTX 指令生成。` -> 现：`[G2] done. Use nvcc --keep --ptx + grep 'mma.sync' to validate PTX generation.`
