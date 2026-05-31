# 练习 E1：流与事件的并发语义

## 目标

`[E1-T01]` (main.cu:1) E1_streams_and_events/main.cu。
`[E1-T02]` (main.cu:2) 练习目标：理解 `cudaStream_t` 的并发语义、`cudaEventRecord` / `cudaStreamWaitEvent`，通过两个版本（默认 stream vs 多 stream）对比耗时并用 Nsight Systems 验证。

理解 `cudaStream_t` 的作用、默认 stream 的同步陷阱、以及如何用 `cudaEventRecord` / `cudaStreamWaitEvent` 构造 kernel 间的依赖关系。通过 Nsight Systems 时间线观察两个 stream 上的 kernel 与 DtoH 拷贝是否真的并发执行。

## 前置理解

- 你知道 CUDA kernel 默认在 stream 0（legacy default stream）上启动。
- 你知道 `cudaEventRecord` 在某条 stream 上记录一个事件，`cudaStreamWaitEvent` 让另一条 stream 等待该事件。
- 你理解"同步"和"并发"对应的硬件行为差异。

## 必做任务

`[E1-T03]` (main.cu:6) 模块 E 练习 E1 的必做任务清单。
`[E1-T10]` (main.cu:43) TODO [必做-2] 实现 `kernel_heavy`：每个线程遍历一段数据，做 INNER_LOOP 次累加/原子操作。
`[E1-T11]` (main.cu:50) TODO [必做-2] kernel 内的 idx 与循环骨架。
`[E1-T13]` (main.cu:60) TODO [必做-4] 实现 `kernel_transform`：简单元素乘以 2.0f。
`[E1-T14]` (main.cu:65) TODO [必做-4] kernel 函数体内实现。
`[E1-T17]` (main.cu:81) TODO [必做-3] 在默认 stream 上依次执行 HtoD / kernel_heavy / kernel_transform / DtoH 四步。
`[E1-T18]` (main.cu:86) TODO [必做-3] 四步均在默认 stream，彼此严格串行。
`[E1-T21]` (main.cu:100) TODO [必做-4] 创建两条非默认 stream（`cudaStreamCreate`）。
`[E1-T22]` (main.cu:105) TODO [必做-4] 创建 event 用于依赖建立（`cudaEventCreate`）。
`[E1-T23]` (main.cu:115) TODO [必做-4] stream A 上：HtoD -> evHtoDDone -> kernel_heavy -> evKernelADone。
`[E1-T24]` (main.cu:121) TODO [必做-4] stream B 上：等 evHtoDDone -> kernel_transform -> DtoH。
`[E1-T25]` (main.cu:127) TODO [必做-5] `cudaDeviceSynchronize()` 等待全部完成。
`[E1-T27]` (main.cu:135) TODO 清理：`cudaEventDestroy` / `cudaStreamDestroy`。
`[E1-T33]` (main.cu:158) TODO [必做-1] `cudaMallocHost` / `malloc` 分配 pinned host 内存。
`[E1-T34]` (main.cu:160) TODO [必做-1] 初始化 h_in（例如 `h_in[i] = (float)i * 0.001f`）。
`[E1-T46]` (main.cu:201) TODO 释放所有内存。

1. 分配 host 端和 device 端两块内存，大小建议 >= 256 MB。
2. 写一个 kernel，内部用 `__syncthreads()` 和有意义的计算（例如递归 reduce）让执行时间足够长（目标 5-50 ms）。
3. 在默认 stream 上连续启动两个 kernel，再做一次 HtoD 拷贝、一个 DtoH 拷贝。用 `cudaDeviceSynchronize` 等待全部完成，记录总耗时。
4. 写另一个版本：创建两条非默认 stream（`cudaStreamCreate`），在 stream 0 上启动两个 kernel，在 stream 1 上做 HtoD 拷贝并启动一个不同的 kernel。用 `cudaEventRecord` 记录关键点，用 `cudaStreamWaitEvent` 在合适位置建立依赖（例如 kernel 1 启动前需要 HtoD 拷贝完成）。最后 `cudaDeviceSynchronize`，记录总耗时。
5. 对比两个版本的耗时；如果两条 stream 真的并发了，第二个版本应该更快。
6. 用 Nsight Systems 采样两个版本：`nsys profile -o trace --stats=true ./exe`。在 GUI 中打开 `.qdrep` 文件，观察时间线上是否看到两个 kernel 或 DtoH 拷贝并行执行。

## 进阶任务

`[E1-T04]` (main.cu:14) 进阶任务清单（TODO [进阶]）。
`[E1-T43]` (main.cu:191) TODO [进阶-A] 第三条 stream，三路 kernel 并发。
`[E1-T44]` (main.cu:193) TODO [进阶-C] 手工任务图：kernel A || kernel B -> kernel C -> kernel D。

- 增加第三条 stream，让三个 kernel 分别在三条 stream 上启动（无依赖），观察它们是否真的三路并发（受 SM 数量限制）。
- 用 `cudaStreamCreate` 时指定 `cudaStreamNonBlocking` 参数；观察它对调度延迟的影响（可能不明显，但在微秒级计时时有差异）。
- 手工编写一个简单的任务图：kernel A 和 B 并行，之后 C 依赖 A/B 完成、再启动 D。用 events 和 `cudaStreamWaitEvent` 手工编排，而不用 CUDA Graph。

## 验收点

`[E1-T05]` (main.cu:32) 常量定义。
`[E1-T06]` (main.cu:34) `INNER_LOOP` 控制 kernel 执行时间。
`[E1-T07]` (main.cu:38) Kernel 声明（TODO 区域）。
`[E1-T08]` (main.cu:40) `kernel_heavy`：模拟耗时 kernel，内部 reduce 大数组。
`[E1-T09]` (main.cu:41) 目标执行时间 5-50 ms（通过 INNER_LOOP 调节）。
`[E1-T12]` (main.cu:54) `kernel_transform`：轻量变换 kernel（在 stream B 上与 kernel_heavy 并发）。
`[E1-T15]` (main.cu:69) 版本 A：默认 stream（全串行）。
`[E1-T16]` (main.cu:74) `stream 0 = 默认 stream`。
`[E1-T20]` (main.cu:93) 版本 B：多 stream（kernel + DtoH 并发）。
`[E1-T28]` (main.cu:147) `main` 入口。
`[E1-T32]` (main.cu:155) 分配 Host 内存（pinned，允许异步拷贝）。
`[E1-T35]` (main.cu:166) 分配 Device 内存。
`[E1-T36]` (main.cu:174) 运行两个版本。
`[E1-T37]` (main.cu:178) 对比结果。
`[E1-T45]` (main.cu:198) 清理。

- 你能用数据证明：默认 stream 版本中，HtoD 与 kernel 无法并发；多 stream 版本中，它们可以并发。
- Nsight Systems 时间线清晰地显示了两个版本的差异（可以截图或导出数据）。
- 你能指出每个 event 在代码中的记录位置，以及每个 `cudaStreamWaitEvent` 建立的依赖理由。
- 如果你加了第三条 stream，能说明为什么三个 kernel 不一定会三路并发（可能受限于 SM 数量或其它资源）。

## 观察点

- event 的核心价值不是"计时"，而是"在多 stream 间建立显式的数据依赖"。
- `cudaStreamWaitEvent` 让一条 stream 暂停，直到另一条 stream 上的某个事件完成；这是 stream 间同步的关键原语。
- 默认 stream（stream 0）在 Legacy 模式下会与所有其它 stream 隐式同步，导致不期望的串行化；这是为什么生产代码必须用非默认 stream。
- Nsight Systems 时间线上，kernel execution、memcpy 等操作各占一行，清晰显示哪些重叠、哪些串行。

## 常见坑

1. 没有读 Compute Capability，误以为 stream 功能在 CC 6.0 及以下可用（实际需要 CC 7.0+）。
2. 创建 stream 后没有 `cudaStreamDestroy`，导致内存泄漏。
3. 在默认 stream 上做操作，还期望看到并发；这样的代码永远是串行的。
4. event 用完没有 `cudaEventDestroy`。
5. 混淆 `cudaStreamSynchronize`（等待某条 stream）与 `cudaDeviceSynchronize`（等待全部 stream）。
6. `cudaStreamWaitEvent` 的语义反向理解：应该是"stream A 等 stream B 的 event"，而不是反过来。
7. Nsight Systems 采样率过高（默认 1 kHz）时，host 端会被采样器拖累，造成虚假的延迟膨胀；建议用 `--sample none` 或降低采样率。
8. 以为 stream 创建后自动非阻塞；实际需要显式指定 `cudaStreamNonBlocking`。

## 提示

- 用 `cudaEventCreate` 时不需指定参数；`cudaEventRecord` 时指定 stream。
- 用 `cudaStreamWaitEvent` 时，第三个参数（flags）通常传 0。
- kernel 执行时间可以用内部的计算密集循环保证，例如 `for(int i=0; i<1000; ++i) atomicAdd(...)`（会比较慢）。
- 更好的办法是用一个轻量的 reduce kernel 处理大数据量，自然执行时间就长了。

## 复盘问题

1. 为什么说默认 stream 是"legacy"的，而新代码应该用非默认 stream？
2. 如果你启动了 3 条 stream，但 GPU 只有 80 个 SM 且每个 SM 最多 2 个 warp block，那三条 stream 上的 kernel 还会并发吗？
3. `cudaStreamWaitEvent(streamA, eventB, 0)` 的精确含义是什么？谁在等谁？
4. 为什么 HtoD 拷贝可以和 kernel 并发，但两个 kernel 之间通常需要显式依赖？

## 对应官方参考

- CUDA C++ Programming Guide：[Streams](https://docs.nvidia.com/cuda/cuda-c-programming-guide/#streams)
- CUDA Runtime API：`cudaStreamCreate`, `cudaEventRecord`, `cudaStreamWaitEvent`
- CUDA Best Practices：Stream 的设计考量

## 输出对照（printf / std::puts 原文）

- `[E1-T19]` (main.cu:90) 原文：`[默认 stream] 总耗时 = %.3f ms` -> 现：`[default stream] total = %.3f ms`
- `[E1-T26]` (main.cu:138) 原文：`[多 stream] 总耗时 = %.3f ms` -> 现：`[multi stream] total = %.3f ms`
- `[E1-T29]` (main.cu:156) 原文：`=== E1: 流与事件并发语义 ===` -> 现：`=== E1: stream and event concurrency ===`
- `[E1-T30]` (main.cu:158) 原文：`数组大小 = %d 元素 (%.0f MB)` -> 现：`array size = %d elems (%.0f MB)`
- `[E1-T31]` (main.cu:161) 原文：`grid = %d, block = %d, inner_loop = %d` -> 现：保持英文不变
- `[E1-T38]` (main.cu:182) 原文：`默认 stream : %.3f ms` -> 现：`default stream : %.3f ms`
- `[E1-T39]` (main.cu:184) 原文：`多  stream  : %.3f ms` -> 现：`multi  stream  : %.3f ms`
- `[E1-T40]` (main.cu:187) 原文：`加速比      : %.2fx` -> 现：`speedup        : %.2fx`
- `[E1-T41]` (main.cu:191) 原文：`提示：用 'nsys profile ...' 采集` -> 现：`hint: capture with 'nsys profile ...'`
- `[E1-T42]` (main.cu:193) 原文：`打开 Nsight Systems GUI 查看时间线上的并发重叠` -> 现：`then open the Nsight Systems GUI to see concurrency overlap on the timeline`
