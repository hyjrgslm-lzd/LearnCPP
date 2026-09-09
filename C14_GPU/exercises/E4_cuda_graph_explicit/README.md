# 练习 E4：显式图 API 与动态参数更新

## 目标

`[E4-T01]` (main.cu:1) E4_cuda_graph_explicit/main.cu。
`[E4-T02]` (main.cu:2) 练习目标：学习 explicit graph API 构建 DAG；用 `cudaGraphExecKernelNodeSetParams` 在运行时动态更新 kernel 参数，无需重新 capture 或 instantiate。

学习 explicit graph API 的方式构建 DAG，以及如何用 `cudaGraphExecKernelNodeSetParams` 在运行时修改 kernel 参数，而无需重新 capture 或 instantiate。这种动态更新对迭代算法（例如神经网络推理循环）非常有用。

## 前置理解

- 你已经完成了 E3，理解了 CUDA Graph 的基本概念。
- 你知道如何用 `cudaGraphAddKernelNode` 手工添加 kernel 节点。
- 你理解 DAG（有向无环图）的依赖关系。

## 必做任务

`[E4-T03]` (main.cu:5) 模块 E 练习 E4 的必做任务清单。
`[E4-T11]` (main.cu:40) TODO [必做-2] 实现 `kernel_A`：`out[idx] = in[idx] * in[idx] + bias`。
`[E4-T12]` (main.cu:47) TODO [必做-2] `kernel_A` 函数体内实现。
`[E4-T14]` (main.cu:51) TODO [必做-2] 实现 `kernel_B`：每个 block 对其负责的元素求和，写入 `partial_sums[blockIdx.x]`。
`[E4-T15]` (main.cu:56) TODO [必做-2] `kernel_B` 函数体内实现（可用 shared memory reduce）。
`[E4-T17]` (main.cu:60) TODO [必做-2] 实现 `kernel_C`：计算 `partial_sums` 的总和，写入 `result[0]`。
`[E4-T18]` (main.cu:67) TODO [必做-2] `kernel_C` 函数体内实现（单线程或小 block 归并）。
`[E4-T20]` (main.cu:93) TODO [必做-1] `cudaGraphCreate(&gb.graph, 0)`。
`[E4-T22]` (main.cu:97) TODO [必做-3] 填写 `gb.paramsA`。
`[E4-T23]` (main.cu:107) TODO [必做-3] `cudaGraphAddKernelNode(&gb.nodeA, gb.graph, nullptr, 0, &gb.paramsA)`。
`[E4-T25]` (main.cu:112) TODO [必做-3] 填写 `gb.paramsB`，`cudaGraphAddKernelNode(&gb.nodeB, ...)`。
`[E4-T27]` (main.cu:118) TODO [必做-3] 填写 `gb.paramsC`，`cudaGraphAddKernelNode(&gb.nodeC, ...)`。
`[E4-T29]` (main.cu:121) TODO [必做-4] `cudaGraphAddDependencies(gb.graph, &gb.nodeA, &gb.nodeB, 1)`。
`[E4-T30]` (main.cu:122) TODO [必做-4] `cudaGraphAddDependencies(gb.graph, &gb.nodeB, &gb.nodeC, 1)`。
`[E4-T32]` (main.cu:125) TODO [必做-5] `cudaGraphInstantiate(&gb.graphExec, gb.graph, NULL, NULL, 0)`。
`[E4-T36]` (main.cu:147) TODO [必做-6] 修改 `paramsA.kernelParams[0]` 指向 `d_inputs[iter]`。
`[E4-T37]` (main.cu:150) TODO [必做-6] `cudaGraphExecKernelNodeSetParams(gb.graphExec, gb.nodeA, &gb.paramsA)`。
`[E4-T38]` (main.cu:152) TODO [必做-6] `cudaGraphLaunch(gb.graphExec, stream)`。
`[E4-T39]` (main.cu:154) TODO [必做-6] 等待并验证 `d_result[0]`（可选，用 `cudaMemcpyAsync`）。
`[E4-T43]` (main.cu:191) TODO [必做-7] `cudaStreamBeginCapture(stream, cudaStreamCaptureModeGlobal)`。
`[E4-T44]` (main.cu:195) TODO [必做-7] `cudaStreamEndCapture(stream, &g)`。
`[E4-T45]` (main.cu:196) TODO [必做-7] `cudaGraphInstantiate(&exec, g, NULL, NULL, 0)`。
`[E4-T46]` (main.cu:197) TODO [必做-7] `cudaGraphLaunch(exec, stream)`。
`[E4-T47]` (main.cu:198) TODO [必做-7] 清理：`cudaGraphExecDestroy` / `cudaGraphDestroy`。
`[E4-T54]` (main.cu:239) TODO [必做-2] 用不同值初始化 `d_inputs[i]`（例如 `cudaMemset` 或 fill kernel）。
`[E4-T64]` (main.cu:265) TODO 释放 `gb.graphExec`, `gb.graph`。

1. 用 `cudaGraphCreate(&graph, 0)` 创建一个空 graph。
2. 定义三个 kernel：A（读 input 做变换）、B（对 A 的结果做约化）、C（归一化或输出）。
3. 用 `cudaGraphAddKernelNode` 分别添加三个 kernel 节点，指定参数。
4. 用 `cudaGraphAddDependencies` 建立依赖关系：B 依赖 A，C 依赖 B。
5. 用 `cudaGraphInstantiate` 编译该 graph。
6. 在循环中（例如 10 次迭代）：
   - 更新 kernel A 的输入指针（指向不同的数据）。
   - 调用 `cudaGraphExecKernelNodeSetParams(graphExec, nodeA, &newParams)` 更新 nodeA 的参数。
   - 用 `cudaGraphLaunch` 执行 graph。
   - 验证输出正确。
7. 对比"每次迭代重新 capture"与"动态参数更新"的耗时。

## 进阶任务

`[E4-T04]` (main.cu:14) 进阶任务清单（TODO [进阶]）。
`[E4-T62]` (main.cu:262) TODO [进阶-A] `cudaGraphAddMemcpyNode`：在 nodeC 后添加 DtoH 拷贝节点。

- 在 graph 中添加 `cudaGraphAddMemcpyNode` 节点，例如在 kernel C 执行后拷贝结果回 host。
- 用 `cudaGraphAddEmptyNode` 创建一个空节点作为同步点，再让其它节点依赖它。
- 尝试删除或修改已存在的节点（需要用 `cudaGraphRemoveNode` 或重建 graph）；观察 graph 结构如何变化。

## 验收点

`[E4-T05]` (main.cu:29) 常量定义。
`[E4-T06]` (main.cu:30) `N_ELEM = 1 << 22`，4M 元素。
`[E4-T07]` (main.cu:32) `LOOP_ITERS = 10`，动态参数更新迭代次数。
`[E4-T08]` (main.cu:33) `N_DATASETS = LOOP_ITERS`，每次迭代切换不同 input 数组。
`[E4-T09]` (main.cu:37) Kernel 声明（TODO 区域）。
`[E4-T10]` (main.cu:39) `kernel_A`：读 input，输出 intermediate（例如逐元素平方 + bias）。
`[E4-T13]` (main.cu:50) `kernel_B`：对 A 的输出做 block-level reduce（存入 `partial_sums`）。
`[E4-T16]` (main.cu:59) `kernel_C`：对 `partial_sums` 做最终归并并归一化输出。
`[E4-T19]` (main.cu:71) 构建 explicit graph（一次性，topology 固定）。
`[E4-T21]` (main.cu:94) 设置 `kernel_A` 参数结构体。
`[E4-T24]` (main.cu:108) 设置 `kernel_B` 参数结构体。
`[E4-T26]` (main.cu:114) 设置 `kernel_C` 参数结构体。
`[E4-T28]` (main.cu:120) 建立依赖：B 依赖 A，C 依赖 B。
`[E4-T31]` (main.cu:124) Instantiate。
`[E4-T33]` (main.cu:131) 动态参数更新循环。
`[E4-T34]` (main.cu:135) `N_DATASETS` 个输入数组。
`[E4-T41]` (main.cu:168) 对比：每次迭代重新 capture（慢）。
`[E4-T49]` (main.cu:215) `main` 入口。
`[E4-T52]` (main.cu:226) 分配 Device 内存。
`[E4-T53]` (main.cu:235) 为每次迭代分配独立的输入数组（模拟不同批次数据）。
`[E4-T55]` (main.cu:245) 构建 explicit graph。
`[E4-T56]` (main.cu:248) 运行动态更新版本。
`[E4-T57]` (main.cu:251) 运行每次 capture 版本。
`[E4-T58]` (main.cu:254) 对比。
`[E4-T63]` (main.cu:264) 清理。

- 三个 kernel 按照指定的依赖顺序执行（可以用 `__threadfence` 或原子操作验证执行顺序）。
- 动态参数更新成功，新数据被正确处理。
- 循环 10 次迭代，输出都正确无误。
- 能给出动态更新相比重新 capture 的性能改进数据（预期改进较小但存在）。

## 观察点

- explicit graph API 给你完全的 DAG 控制权——你可以精确指定节点和依赖，而不依赖 capture 的启发式推断。
- `cudaGraphExecKernelNodeSetParams` 的成本远低于重新 capture + instantiate。
- 即使参数变了，graph 的拓扑结构（节点和边）保持不变，只有参数值被更新。

## 常见坑

1. 忘记 `cudaGraphInstantiate` 直接修改 graph 对象，导致操作无效。
2. `cudaGraphExecKernelNodeSetParams` 时，参数结构体的内存布局必须与原始参数一致，否则数据混乱。
3. 修改了节点参数但没有 `cudaStreamSynchronize` 等待前一次 launch 完成，导致数据竞争。
4. 试图在 instantiate 后修改 graph 的拓扑（添加删除节点），这是不允许的；必须修改 graph 再重新 instantiate。
5. `cudaGraphRemoveNode` 后，依赖该节点的其它节点仍然保留（成为悬空依赖），导致行为不对。
6. 创建了大量 graph 对象但没有及时 `cudaGraphDestroy`。
7. 在 `cudaGraphAddDependencies` 时顺序搞反，例如应该是 B 依赖 A，结果写反了。

## 提示

- `cudaGraphAddKernelNode` 返回一个 node handle；保存这些 handle 以便后续修改。
- 参数结构体通常是 `cudaKernelNodeParams`；确保 kernel function pointer、grid size、block size、shared memory、args array 都设置正确。
- 如果 kernel 参数很多，考虑用一个结构体打包它们，再传指针，这样参数更新时更方便。

## 复盘问题

1. explicit graph API 相比 stream capture 的优势和劣势各是什么？
2. 为什么 `cudaGraphExecKernelNodeSetParams` 允许修改参数但不允许修改节点拓扑？
3. 如果你用 explicit API 构建一个 100 节点的 graph，每次都更新全部参数，性能会怎样？

## 对应官方参考

- CUDA C++ Programming Guide：[Creating a Graph Explicitly](https://docs.nvidia.com/cuda/cuda-c-programming-guide/#creating-a-graph-explicitly)
- CUDA Runtime API：`cudaGraphCreate`, `cudaGraphAddKernelNode`, `cudaGraphAddDependencies`, `cudaGraphExecKernelNodeSetParams`

## 输出对照（printf / std::puts 原文）

- `[E4-T35]` (main.cu:140) 原文：`--- 动态参数更新（%d 次迭代）---` -> 现：`--- dynamic param update (%d iters) ---`
- `[E4-T40]` (main.cu:161) 原文：`动态更新总耗时 = %.3f ms, 平均 = %.3f ms/iter` -> 现：`dynamic update total = %.3f ms, avg = %.3f ms/iter`
- `[E4-T42]` (main.cu:178) 原文：`--- 每次重新 capture（%d 次迭代）---` -> 现：`--- re-capture every iter (%d iters) ---`
- `[E4-T48]` (main.cu:207) 原文：`重新 capture 总耗时 = %.3f ms, 平均 = %.3f ms/iter` -> 现：`recapture total = %.3f ms, avg = %.3f ms/iter`
- `[E4-T50]` (main.cu:220) 原文：`=== E4: 显式 Graph API 与动态参数更新 ===` -> 现：`=== E4: explicit Graph API and dynamic param update ===`
- `[E4-T51]` (main.cu:222) 原文：`N_ELEM = %d, grid = %d, LOOP_ITERS = %d` -> 现：保持英文不变
- `[E4-T59]` (main.cu:255) 原文：`动态参数更新 : %.3f ms total` -> 现：`dynamic param update : %.3f ms total`
- `[E4-T60]` (main.cu:257) 原文：`每次 capture : %.3f ms total` -> 现：`recapture each iter  : %.3f ms total`
- `[E4-T61]` (main.cu:259) 原文：`capture 开销倍数 : %.2fx` -> 现：`capture overhead ratio : %.2fx`
