# 练习 F1：Nsight Systems 时间线与 NVTX 标注

## 目标

`[F1-T01]` (main.cu:2) 练习 F1：Nsight Systems 时间线与 NVTX 标注。
`[F1-T02]` (main.cu:3) 学习目标：3 阶段 pipeline，每阶段用 `NVTX_RANGE` 包裹；两个 kernel 在不同 stream 启动观察并发重叠；`nsys profile` 采集后在 GUI 查看。
`[F1-T03]` (main.cu:9) 编译命令。
`[F1-T04]` (main.cu:10) 采集命令。
`[F1-T05]` (main.cu:11) 运行命令。

学会用 Nsight Systems 采样一个完整应用，从时间线上直观观察 CPU-GPU 交互、多 stream 并发、kernel 与显存拷贝的重叠。通过 NVTX range 标注，把应用逻辑与底层 GPU 操作相关联。

## 前置理解

- 你知道 kernel launch、显存拷贝等操作都在 stream 上发生。
- 你理解 NVTX（NVIDIA Tools Extension，NVIDIA 工具扩展）是用户级标注 API（Application Programming Interface，应用程序接口）。
- 你能看懂时间线图（横轴时间，纵轴资源）。

## 编译

```bash
cmake --build build --target F1_nsight_systems_timeline
```

## 采集命令

**第一次采集（无标注）**：

```bash
nsys profile -o trace_no_nvtx --stats=true ./F1_nsight_systems_timeline
```

**第二次采集（加 NVTX 标注后）**：

```bash
nsys profile -o trace --stats=true --trace=cuda,nvtx ./F1_nsight_systems_timeline
```

打开生成的 `trace.nsys-rep` 文件（Nsight Systems GUI）。

## 必做任务

`[F1-T06]` (main.cu:23) 常量定义。
`[F1-T07]` (main.cu:24) 256 MB / sizeof(float) = 64M 元素。
`[F1-T08]` (main.cu:25) 256 MB 数据量。
`[F1-T09]` (main.cu:27) kernel 工作量（控制执行时间）。
`[F1-T10]` (main.cu:30) Kernel A：向量加法（Stream 0），简单算术，约 10-50 ms（由 ITERS 控制），arithmetic intensity 约 4 ops / 12 bytes 约 0.33 FLOP/byte（内存密集）。
`[F1-T11]` (main.cu:33) TODO [必做-1] 完成 kernel 主体：每个线程执行 ITERS 次迭代累加。
`[F1-T12]` (main.cu:46) TODO [必做-1] 实现带 ITERS 次迭代的向量加法，人为拉长执行时间。
`[F1-T13]` (main.cu:53) Kernel B：向量缩放（Stream 1，与 Kernel A 并发），单操作，比 Kernel A 更轻量，便于观察时间重叠。
`[F1-T14]` (main.cu:55) TODO [必做-1] 完成 kernel 主体：每个线程将输入乘以 scale。
`[F1-T15]` (main.cu:67) TODO [必做-1] `out[tid] = in[tid] * scale;`。
`[F1-T17]` (main.cu:81) `main` 入口。
`[F1-T22]` (main.cu:124) 阶段 1：HtoD 拷贝（256 MB）。
`[F1-T23]` (main.cu:128) 蓝色。
`[F1-T24]` (main.cu:130) TODO [必做-2] 用 stream0 异步拷贝 `d_a`、`d_b`。
`[F1-T25]` (main.cu:139) 阶段 2A：Kernel A 在 stream0 上启动（compute phase）。
`[F1-T26]` (main.cu:145) 绿色。
`[F1-T27]` (main.cu:147) TODO [必做-2] 传入正确参数，通过 stream0 启动。
`[F1-T28]` (main.cu:149) 不在此处同步，让 Kernel A 与 Kernel B 并发。
`[F1-T29]` (main.cu:153) 阶段 2B：Kernel B 在 stream1 上启动（与 Kernel A 并发）。
`[F1-T30]` (main.cu:159) 橙色。
`[F1-T31]` (main.cu:161) TODO [必做-2] 通过 stream1 启动，scale=2.0f。
`[F1-T33]` (main.cu:174) 阶段 3：DtoH 拷贝结果。
`[F1-T34]` (main.cu:178) 红色。
`[F1-T35]` (main.cu:180) TODO [必做-2] 用 stream0 异步拷贝 `d_c` 回 `h_c`。
`[F1-T36]` (main.cu:189) 阶段 4：CPU 端计算（sleep 模拟）。
`[F1-T37]` (main.cu:193) 黄色。
`[F1-T38]` (main.cu:195) TODO [必做-3] 用简单的 CPU loop（或 `std::this_thread::sleep_for`）模拟 host 工作。
`[F1-T39]` (main.cu:206) TODO [必做-4] 用 `nsys profile` 重新采样，在 Nsight Systems GUI 中确认 NVTX 彩色条纹与 CUDA 操作相对应、Kernel A 与 Kernel B 时间重叠、DtoH 是否与 Kernel B 重叠；将观察结论填写为注释。
`[F1-T40]` (main.cu:213) TODO [必做-5] 截图时间线关键区段，标注各 NVTX range 颜色含义。

1. 写一个应用程序，包含以下步骤：HtoD 拷贝一块 256 MB 数据；启动第一个 kernel（执行时间 10-50 ms）；同时启动第二个 kernel 在不同 stream 上（应该与第一个并发）；DtoH 拷贝结果；CPU 端做一些计算（例如 sleep 或 CPU loop）。
2. 不用任何标注，用 `nsys profile -o trace --stats=true ./exe` 采样程序。
3. 打开生成的 `trace.nsys-rep` 文件（用 Nsight Systems GUI），观察默认时间线。
4. 在代码中用 `NVTX_RANGE_COLOR` 在关键位置插入 NVTX range：`HtoD_256MB`（蓝色）、`Kernel_A_stream0`（绿色）、`Kernel_B_Stream1`（橙色）、`DtoH`（红色）、`CPU_Work`（黄色）。
5. 重新编译运行，再采样。打开时间线，应该能看到彩色的 NVTX range 条纹与 CUDA 操作相对应。
6. 观察并记录：两个 kernel 是否真的并发（应该有时间重叠）、DtoH 是否与某个 kernel 重叠。
7. 导出或截图关键部分的时间线，注解观察结果。

## NVTX range 颜色说明

| Range 名称 | 颜色 | ARGB 值 |
|---|---|---|
| `HtoD_256MB` | 蓝色 | `0xFF4080FF` |
| `Kernel_A_stream0` | 绿色 | `0xFF40FF40` |
| `Kernel_B_Stream1` | 橙色 | `0xFFFF8040` |
| `DtoH` | 红色 | `0xFFFF4040` |
| `CPU_Work` | 黄色 | `0xFFFFFF40` |

## 观察点

`[F1-T16]` (main.cu:73) 辅助：打印带宽利用率。
`[F1-T18]` (main.cu:88) 分配 host / device 内存。
`[F1-T19]` (main.cu:99) 使用 pinned memory 以充分发挥 PCIe 带宽。
`[F1-T20]` (main.cu:114) 初始化 host 数据。
`[F1-T21]` (main.cu:120) 创建两个 stream（stream1 用于 Kernel B 并发）。
`[F1-T32]` (main.cu:170) 等待两个 kernel 完成后再计时。

- 时间线的 CPU track 显示 kernel launch 和其它 host API 调用；GPU track 显示实际执行。
- launch 和实际执行之间可能有延迟（launch latency）。
- NVTX range 帮助把时间线与源代码逻辑对应起来，让分析更容易。
- 两个 stream 上的 kernel 如果没有依赖，应该能看到时间重叠。

## 常见坑

1. 没有链接 NVTX library（`nvToolsExt64_1.lib` 或类似），导致 NVTX 调用无效。
2. NVTX range 嵌套错误，导致时间线显示混乱。
3. 采样率设置过高（默认 1 kHz），导致采样器本身成为瓶颈，虚假放大时间。
4. 忘记 `--stats=true` 参数，错过了统计汇总页面。
5. 在时间线上看到的 kernel 名称是通用的（例如 `kernel_0`），而不是用户定义的名称；这是因为没有 debug info（调试信息）。
6. 用 Nsight Systems GUI 打开 `.nsys-rep` 文件时没有指定正确的 symbol 路径，导致无法解析函数名。

## 进阶任务

`[F1-T41]` (main.cu:214) TODO [进阶-1] 增加第三个 stream，添加 Kernel C，使时间线更复杂。
`[F1-T42]` (main.cu:215) TODO [进阶-2] 用 NVTX domain 功能区分"数据传输"与"计算"两个逻辑模块。
`[F1-T43]` (main.cu:216) TODO [进阶-3] 调整 ITERS 让 Kernel A 更长，对比改变并发度后的时间线变化。

- 增加更多 stream 和 kernel，使时间线变得复杂；观察 Nsight Systems 如何展现这种复杂性。
- 尝试在 kernel 内部用不同的迭代次数改变执行时间，观察时间线如何变化。
- 用 NVTX 的 domain 功能区分不同的逻辑模块（需要调用 `nvtxDomainCreateA`）。

## 验收点

`[F1-T44]` (main.cu:219) 清理。
`[F1-T45]` (main.cu:233) 完成提示。

- 时间线图清晰地显示了 HtoD、两个 kernel、DtoH 等操作。
- NVTX range 标注能够准确地标记应用的关键阶段。
- 你能从时间线上指出哪些操作有时间重叠、哪些是串行的。
- 截图或导出数据证明观察结果。

## 复盘问题

1. 时间线上的 CPU track 和 GPU track 分别代表什么？
2. launch 和实际执行之间的延迟通常是多久？
3. NVTX range 对性能有影响吗？
4. 如何在 Nsight Systems 时间线上确认两个 kernel 真的在并发？

## 对应官方参考

- Nsight Systems Documentation：https://docs.nvidia.com/nsight-systems/
- NVIDIA Tools Extension：https://docs.nvidia.com/gameworks/content/developertools/desktop/analysis/report/nvtx.htm

## 输出对照（printf / std::puts 原文）

- `[F1-T22]` (main.cu:130) 原文：`--- 阶段 1: HtoD 拷贝（256 MB x2）---` -> 现：`--- Stage 1: HtoD copy (256 MB x2) ---`
- `[F1-T25]` (main.cu:144) 原文：`--- 阶段 2A: Kernel A — vector_add（stream0）---` -> 现：`--- Stage 2A: Kernel A -- vector_add (stream0) ---`
- `[F1-T29]` (main.cu:158) 原文：`--- 阶段 2B: Kernel B — vector_scale（stream1，与 A 并发）---` -> 现：`--- Stage 2B: Kernel B -- vector_scale (stream1, concurrent with A) ---`
- `[F1-T33]` (main.cu:177) 原文：`--- 阶段 3: DtoH 拷贝结果（256 MB）---` -> 现：`--- Stage 3: DtoH copy result (256 MB) ---`
- `[F1-T36]` (main.cu:192) 原文：`--- 阶段 4: CPU 计算（模拟 host 工作）---` -> 现：`--- Stage 4: CPU compute (simulating host work) ---`
- 启动信息中文 `启动:` -> `launch:`，`总耗时（含并发）` -> `total elapsed (with concurrency)`，`CPU sum（每64元素）` -> `CPU sum (every 64 elements)`。
