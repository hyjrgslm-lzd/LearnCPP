# 练习 E3：流捕获与 CUDA Graph

## 目标

`[E3-T01]` (main.cu:1) E3_cuda_graph_capture/main.cu。
`[E3-T02]` (main.cu:2) 练习目标：学习 stream capture API 隐式构图，对比逐个 launch 与 graph replay 的开销。

理解 CUDA Graph 的核心价值——消除 kernel launch 开销——并学习两种构建方式：stream capture API（隐式构图）与 explicit graph API（显式构图）。通过对比 launch overhead，感受 graph replay 带来的性能提升。

## 前置理解

- 你知道 stream 的基本用法。
- 你理解 kernel launch 需要 host-device 通信，存在延迟（通常微秒级）。
- 你想象过"如果能批量启动 N 个 kernel 而不用反复调用 CUDA API，会快多少"。

## 必做任务

`[E3-T03]` (main.cu:5) 模块 E 练习 E3 的必做任务清单。
`[E3-T11]` (main.cu:40) TODO [必做-1] 实现 `kernel_saxpy`：`out[idx] = a * x[idx] + y[idx]`。
`[E3-T12]` (main.cu:47) TODO [必做-1] kernel 函数体内实现。
`[E3-T15]` (main.cu:68) TODO [必做-2] 循环 `LAUNCH_ITERS` 次启动 `kernel_saxpy`。
`[E3-T19]` (main.cu:96) TODO [必做-3] `cudaStreamBeginCapture(stream, cudaStreamCaptureModeGlobal)`。
`[E3-T20]` (main.cu:99) TODO [必做-3] 在 capture 区间内循环 `LAUNCH_ITERS` 次 launch `kernel_saxpy`。
`[E3-T21]` (main.cu:105) TODO [必做-3] `cudaStreamEndCapture(stream, &graph)`。
`[E3-T24]` (main.cu:114) TODO [必做-4] `cudaGraphInstantiate(&graphExec, graph, NULL, NULL, 0)`。
`[E3-T26]` (main.cu:120) TODO [必做-5] 循环 `GRAPH_REPLAYS` 次 `cudaGraphLaunch(graphExec, stream)`。
`[E3-T29]` (main.cu:135) TODO [必做-5] `cudaGraphExecDestroy(graphExec)`。
`[E3-T30]` (main.cu:136) TODO [必做-5] `cudaGraphDestroy(graph)`。
`[E3-T36]` (main.cu:163) TODO [必做-1] 初始化 d_x, d_y（例如用 `cudaMemset` 或 fill kernel）。

1. 写一个轻量 kernel（执行时间 < 1 ms），例如简单的元素加法。
2. 在循环中启动该 kernel 1000 次，用 `cudaEventRecord` 计时总耗时。这是 baseline（每次 launch overhead）。
3. 用 `cudaStreamBeginCapture(stream, cudaStreamCaptureModeGlobal)` 开始捕获，在同一条 stream 上启动同一个 kernel 1000 次，再用 `cudaStreamEndCapture(stream, &graph)` 结束捕获，获得 graph 对象。
4. 用 `cudaGraphInstantiate(&graphExec, graph, NULL, NULL, 0)` 编译 graph 为可执行形式。
5. 在循环中用 `cudaGraphLaunch(graphExec, stream)` 重放该 graph 100 次（共执行 100,000 个 kernel 启动），用 event 计时。
6. 对比两个版本的总耗时；预期 graph 版本快 1-3 个数量级（因为消除了 100,000 × launch overhead）。
7. 打印出单个 kernel 的执行时间（应该很短），以及平均 launch overhead 的估计（总耗时 / 启动次数）。

## 进阶任务

`[E3-T04]` (main.cu:13) 进阶任务清单（TODO [进阶]）。
`[E3-T47]` (main.cu:189) TODO [进阶-A] 在 graph 中加入 `cudaMemcpy` 节点，验证 capture 有效。

- 增加 graph 的复杂度：不只是 kernel launch，还包含 `cudaMemcpy` 和依赖关系；观察 capture 是否仍然有效。
- 在 capture 期间故意做一些 host-side 操作（例如日志输出），观察它是否被忽略或导致 capture 失败。
- 尝试在 capture 过程中调用 `cudaStreamSynchronize`；观察是否报错或出现不期望的行为。

## 验收点

`[E3-T05]` (main.cu:28) 常量定义。
`[E3-T06]` (main.cu:30) `N_ELEM = 1 << 20`，1M 元素（轻量 kernel）。
`[E3-T07]` (main.cu:32) `LAUNCH_ITERS = 1000`，逐个 launch 次数。
`[E3-T08]` (main.cu:33) `GRAPH_REPLAYS = 100`，graph 重放次数。
`[E3-T09]` (main.cu:36) Kernel（TODO 区域）。
`[E3-T10]` (main.cu:39) `kernel_saxpy`：轻量 SAXPY，执行时间 < 1 ms。
`[E3-T13]` (main.cu:51) 版本 A：逐个 launch（baseline）。
`[E3-T14]` (main.cu:60) 预热一次。
`[E3-T17]` (main.cu:83) 版本 B：stream capture + graph replay。
`[E3-T18]` (main.cu:91) 步骤 1：capture。
`[E3-T22]` (main.cu:107) 打印 graph 包含的节点数。
`[E3-T23]` (main.cu:111) 步骤 2：instantiate。
`[E3-T25]` (main.cu:115) 步骤 3：replay 100 次。
`[E3-T28]` (main.cu:133) 清理。
`[E3-T31]` (main.cu:141) `main` 入口。
`[E3-T35]` (main.cu:153) 分配 Device 内存。
`[E3-T37]` (main.cu:164) 创建 stream（非默认，避免 legacy default stream 干扰）。
`[E3-T38]` (main.cu:168) 运行版本 A。
`[E3-T39]` (main.cu:171) 运行版本 B。
`[E3-T40]` (main.cu:174) 总结对比。
`[E3-T41]` (main.cu:175) 逐个 launch 版：`LAUNCH_ITERS` 个 kernel。
`[E3-T42]` (main.cu:176) graph 版：`GRAPH_REPLAYS * LAUNCH_ITERS` 个 kernel（全量启动成本摊薄）。
`[E3-T43]` (main.cu:177) 以每 `LAUNCH_ITERS` 个 kernel 为单位对比。
`[E3-T48]` (main.cu:191) 清理。

- 你能用秒表数据证明：graph replay 版本比逐个 launch 版本快至少 10 倍。
- `cudaStreamEndCapture` 成功返回 `cudaSuccess`。
- `cudaGraphLaunch` 执行正确，结果与逐个 launch 版本一致（可以验证最终输出）。
- 你能估算出单个 launch 的平均开销（以微秒计）。

## 观察点

- graph capture 的核心是"记录一系列 GPU 操作"，而不真的执行它们；执行延迟到 `cudaGraphLaunch` 时。
- `cudaStreamBeginCapture` 必须与 `cudaStreamEndCapture` 配对，中间不能有 host-side 同步操作（如 `cudaStreamSynchronize`）。
- 一旦 graph 被 instantiate，就变成一个不可变的对象 `cudaGraphExec_t`；每次 launch 都复用相同的图结构。
- graph 的威力不在单个 launch，而在"批量重放"——你花一次捕获成本，换来千次快速 replay。

## 常见坑

1. 在 capture 期间调用 `cudaStreamSynchronize`，导致 capture 失败或行为不对。
2. 忘记 `cudaGraphInstantiate` 直接用 graph 对象 launch；这不会编译失败，但运行会出错。
3. 修改了 kernel 的参数（例如 block size 或输入数据指针）后没有重新 capture 或 instantiate，还期望新参数生效。
4. graph 中包含了 host callback，导致 graph 无法高效重放（host callback 会打破 GPU 管道）。
5. 创建 graph 后没有 `cudaGraphDestroy`。
6. 在 graph 执行期间修改 graph 的内容（如果你存了 graph 对象的引用），导致数据竞争。
7. 以为 capture 一定比手工逐个 launch 更快；实际要取决于 kernel 执行时间和 launch 数量比例。
8. 在 capture 模式下用了不支持 capture 的 API（例如某些 cuBLAS 调用），导致 capture 失败。

## 提示

- `cudaStreamCaptureModeGlobal` 会捕获该 stream 及所有使用了 default stream 的操作；`cudaStreamCaptureModeThreadLocal` 只捕获该 thread 的操作。
- 如果 capture 过程中 kernel 返回了错误，`cudaStreamEndCapture` 会返回非零值；检查返回值很重要。
- 可以用 `cudaGraphGetNodes` 和 `cudaGraphNodeGetType` 来检查生成的 graph 包含了哪些节点。

## 复盘问题

1. 为什么 CUDA Graph 能显著降低 launch overhead？
2. 如果你的 kernel 每次执行时间都是 100 ms，capture 和 graph launch 还有优势吗？
3. `cudaStreamCaptureModeGlobal` 和 `cudaStreamCaptureModeThreadLocal` 的适用场景分别是什么？
4. 一旦 instantiate，graph 就不能修改了吗？怎样在运行时更新 kernel 参数？

## 对应官方参考

- CUDA C++ Programming Guide：[CUDA Graphs](https://docs.nvidia.com/cuda/cuda-c-programming-guide/#cuda-graphs)
- CUDA Runtime API：`cudaStreamBeginCapture`, `cudaStreamEndCapture`, `cudaGraphInstantiate`, `cudaGraphLaunch`

## 输出对照（printf / std::puts 原文）

- `[E3-T16]` (main.cu:75) 原文：`[逐个 launch] %d 次总耗时 = %.3f ms, 平均 = %.3f us/launch` -> 现：`[per-launch] %d total = %.3f ms, avg = %.3f us/launch`
- `[E3-T27]` (main.cu:129) 原文：`[graph replay] %d 次 replay（共 %d 个 kernel）总耗时 = %.3f ms, 平均 = %.3f us/kernel` -> 现：`[graph replay] %d replays (total %d kernels) = %.3f ms, avg = %.3f us/kernel`
- `[E3-T32]` (main.cu:149) 原文：`=== E3: 流捕获与 CUDA Graph ===` -> 现：`=== E3: stream capture and CUDA Graph ===`
- `[E3-T33]` (main.cu:150) 原文：`N_ELEM = %d, BLOCK = %d` -> 现：保持英文不变
- `[E3-T34]` (main.cu:151) 原文：`逐个 launch 次数 = %d, graph replay 次数 = %d` -> 现：`per-launch count = %d, graph replays = %d`
- `[E3-T44]` (main.cu:180) 原文：`逐个 launch（%d 核）  : %.3f ms` -> 现：`per-launch (%d kernels)         : %.3f ms`
- `[E3-T45]` (main.cu:182) 原文：`graph replay（每批 %d 核）: %.3f ms` -> 现：`graph replay (per batch %d kernels): %.3f ms`
- `[E3-T46]` (main.cu:185) 原文：`加速比 : %.1fx` -> 现：`speedup : %.1fx`
- `[E3-T49]` (main.cu:196) 原文：`提示：用 'nsys profile ...' 采集` -> 现：`hint: capture with 'nsys profile ...'`
- `[E3-T50]` (main.cu:197) 原文：`在时间线上对比逐个 launch 与 graph replay 的 launch 密度` -> 现：`compare per-launch vs graph replay launch density on the timeline`
