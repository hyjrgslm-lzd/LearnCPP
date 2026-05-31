# 练习 F2：Nsight Compute 与 kernel 级性能指标

## 目标

`[F2-T01]` (main.cu:2) 练习 F2：Nsight Compute 与 kernel 级性能指标。
`[F2-T02]` (main.cu:3) 学习目标：memory-bound copy kernel 高 DRAM 带宽利用率、低计算强度；compute-bound naive 128x128 matmul kernel 高 FMA 密度；`ncu --set full -o report` 采集后从 Speed-Of-Light 页面读出 Compute% / Memory%；Memory Workload Analysis 看 L1/L2/DRAM 事务数、coalescing efficiency；Source-SASS 关联源代码行号（需 `GPU_STUDY_LINEINFO=ON`）。
`[F2-T03]` (main.cu:9) 编译命令。
`[F2-T04]` (main.cu:10) 采集命令。
`[F2-T05]` (main.cu:11) 运行命令。

学会用 Nsight Compute 深度分析单个 kernel 的性能。从 "Speed Of Light" 页面读出 compute 和 memory 的实现吞吐、从 "Memory Workload Analysis" 看出访问模式、从 "Source-SASS" 关联高级代码和汇编。

## 前置理解

- 你知道 kernel 执行时间只是一个指标，真正关键是"吞吐与峰值的比例"。
- 你理解 L1/L2 缓存的层级。
- 你能读懂基本的 SASS 伪码（或至少知道它代表什么）。

## 编译

```bash
# 标准编译
cmake --build build --target F2_nsight_compute_first_look

# 启用行号信息（Source-SASS 视图所需）
cmake -DGPU_STUDY_LINEINFO=ON ..
cmake --build build --target F2_nsight_compute_first_look
```

## 采集命令

**完整采集（所有指标）**：

```bash
ncu --set full -o f2_report ./F2_nsight_compute_first_look
```

**快速查看 Speed Of Light**：

```bash
ncu --set speedoflight -o f2_sol ./F2_nsight_compute_first_look
```

**仅采集特定 kernel**：

```bash
ncu --set full --kernel-name kernel_copy_memory_bound -o f2_copy ./F2_nsight_compute_first_look
ncu --set full --kernel-name kernel_matmul_naive -o f2_matmul ./F2_nsight_compute_first_look
```

打开生成的 `.ncu-rep` 文件（Nsight Compute GUI）。

## 必做任务

`[F2-T06]` (main.cu:23) 常量定义。
`[F2-T07]` (main.cu:24) 16M 元素，64 MB（copy kernel 数据量）。
`[F2-T08]` (main.cu:25) 矩阵尺寸 128x128。
`[F2-T09]` (main.cu:26) matmul tile 大小。
`[F2-T10]` (main.cu:27) copy kernel block size。
`[F2-T11]` (main.cu:30) Kernel 1：memory-bound copy（带宽受限），AI = 2 ops / 8 bytes = 0.25 FLOP/byte，预期 Speed-Of-Light Memory% 高 / Compute% 低。
`[F2-T12]` (main.cu:33) TODO [必做-1] 完成 kernel 主体：`out[tid] = in[tid]`（带简单算术保留计算特征）。
`[F2-T13]` (main.cu:43) TODO [必做-1] `out[tid] = in[tid] * 1.0f + 0.0f;` 保持 memory-bound 特征：每元素 2 ops，访问 8 bytes。
`[F2-T14]` (main.cu:51) Kernel 2：compute-bound naive matmul（128x128），AI ≈ 21 FLOP/byte（高），预期 Compute% 高。注意：naive（无 tiling）用于清晰展示 compute-bound 特征。
`[F2-T15]` (main.cu:54) TODO [必做-1] 完成 kernel 主体：`C[row][col] = sum(A[row][k] * B[k][col])`。
`[F2-T16]` (main.cu:67) TODO [必做-1] 展开内积循环。
`[F2-T28]` (main.cu:159) TODO [必做-2] Nsight Compute 确认 Memory% 约等于带宽利用率 / 理论峰值 x100。
`[F2-T29]` (main.cu:160) TODO [必做-2] 记录 L1/L2/DRAM 事务数（Memory Workload Analysis 页面）。
`[F2-T31]` (main.cu:184) TODO [必做-3] Nsight Compute 确认 Compute% 约等于实现 FMA 吞吐 / 理论峰值 x100。
`[F2-T32]` (main.cu:185) TODO [必做-3] 导航到 Source-SASS 页面，关联内积循环行号。
`[F2-T33]` (main.cu:189) TODO [必做-4] 用 `ncu --set full -o f2_report` 采集本程序，在 GUI 中：打开 Speed Of Light 页面记录 Compute% 和 Memory%；判断每个 kernel 的瓶颈类型（compute-bound / memory-bound）；Memory Workload Analysis 记录 L1/L2/DRAM 读写事务数；将结论填写为注释。
`[F2-T34]` (main.cu:197) TODO [必做-5] 编译时启用 `GPU_STUDY_LINEINFO=ON`，重采样，观察 Source-SASS 页面是否能清晰关联内积循环的源代码行。

1. 完成 `kernel_copy_memory_bound` 和 `kernel_matmul_naive` 的主体实现。
2. 不用任何特殊编译选项，用 `ncu --set full -o report ./exe` 采样该应用。
3. Nsight Compute GUI 打开生成的报告；导航到 "Speed Of Light" 页面。观察 Compute %（实现吞吐 / 理论峰值）、Memory %（实现带宽 / 理论峰值），根据这两个指标判断 kernel 是 compute-bound 还是 memory-bound。
4. 导航到 "Memory Workload Analysis" 页面，观察 L1/L2/device memory 的读写事务数、是否有 bank conflict（如果有 shared memory）、coalescing efficiency（全局内存访问的合并率）。
5. 导航到 "Source-SASS" 页面；观察汇编代码。
6. 记录关键指标，给出分析结论。

## 两个 kernel 的理论算术强度（AI）

| Kernel | 计算量 | 数据访问量 | AI（FLOP/byte） | 预期瓶颈 |
|---|---|---|---|---|
| `kernel_copy_memory_bound` | 2 ops/元素 | 8 bytes/元素 | 0.25 | memory-bound |
| `kernel_matmul_naive`（128^3） | 2x128^3 FLOP | 3x128^2x4 bytes | 约 21 | compute-bound |

## Speed Of Light 解读指引

- `Memory % 高 + Compute % 低` -> **memory-bound**：优化方向是减少带宽压力（向量化、合并访问）。
- `Compute % 高 + Memory % 低` -> **compute-bound**：优化方向是减少指令延迟（循环展开、共享内存）。
- 两者都低 -> **latency-bound**：优化方向是提高 occupancy 或减少 pipeline stall。

## 观察点

`[F2-T17]` (main.cu:73) 辅助：打印算术强度估算 `AI = ops / bytes_accessed`。
`[F2-T19]` (main.cu:99) 分配内存。
`[F2-T20]` (main.cu:114) 初始化设备内存。
`[F2-T21]` (main.cu:120) 用 host 初始化矩阵（对角 = `1/MAT_N`，保证数值稳定）。
`[F2-T22]` (main.cu:138) 算术强度预估（在 Nsight Compute 中验证）。
`[F2-T23]` (main.cu:144) 1 mul + 1 add = 2 ops。
`[F2-T24]` (main.cu:145) 1 read + 1 write。
`[F2-T25]` (main.cu:148) 2 FLOP per FMA。
`[F2-T26]` (main.cu:149) A+B+C（naive: no cache reuse）。
`[F2-T27]` (main.cu:153) 运行 Kernel 1：memory-bound copy。
`[F2-T30]` (main.cu:172) 运行 Kernel 2：compute-bound naive matmul。

- Speed Of Light 页面是快速判断瓶颈的利器。
- coalescing efficiency 低说明存在不规则访问；bank conflict 计数高说明 shared memory 访问有竞争。
- Source-SASS 视图需要 debug info（调试信息），但不应该显著改变性能特性。

## 常见坑

1. 没有加 `--set full` 参数，默认采样集可能太快，某些细节指标不可用。
2. 在 Release 模式编译时加 `--generate-line-info` 会膨胀二进制，但不影响运行性能。
3. kernel 执行时间太短（< 1 ms），采样数据可能不准确；需要足够长的执行时间或多次 launch。
4. 误解 roofline 图的坐标轴（横轴是算术强度，不是其它）。
5. 以为 Nsight Compute 的指标自动对应代码行；实际需要 source-SASS 对齐才行。
6. 在多 GPU 系统上没有指定正确的 GPU ID（用 `-i` 参数）。
7. 忘记应用本身可能包含多个 kernel；需要筛选或单独采样特定 kernel。

## 进阶任务

`[F2-T35]` (main.cu:201) TODO [进阶-1] 用 `ncu --set speedoflight` 快速对比两个 kernel 的瓶颈类型。
`[F2-T36]` (main.cu:202) TODO [进阶-2] 在 matmul 中加 shared memory tiling，对比 AI 和 Memory% 变化。
`[F2-T37]` (main.cu:203) TODO [进阶-3] 用 Nsight Compute rule 功能，查看自动给出的优化建议。

- 编译时加 `--generate-line-info`（`GPU_STUDY_LINEINFO=ON`），重新采样，观察 Source-SASS 视图。
- 对同一个 kernel 的不同版本（naive vs tiled）分别采样，对比 roofline 位置的变化。
- 在 Nsight Compute 中使用 rule 功能，自动检测常见性能问题并给出建议。

## 验收点

`[F2-T18]` (main.cu:90) `main` 入口。
`[F2-T38]` (main.cu:207) 清理。
`[F2-T39]` (main.cu:215) 完成提示。

- 你能从报告中准确指出该 kernel 的 compute % 和 memory %。
- 你能判断 kernel 的瓶颈类型（compute-bound / memory-bound / latency-bound）。
- Memory Workload Analysis 显示了 L1/L2/device 的访问模式。
- 记录了至少一条可行的优化建议。

## 复盘问题

1. Speed Of Light 中的 Compute % 和 Memory % 各代表什么？两者都高是否可能？
2. 如何从 Memory Workload Analysis 判断访问模式是否优化？
3. bank conflict 会如何影响性能？
4. Source-SASS 视图中，一条源代码行通常对应多少条 SASS 指令？

## 对应官方参考

- Nsight Compute Kernel Profiling Guide：https://docs.nvidia.com/nsight-compute/ProfilingGuide/
- NVIDIA GPU Performance Metrics：https://docs.nvidia.com/gameworks/content/developertools/desktop/analysis/report/metrics.html

## 输出对照（printf / std::puts 原文）

- `[F2-T22]` (main.cu:142) 原文：`--- 算术强度（AI）理论估算 ---` -> 现：`--- Theoretical arithmetic intensity (AI) estimate ---`
- `[F2-T27]` (main.cu:154) 原文：`--- Kernel 1: memory-bound copy（ncu 预期：Memory%% 高）---` -> 现：`--- Kernel 1: memory-bound copy (ncu expected: Memory%% high) ---`
- `[F2-T30]` (main.cu:174) 原文：`--- Kernel 2: compute-bound naive matmul %dx%d（ncu 预期：Compute%% 高）---` -> 现：`--- Kernel 2: compute-bound naive matmul %dx%d (ncu expected: Compute%% high) ---`
- 启动信息中文 `启动:` -> `launch:`，`时间=%.3f ms 带宽=%.1f GB/s` -> `time=%.3f ms BW=%.1f GB/s`，`时间=%.3f ms GFLOP/s=%.1f（stub: TODO 完成后填入正确值）` -> `time=%.3f ms GFLOP/s=%.1f (stub: TODO fill correct value when complete)`。
- `[F2-T39]` (main.cu:218) 原文：`[F2] 完成。用 ncu --set full -o f2_report 采集后在 Nsight Compute GUI 打开报告。` -> 现：`[F2] done. ncu --set full -o f2_report and open report in Nsight Compute GUI.`
