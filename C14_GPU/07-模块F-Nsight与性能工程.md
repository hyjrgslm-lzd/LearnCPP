# 07 模块 F：Nsight 与性能工程

## 模块目标

这个模块是 Stage 1 的最后一个理论与工具模块。你会从被动的"代码能跑"升级为主动的"能用数据说话"。

Nsight Systems（系统级时间线）与 Nsight Compute（kernel 级深度分析）是 GPU 开发的两把瑞士军刀。你会学到：如何从时间线上看出 kernel 与显存拷贝是否真的并发、如何从 roofline 图判断一个 kernel 距离峰值还差多少、如何用 compute-sanitizer 捕捉那些隐蔽的 bug（数据竞争、越界访问、发散 `__syncthreads`）。

## 前置知识

- 模块 A–E 全部完成：kernel 编写、内存、线程、warp 原语、流事件。
- 对"延迟"与"吞吐"有直观认识。
- 能读基本的 unix/windows 命令行。

## 模块完成标准

做完本模块，你至少要能说清楚：

- Nsight Systems 中 GPU 操作（kernel、memcpy）的时间线与 CPU timeline 如何关联。
- Nsight Compute 的 "Speed Of Light" 页面里的 Compute % 和 Memory % 指标各表示什么。
- roofline 图的坐标轴（算术强度 vs 吞吐量）和如何从中推导优化方向。
- compute-sanitizer 的四类报告（memcheck、racecheck、synccheck、initcheck）各检测什么问题。
- 如何用 `--generate-line-info` 让 source-SASS 视图能关联到代码行号。

## 硬件与工具链要求

- **最低 Compute Capability**：CC 6.0+（Nsight Compute 支持）；建议 CC 7.0+ 以获得完整指标。
- **CUDA Toolkit**：13.x 含 Nsight 工具链。
- **Nsight 工具**：Nsight Systems、Nsight Compute、compute-sanitizer 均随 CUDA Toolkit 安装。
- **样例硬件**：Hopper sm_90a 为主；Ampere/Ada 回退对比。

---

## 练习 F1：Nsight Systems 时间线与 NVTX 标注

### 目标

学会用 Nsight Systems 采样一个完整应用，从时间线上直观观察 CPU-GPU 交互、多 stream 并发、kernel 与显存拷贝的重叠。通过 NVTX range 标注，把应用逻辑与底层 GPU 操作相关联。

### 前置理解

- 你知道 kernel launch、显存拷贝等操作都在 stream 上发生。
- 你理解 NVTX（NVIDIA Tools Extension）是用户级标注 API。
- 你能看懂时间线图（横轴时间，纵轴资源）。

### 必做任务

1. 写一个应用程序，包含以下步骤：
   - HtoD 拷贝一块 256 MB 数据。
   - 启动第一个 kernel（执行时间 10-50 ms）。
   - 同时启动第二个 kernel 在不同 stream 上（应该与第一个并发）。
   - DtoH 拷贝结果。
   - CPU 端做一些计算（例如 sleep 或 CPU loop）。
2. 不用任何标注，用 `nsys profile -o trace --stats=true ./exe` 采样程序。
3. 打开生成的 `trace.qdrep` 文件（用 Nsight Systems GUI），观察默认时间线；应该能看到 CUDA 操作但可能难以对应应用逻辑。
4. 在代码中用 `#include "common/nvtx_range.cuh"`（项目提供的 RAII 包装），在关键位置插入 NVTX range：
   - `nvtx_scoped_range scope("HtoD_256MB")`（使用提供的 RAII 类）
   - `nvtx_scoped_range scope("Kernel_A")`（kernel 启动之前）
   - `nvtx_scoped_range scope("Kernel_B_Stream1")`
   - `nvtx_scoped_range scope("DtoH")`
   - `nvtx_scoped_range scope("CPU_Work")`
5. 重新编译运行，再采样。打开时间线，应该能看到彩色的 NVTX range 条纹与 CUDA 操作相对应。
6. 观察并记录：两个 kernel 是否真的并发（应该有时间重叠）、DtoH 是否与某个 kernel 重叠。
7. 导出或截图关键部分的时间线，注解观察结果。

### 进阶任务

- 增加更多 stream 和 kernel，使时间线变得复杂；观察 Nsight Systems 如何展现这种复杂性。
- 尝试在 kernel 内部用 `__synchronous` 或异步操作改变执行时间，观察时间线如何变化。
- 用 NVTX 的 domain 功能区分不同的逻辑模块（需要调用 `nvtxDomainCreateA`；如果 RAII 包装不支持可以手工调用）。

### 验收点

- 时间线图清晰地显示了 HtoD、两个 kernel、DtoH 等操作。
- NVTX range 标注能够准确地标记应用的关键阶段。
- 你能从时间线上指出哪些操作有时间重叠、哪些是串行的。
- 截图或导出数据证明观察结果。

### 观察点

- 时间线的 CPU track 显示 kernel launch 和其它 host API 调用；GPU track 显示实际执行。
- launch 和实际执行之间可能有延迟（launch latency）。
- NVTX range 帮助把时间线与源代码逻辑对应起来，让分析更容易。
- 两个 stream 上的 kernel 如果没有依赖，应该能看到时间重叠。

### 常见坑

1. 没有链接 NVTX library（`nvToolsExt64_1.lib` 或类似），导致 NVTX 调用无效。
2. NVTX range 嵌套错误，导致时间线显示混乱。
3. 采样率设置过高（默认 1 kHz），导致采样器本身成为瓶颈，虚假放大时间。
4. 忘记 `--stats=true` 参数，错过了统计汇总页面。
5. 在时间线上看到的 kernel 名称是通用的（例如 `kernel_0`），而不是用户定义的名称；这是因为没有 debug info。
6. 用 Nsight Systems GUI 打开 `.qdrep` 文件时没有指定正确的 symbol 路径，导致无法解析函数名。

### 提示

- RAII 包装 `nvtx_range` 通常在 `common/nvtx_range.cuh` 中提供；确保包含路径正确。
- 如果用低级 `nvtxRangePushA` / `nvtxRangePop`，记得要配对。
- Nsight Systems GUI 可能需要几秒钟加载大 `.qdrep` 文件；耐心等待。

### 复盘问题

1. 时间线上的 CPU track 和 GPU track 分别代表什么？
2. launch 和实际执行之间的延迟通常是多久？
3. NVTX range 对性能有影响吗？
4. 如何在 Nsight Systems 时间线上确认两个 kernel 真的在并发？

### 对应官方参考

- Nsight Systems Documentation：https://docs.nvidia.com/nsight-systems/
- NVIDIA Tools Extension：https://docs.nvidia.com/gameworks/content/developertools/desktop/analysis/report/nvtx.htm

---

## 练习 F2：Nsight Compute 与 kernel 级性能指标

### 目标

学会用 Nsight Compute 深度分析单个 kernel 的性能。从 "Speed Of Light" 页面读出 compute 和 memory 的实现吞吐、从 "Memory Workload Analysis" 看出访问模式、从 "Source-SASS" 关联高级代码和汇编。

### 前置理解

- 你知道 kernel 执行时间只是一个指标，真正关键是"吞吐与峰值的比例"。
- 你理解 L1/L2 缓存的层级。
- 你能读懂基本的 SASS 伪码（或至少知道它代表什么）。

### 必做任务

1. 用之前模块的某个 kernel（例如 simple reduction 或 coalesced copy），或写一个新的轻量 kernel。
2. 不用任何特殊编译选项，用 `ncu --set full -o report ./exe` 采样该应用。
3. Nsight Compute GUI 打开生成的报告；导航到 "Speed Of Light" 页面。观察：
   - **Compute %**：实现吞吐 / 理论峰值。
   - **Memory %**：实现带宽 / 理论峰值。
   - 根据这两个指标判断 kernel 是 compute-bound 还是 memory-bound。
4. 导航到 "Memory Workload Analysis" 页面，观察：
   - L1/L2/device memory 的读写事务数。
   - 是否有 bank conflict（如果有 shared memory）。
   - coalescing efficiency（全局内存访问的合并率）。
5. 导航到 "Source-SASS" 页面；观察汇编代码（如果有源代码关联，能看到代码行对应的 SASS）。
6. 记录关键指标，给出分析结论（例如"该 kernel memory-bound，带宽利用率 45%，可能通过 X 优化"）。

### 进阶任务

- 编译时加 `--generate-line-info`（需要 CMake 设置 `GPU_STUDY_LINEINFO ON`），重新采样，观察 Source-SASS 视图是否能更清晰地关联源代码。
- 对同一个 kernel 的不同版本（例如 naive vs optimized）分别采样，对比 roofline 位置的变化。
- 在 Nsight Compute 中使用 rule 功能（predefined perf rules），自动检测常见性能问题并给出建议。

### 验收点

- 你能从报告中准确指出该 kernel 的 compute % 和 memory %。
- 你能判断 kernel 的瓶颈类型（compute-bound / memory-bound / latency-bound）。
- Memory Workload Analysis 显示了 L1/L2/device 的访问模式。
- 记录了至少一条可行的优化建议（可能来自 rule、也可能你自己分析得出）。

### 观察点

- Speed Of Light 页面是快速判断瓶颈的利器。
- 如果 Compute % 高但 Memory % 低，说明 memory 是瓶颈；反之 compute 是瓶颈。
- coalescing efficiency 低说明存在不规则访问；bank conflict 计数高说明 shared memory 访问有竞争。
- Source-SASS 视图需要 debug info，但不应该显著改变性能特性（除非 inline 策略改变）。

### 常见坑

1. 没有加 `--set full` 参数，默认采样集可能太快，某些细节指标不可用。
2. 在 Release 模式编译时加 `--generate-line-info` 会膨胀二进制，但不影响运行性能。
3. kernel 执行时间太短（< 1 ms），采样数据可能不准确；需要足够长的执行时间或多次 launch。
4. 误解 roofline 图的坐标轴（横轴是算术强度，不是其它）。
5. 以为 Nsight Compute 的指标自动对应代码行；实际需要 source-SASS 对齐才行。
6. 在多 GPU 系统上没有指定正确的 GPU ID（用 `-i` 参数）。
7. 忘记应用本身可能包含多个 kernel；需要筛选或单独采样特定 kernel。

### 提示

- `ncu --set full` 非常详细但慢；如果只要快速查看，可以用 `--set speedoflight`。
- 在 Nsight Compute GUI 中，可以导出数据为 CSV 或其它格式，便于后续分析。
- 如果报告加载很慢，可能是数据量太大；尝试降低采样周期或缩小分析范围。

### 复盘问题

1. Speed Of Light 中的 Compute % 和 Memory % 各代表什么？两者都高是否可能？
2. 如何从 Memory Workload Analysis 判断访问模式是否优化？
3. bank conflict 会如何影响性能？
4. Source-SASS 视图中，一条源代码行通常对应多少条 SASS 指令？

### 对应官方参考

- Nsight Compute Kernel Profiling Guide：https://docs.nvidia.com/nsight-compute/ProfilingGuide/
- NVIDIA GPU Performance Metrics：https://docs.nvidia.com/gameworks/content/developertools/desktop/analysis/report/metrics.html

---

## 练习 F3：Roofline 模型与性能分析

### 目标

理解 roofline 模型的概念、如何从 Nsight Compute 的 roofline 图中读出算术强度和吞吐、以及如何用 roofline 推导优化方向。通过把多个 kernel 绘制在同一个 roofline 图上，直观看出性能差距和优化潜力。

### 前置理解

- 你知道算术强度的定义：`(计算次数) / (字节搬运)`。
- 你理解 GPU 既有计算峰值，也有带宽上限，二者共同约束吞吐。
- 你对 F2 中的 Nsight Compute 使用有基本认识。

### 必做任务

1. 从之前的练习（例如 D6 或自己写的 kernel）中选择三个 kernel：
   - **K1**：减法算子（低算术强度，预期内存绑定）。
   - **K2**：中等算术强度算子（例如 matrix transpose with computation）。
   - **K3**：算术强度高的算子（例如 GEMM 内层循环）。
2. 分别对三个 kernel 用 `ncu --set full -o report_k<i> ./exe_k<i>` 采样。
3. 对每个报告，手工计算或从 Nsight Compute 报告中提取：
   - 算术强度（FLOP/byte 或 OP/byte）。
   - 实现吞吐（FLOP/s 或 GB/s）。
4. 在一张 roofline 图上绘制三个点，横轴为算术强度，纵轴为实现吞吐。同时画出两条边界线：
   - 计算 roof：`吞吐 = 计算峰值`（水平线）。
   - 内存 roof：`吞吐 = 带宽 * 算术强度`（斜线）。
5. 观察三个点各自落在哪个象限（内存绑定还是计算绑定），以及距离 roof 还差多远。
6. 基于 roofline 位置，给出每个 kernel 的优化建议（例如"K1 应该增加算术强度"、"K2 接近内存 roof，优化空间有限"）。

### 进阶任务

- 尝试优化其中一个 kernel（例如用 shared memory 或循环展开提高算术强度），重新采样并绘制新的点，观察在 roofline 上的位移。
- 如果有多块硬件（例如 Hopper 和 Ampere），分别绘制两个 roofline，对比相同 kernel 在两者上的位置。
- 用 Nsight Compute 的内置 roofline chart（如果可用），对比自己手工绘制的结果。

### 验收点

- 三个 kernel 的 roofline 图完整，标注清楚。
- 你能准确说出每个 kernel 的算术强度和实现吞吐。
- 你能解释为什么某个 kernel 被内存 roof 或计算 roof 约束。
- 至少给出一条可行的优化建议，并说明原理。

### 观察点

- roofline 模型把性能分析简化为"吞吐约束"的问题。
- 低算术强度的 kernel（内存绑定）优化的方向是"降低数据搬运"或"增加局部计算"。
- 高算术强度的 kernel（计算绑定）优化的方向是"更好地利用计算单元"或"降低指令延迟"。
- 如果一个 kernel 已经接近计算 roof，进一步优化的收益很小；应该考虑算法替代。

### 常见坑

1. 算术强度计算错误（忘记考虑缓存效果或多次访问）。
2. 单位混淆（FLOP/s vs GB/s，或者 bits vs bytes）。
3. 理论峰值填错（查错硬件规格或 GPU 规格）。
4. roofline 斜线的斜率不对（应该 = 带宽 / 计算峰值）。
5. 选择的三个 kernel 算术强度差距太小，无法清晰展示 roofline 的两个区域。

### 提示

- Nsight Compute 报告中通常有"arithmetic intensity"等字段，可以直接使用而不需手工计算。
- roofline 图可以用 Excel、Python matplotlib 或其它工具绘制。
- 理论吞吐从 GPU 规格取（例如 Hopper sm_90a 的 FP32 峰值是多少 GB/s、多少 TFLOPS）。

### 复盘问题

1. roofline 图的两条边界线各代表什么约束？
2. 如果一个 kernel 的点在 roofline 图上正好在两条边界线的交点，说明什么？
3. 如何从 roofline 位置推导优化方向？
4. 两个 kernel 算术强度相同但吞吐不同，可能的原因是什么？

### 对应官方参考

- Roofline 论文：S. Williams et al., "Roofline: An Insightful Visual Performance Model for Floating-Point Programs"
- Nsight Compute Roofline：https://docs.nvidia.com/nsight-compute/

---

## 练习 F4：Compute Sanitizer 与 bug 检测

### 目标

学会用 compute-sanitizer 工具套件的四个检测器（memcheck、racecheck、synccheck、initcheck），抓住那些 Nsight Compute 看不到、运行时才爆的 bug。通过构造故意错误的 kernel，理解每个检测器的工作原理。

### 前置理解

- 你知道共享内存的 bank conflict（虽然不是 bug，但 Nsight 会报）。
- 你理解"data race"的概念（多线程写同一块内存）。
- 你知道 `__syncthreads` 的作用，以及不同步的后果。

### 必做任务

1. 写 4 个故意有 bug 的 kernel，分别触发四类问题：
   - **Kernel A（memcheck）**：越界访问。例如：`global_mem[blockIdx.x * blockDim.x + threadIdx.x + 1000]`，超出分配范围。
   - **Kernel B（racecheck）**：shared memory 数据竞争。例如：两个不同的 warp 写同一个 `__shared__` 内存位置，无同步。
   - **Kernel C（synccheck）**：divergent `__syncthreads`。例如：`if(threadIdx.x < 16) __syncthreads();` 导致某些线程执行，某些不执行。
   - **Kernel D（initcheck）**：未初始化读取。例如：读一个 shared memory 位置，但该位置从未被赋值。
2. 对每个 kernel，分别用四个检测器之一运行（如果程序会立刻 crash，需要加 error handling）：
   - `compute-sanitizer --tool memcheck ./exe_a`
   - `compute-sanitizer --tool racecheck ./exe_b`
   - `compute-sanitizer --tool synccheck ./exe_c`
   - `compute-sanitizer --tool initcheck ./exe_d`
3. 观察每个检测器的报告，记录：
   - 检测到了什么错误。
   - 错误的位置（block/thread ID、内存地址等）。
   - 建议的修复方法。
4. 修复每个 bug，重新运行对应的检测器，验证错误消失。

### 进阶任务

- 构造一个包含多个隐蔽 bug 的 kernel，运行 `compute-sanitizer --tool all` 看是否能全部检测。
- 尝试用 `--debug full` 或其它参数获得更详细的调试信息（可能需要 debug symbols）。
- 在一个对的 kernel 上运行所有四个检测器，观察是否误报（应该没有）。

### 验收点

- 四个 kernel 分别编译通过。
- 每个检测器都成功检测到对应的 bug。
- 修复后，检测器不再报错。
- 记录了每类 bug 的症状和修复方法。

### 观察点

- memcheck 检测越界访问、use-after-free、memory leak 等；是最常见的工具。
- racecheck 检测 shared memory 中的数据竞争；需要开启 SM level 追踪。
- synccheck 检测 `__syncthreads` 和其它同步原语的发散；可以防止死锁。
- initcheck 检测未初始化的读取；对数值稳定性很关键。
- compute-sanitizer 的开销很大（kernel 变慢 10-100 倍），所以通常只在调试时用。

### 常见坑

1. 某些检测器需要特定的 Compute Capability（例如 racecheck 需要 CC 7.0+）；在不支持的硬件上会失败。
2. compute-sanitizer 输出信息量很大，容易遗漏关键错误；需要仔细阅读报告。
3. 关闭了某些优化选项或改变了编译参数后，某些 bug 可能无法检测（compiler 的优化会影响检测准确度）。
4. 误以为检测器的缺席 = bug 不存在；某些隐蔽的 race 可能逃脱检测。
5. 在 memcheck 模式下，某些内存操作可能被标记为"maybe error"而非"definite error"；需要判断是否真的是 bug。

### 提示

- compute-sanitizer 的输出默认到 stderr；可以用 `2>&1` 重定向到文件便于查看。
- 如果 kernel 太快，检测器可能来不及检测；可以加一个 loop 让 kernel 运行多次。
- `--log-level info` 可以增加日志详细程度。

### 复盘问题

1. memcheck 和 racecheck 各检测什么类型的 bug？它们能同时运行吗？
2. synccheck 如何检测 divergent `__syncthreads`？
3. initcheck 如何知道一块 shared memory 没被初始化？
4. compute-sanitizer 的性能开销为什么这么大？

### 对应官方参考

- compute-sanitizer Documentation：https://docs.nvidia.com/cuda/compute-sanitizer/

---

## 练习 F5：Nsight Visual Studio Edition 中的 kernel 断点与寄存器查看

### 目标

学会在 Nsight Visual Studio Edition（VSE）中为 kernel 设置断点、单步执行、以及查看 warp/lane 级的寄存器值。这对深入理解 kernel 执行流程和调试棘手的 bug 非常有用。（如果用 Linux + CUDA-GDB，流程类似但工具不同；本文档以 VSE 为主，Linux 用户参考 CUDA-GDB 文档。）

### 前置理解

- 你用过 Visual Studio 的 CPU 调试功能（设断点、查看变量等）。
- 你理解 warp 和 lane 的概念。
- 你能接受 kernel 在调试模式下会显著变慢。

### 必做任务

1. 在 Visual Studio 中安装 Nsight VSE 插件（通常随 CUDA Toolkit 或单独下载）。
2. 写一个简单的 kernel，包含一些可观察的计算步骤（例如 prefix sum 或简单的 reduce）。
3. 在 kernel 的某个关键行设置 CPU 侧断点（虽然会在 host 端），或在 kernel 代码行设置"GPU 断点"（如果 VSE 支持）。
4. 构建应用（确保 Debug 配置）。在 VSE 中启动调试。
5. 当程序执行到 kernel launch 时，尝试进入 kernel（可能需要特定的快捷键或菜单项）。
6. 进入后，应该能看到当前执行的 warp 和 lane；单步执行几条指令。
7. 在 VSE 的 "Debug" 或 "CUDA Debugging" 窗口中，查看当前 warp 的寄存器值（例如 `$r0`, `$r1` 等）。
8. 也可以查看 shared memory 和全局内存的内容。

### 进阶任务

- 尝试在不同的 warp 间切换，观察它们的寄存器值如何不同。
- 使用条件断点（当某个寄存器值满足条件时停止）来定位特定的数据值或行为。
- 如果用 Linux + CUDA-GDB，类似流程：`cuda-gdb ./exe`，然后 `target cuda device <n>` 选择 GPU，`break` 设置断点，`run` 启动等。

### 验收点

- 能够在 kernel 中设置断点并进入调试。
- 能够查看某个 warp 的寄存器值。
- 能够单步执行并观察寄存器变化。
- 记录了调试过程（可以是文字描述或截图）。

### 观察点

- kernel 调试比 CPU 调试慢得多，因为需要在 GPU 上维护调试状态。
- 一次只能调试一个或一小组 warp（取决于硬件限制），其它 warp 被 halt。
- 寄存器在 lane 级独立存在，所以查看时需要指定 warp/lane。
- shared memory 的内容在调试过程中可以修改，用于测试不同的数据场景。

### 常见坑

1. 没有用 Debug 配置编译，导致无法进行 GPU 级调试（某些优化会导致编译后代码与源代码对应关系丢失）。
2. 在执行大 kernel 时设置了 halt，导致 GPU 被锁死（其它应用无法使用 GPU）；需要小心管理。
3. VSE 的 GPU 调试功能在某些 GPU 上不可用（需要特定的 CC 和驱动版本）；需要提前检查。
4. 混淆了"warp ID"与"block ID"，导致查看错误的寄存器。
5. 某些寄存器名称在不同架构上不同；文档中需要说明基于的架构。

### 提示

- 通常在 VSE 中，有一个 "CUDA Debugging" 工具栏或窗口，里面可以选择要观察的 kernel、block、warp、lane。
- 如果用 CUDA-GDB（Linux），命令类似于 CPU 调试（break、run、next 等），但需要用 `cuda` 前缀的命令切换 CUDA 上下文。
- 对于大型 kernel，建议先用小数据量调试，否则会很慢。

### 复盘问题

1. 为什么 kernel 调试比 CPU 调试慢这么多？
2. 一次调试时只能看一个 warp/lane 的状态，意味着什么？如何调试多个 warp 的交互？
3. 寄存器值在调试过程中能修改吗？有什么应用场景？

### 对应官方参考

- Nsight Visual Studio Edition：https://docs.nvidia.com/nsight-visual-studio-edition/
- CUDA-GDB Debugging Guide：https://docs.nvidia.com/cuda/cuda-gdb/（Linux）

---

## 做完本模块后应达到的水平

至少把下面几句话说顺：

- Nsight Systems 时间线如何关联 CPU 和 GPU 操作，NVTX range 如何帮助对齐应用逻辑。
- Nsight Compute 的 "Speed Of Light" 如何快速判断 kernel 的瓶颈（compute-bound vs memory-bound）。
- roofline 模型如何可视化性能约束，以及如何从 roofline 位置推导优化方向。
- compute-sanitizer 的四类检测器（memcheck、racecheck、synccheck、initcheck）各自检测什么问题。
- `--generate-line-info` 编译选项如何让 Nsight Compute 的 source-SASS 视图能关联到源代码行号。
- 如何用 Nsight VSE 或 CUDA-GDB 在 kernel 中设置断点并查看 warp/lane 级寄存器状态。

你应该能够：

- 用 Nsight Systems 采样一个复杂的 CUDA 应用，从时间线上直观看出并发机会和瓶颈。
- 用 Nsight Compute 分析一个 kernel，给出"内存绑定还是计算绑定"的结论和改进建议。
- 手工绘制一个 roofline 图，把多个 kernel 的性能位置标注出来。
- 用 compute-sanitizer 定位数据竞争、越界访问、同步错误等难以察觉的 bug。
- 在需要时用 kernel 调试器进入 kernel，查看 warp/lane 级的执行状态。

</content>
