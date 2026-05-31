# 06 模块 E：流、事件与 CUDA Graph

## 模块目标

这个模块要把你从"单条执行链路"推进到"多流并发"与"图级复用"。

你会学到：为什么默认 stream 会意外同步、为什么异步内存分配比 `cudaMalloc` 更高效、为什么 CUDA Graph 能显著降低 host-device 开销。更重要的是，你会通过 Nsight Systems 亲眼看到多条 stream 上的 kernel 和显存拷贝如何并发执行，以及 CUDA Graph replay 如何消除频繁 launch 的累积延迟。

## 前置知识

- 模块 A–D 全部完成：kernel 编写、内存读写、线程同步、warp 原语。
- 理解 `cudaMemcpy` 与 `cudaMemcpyAsync` 的区别。
- 知道 `cudaStreamSynchronize` 和 `cudaDeviceSynchronize` 的作用。

## 模块完成标准

做完本模块，你至少要能说清楚：

- 什么是默认 stream，为什么它会导致不期望的全局同步。
- stream capture API 与 explicit graph API 分别的适用场景。
- 异步内存分配（`cudaMallocAsync` + 内存池）相比 `cudaMalloc` 的延迟优势来自哪里。
- 如何用 CUDA Graph 把多个 kernel 与内存操作打包，并通过 `cudaGraphExecKernelNodeSetParams` 在运行时动态调整参数。
- 什么是 Multi-Process Service（MPS），在什么场景下开启它。

## 硬件与工具链要求

- **最低 Compute Capability**：CC 7.0（Volta，支持 `cudaStreamBeginCapture`）；CC 9.0+ 时 CUDA Graph 可与 TMA 异步拷贝结合（进阶）。
- **CUDA Toolkit**：13.x。
- **样例硬件**：Hopper sm_90a 为主；Ampere/Ada 回退对比。

---

## 练习 E1：流与事件的并发语义

### 目标

理解 `cudaStream_t` 的作用、默认 stream 的同步陷阱、以及如何用 `cudaEventRecord` / `cudaStreamWaitEvent` 构造 kernel 间的依赖关系。通过 Nsight Systems 时间线观察两个 stream 上的 kernel 与 DtoH 拷贝是否真的并发执行。

### 前置理解

- 你知道 CUDA kernel 默认在 stream 0（legacy default stream）上启动。
- 你知道 `cudaEventRecord` 在某条 stream 上记录一个事件，`cudaStreamWaitEvent` 让另一条 stream 等待该事件。
- 你理解"同步"和"并发"对应的硬件行为差异。

### 必做任务

1. 分配 host 端和 device 端两块内存，大小建议 ≥ 256 MB。
2. 写一个 kernel，内部用 `__syncthreads()` 和有意义的计算（例如递归 reduce）让执行时间足够长（目标 5-50 ms）。
3. 在默认 stream 上连续启动两个 kernel，再做一次 HtoD 拷贝、一个 DtoH 拷贝。用 `cudaDeviceSynchronize` 等待全部完成，记录总耗时。
4. 写另一个版本：创建两条非默认 stream（`cudaStreamCreate`），在 stream 0 上启动两个 kernel，在 stream 1 上做 HtoD 拷贝并启动一个不同的 kernel。用 `cudaEventRecord` 记录关键点，用 `cudaStreamWaitEvent` 在合适位置建立依赖（例如 kernel 1 启动前需要 HtoD 拷贝完成）。最后 `cudaDeviceSynchronize`，记录总耗时。
5. 对比两个版本的耗时；如果两条 stream 真的并发了，第二个版本应该更快。
6. 用 Nsight Systems 采样两个版本：`nsys profile -o trace --stats=true ./exe`。在 GUI 中打开 `.qdrep` 文件，观察时间线上是否看到两个 kernel 或 DtoH 拷贝并行执行。

### 进阶任务

- 增加第三条 stream，让三个 kernel 分别在三条 stream 上启动（无依赖），观察它们是否真的三路并发（受 SM 数量限制）。
- 用 `cudaStreamCreate` 时指定 `cudaStreamNonBlocking` 参数；观察它对调度延迟的影响（可能不明显，但在微秒级计时时有差异）。
- 手工编写一个简单的任务图：kernel A 和 B 并行，之后 C 依赖 A/B 完成、再启动 D。用 events 和 `cudaStreamWaitEvent` 手工编排，而不用 CUDA Graph。

### 验收点

- 你能用数据证明：默认 stream 版本中，HtoD 与 kernel 无法并发；多 stream 版本中，它们可以并发。
- Nsight Systems 时间线清晰地显示了两个版本的差异（可以截图或导出数据）。
- 你能指出每个 event 在代码中的记录位置，以及每个 `cudaStreamWaitEvent` 建立的依赖理由。
- 如果你加了第三条 stream，能说明为什么三个 kernel 不一定会三路并发（可能受限于 SM 数量或其它资源）。

### 观察点

- event 的核心价值不是"计时"，而是"在多 stream 间建立显式的数据依赖"。
- `cudaStreamWaitEvent` 让一条 stream 暂停，直到另一条 stream 上的某个事件完成；这是 stream 间同步的关键原语。
- 默认 stream（stream 0）在 Legacy 模式下会与所有其它 stream 隐式同步，导致不期望的串行化；这是为什么生产代码必须用非默认 stream。
- Nsight Systems 时间线上，kernel execution、memcpy 等操作各占一行，清晰显示哪些重叠、哪些串行。

### 常见坑

1. 没有读 Compute Capability，误以为 stream 功能在 CC 6.0 及以下可用（实际需要 CC 7.0+）。
2. 创建 stream 后没有 `cudaStreamDestroy`，导致内存泄漏。
3. 在默认 stream 上做操作，还期望看到并发；这样的代码永远是串行的。
4. event 用完没有 `cudaEventDestroy`。
5. 混淆 `cudaStreamSynchronize`（等待某条 stream）与 `cudaDeviceSynchronize`（等待全部 stream）。
6. `cudaStreamWaitEvent` 的语义反向理解：应该是"stream A 等 stream B 的 event"，而不是反过来。
7. Nsight Systems 采样率过高（默认 1 kHz）时，host 端会被采样器拖累，造成虚假的延迟膨胀；建议用 `--sample none` 或降低采样率。
8. 以为 stream 创建后自动非阻塞；实际需要显式指定 `cudaStreamNonBlocking`。

### 提示

- 用 `cudaEventCreate` 时不需指定参数；`cudaEventRecord` 时指定 stream。
- 用 `cudaStreamWaitEvent` 时，第三个参数（flags）通常传 0。
- kernel 执行时间可以用内部的计算密集循环保证，例如 `for(int i=0; i<1000; ++i) atomicAdd(...)`（会比较慢）。
- 更好的办法是用一个轻量的 reduce kernel 处理大数据量，自然执行时间就长了。

### 复盘问题

1. 为什么说默认 stream 是"legacy"的，而新代码应该用非默认 stream？
2. 如果你启动了 3 条 stream，但 GPU 只有 80 个 SM 且每个 SM 最多 2 个 warp block，那三条 stream 上的 kernel 还会并发吗？
3. `cudaStreamWaitEvent(streamA, eventB, 0)` 的精确含义是什么？谁在等谁？
4. 为什么 HtoD 拷贝可以和 kernel 并发，但两个 kernel 之间通常需要显式依赖？

### 对应官方参考

- CUDA C++ Programming Guide：[Streams](https://docs.nvidia.com/cuda/cuda-c-programming-guide/#streams)
- CUDA Runtime API：`cudaStreamCreate`, `cudaEventRecord`, `cudaStreamWaitEvent`
- CUDA Best Practices：Stream 的设计考量

---

## 练习 E2：异步内存分配与内存池

### 目标

理解 `cudaMallocAsync` 与 `cudaMallocManaged` 的区别、内存池（memory pool）的设计原理、以及异步分配相比同步 `cudaMalloc` 为什么更低延迟。通过对比释放延迟，感受流有序内存分配器（Stream Ordered Memory Allocator）的优势。

### 前置理解

- 你知道 `cudaMalloc` 是阻塞的——它会等待全部在途 GPU 操作完成后才返回。
- 你理解 stream 的基本概念。
- 你有过显存不足的困扰。

### 必做任务

1. 写一个测试程序，分别在循环中调用 `cudaMalloc` 和 `cudaMallocAsync`（使用默认内存池）各 100 次，每次分配 64 MB。用 `std::chrono` 计时。对比两者的分配延迟。
2. 在 default memory pool 的基础上，用 `cudaMemPoolCreate` 创建一个自定义内存池，再用 `cudaMallocAsync(..., stream, pool_handle)` 在该池上分配内存。观察分配延迟是否变化（通常不大，因为内存池管理的是复用策略）。
3. 在 `cudaMallocAsync` 分配内存后，立刻启动一个 kernel，中途不做任何同步操作，再释放内存。用 `cudaFreeAsync` 释放；观察是否编译通过（应该通过）。
4. 设计一个对比实验：在同一条 stream 上，循环做"分配 → kernel → 释放"各 10 次；用 `cudaStreamSynchronize` 在循环外计时总耗时。对比用 `cudaMalloc/cudaFree` 和用 `cudaMallocAsync/cudaFreeAsync` 两种方式。
5. 用 Nsight Compute 采样一个使用 `cudaMallocAsync` 的 kernel，观察 "Memory Workload Analysis" 中是否能看到内存池的复用（通常表现为分配/释放操作的减少）。

### 进阶任务

- 尝试创建多个内存池，分别用于不同大小的分配；观察池隔离对碎片化的影响（概念理解即可，实测困难）。
- 用 `cudaMemPoolSetAttribute` 设置内存池的释放阈值（threshold），让 GPU 释放不是立刻执行而是延迟。观察在内存紧张场景下的回收行为（需要造成内存压力）。
- 对比 `cudaMallocAsync`（stream ordered）与 `cudaMallocManaged`（统一内存）：在同一个程序中分别测试，观察 page fault 行为（用 compute-sanitizer 可能能看到端倪）。

### 验收点

- 你能给出定量数据：`cudaMallocAsync` 的分配延迟相比 `cudaMalloc` 的改进百分比。
- 你的代码编译通过，且在运行时不触发任何 CUDA 错误。
- 在 kernel 与显存拷贝之间使用 `cudaFreeAsync` 时，没有出现数据竞争或 use-after-free。
- Nsight Compute 报告显示内存操作得到了适当的重排或优化。

### 观察点

- `cudaMalloc` 导致的阻塞来自它必须等待所有在途操作完成（device synchronization）；`cudaMallocAsync` 避免了这一点。
- 内存池的核心是"复用已分配的内存块"，而不是每次都向 OS 申请新块。
- Stream ordered allocator 允许在同一条 stream 上的后续操作"看到"分配结果，即使 host 代码还没看到分配返回。
- `cudaFreeAsync` 不会立刻返还显存给 OS，而是还给内存池等待后续复用。

### 常见坑

1. 忘记在 `cudaMallocAsync` 时指定 stream；会使用默认 stream（通常是 stream 0），可能导致不期望的同步。
2. 释放了属于某个内存池的指针，但没有通过 `cudaFreeAsync(..., stream, pool)` 指定正确的池；导致释放失败或错误。
3. 跨 stream 使用 `cudaMallocAsync` 分配的内存但没有 `cudaStreamWaitEvent` 进行同步，导致数据竞争。
4. 创建内存池后没有 `cudaMemPoolDestroy`。
5. 忘记 `cudaMallocAsync` 返回的是 `void**`，而不是直接的 `void*`；如果用 `cudaMalloc` 的写法会导致类型错误。实际上新 API 也是 `void*` 返回，但要记清楚。
6. 在 CPU-only 环境没有设置 CUDA_VISIBLE_DEVICES，导致程序试图分配却失败。
7. 异步分配不代表"无需等待"，只是说"等待延迟更低"；关键操作前仍需 `cudaStreamSynchronize`。
8. 内存池的释放阈值设置得太高，导致显存泄漏现象（其实是延迟释放）。

### 提示

- `cudaMallocAsync(ptr, size, stream)` 和 `cudaMallocAsync(ptr, size, stream, pool)` 是两个重载；后者指定自定义池。
- 在分配之前，建议打印当前 free/total 显存以 baseline。
- 如果要对比多个分配策略，用一个简单的 struct 包装分配函数，方便 A/B test。

### 复盘问题

1. 为什么说 `cudaMallocAsync` 比 `cudaMalloc` 快，而本质上两者都是显存申请？
2. 内存池的"stream ordered"是什么意思？为什么顺序很重要？
3. 如果你在一条 stream 上用 `cudaMallocAsync` 分配、在另一条 stream 上访问该内存，会发生什么？
4. `cudaMemPoolSetAttribute` 的释放阈值设得很高（比如 100%）有什么坏处？

### 对应官方参考

- CUDA C++ Programming Guide：[Stream Ordered Memory Allocator](https://docs.nvidia.com/cuda/cuda-c-programming-guide/#stream-ordered-memory-allocator)
- CUDA Runtime API：`cudaMallocAsync`, `cudaFreeAsync`, `cudaMemPoolCreate`, `cudaMemPoolSetAttribute`

---

## 练习 E3：流捕获与 CUDA Graph

### 目标

理解 CUDA Graph 的核心价值——消除 kernel launch 开销——并学习两种构建方式：stream capture API（隐式构图）与 explicit graph API（显式构图）。通过对比 launch overhead，感受 graph replay 带来的性能提升。

### 前置理解

- 你知道 stream 的基本用法。
- 你理解 kernel launch 需要 host-device 通信，存在延迟（通常微秒级）。
- 你想象过"如果能批量启动 N 个 kernel 而不用反复调用 CUDA API，会快多少"。

### 必做任务

1. 写一个轻量 kernel（执行时间 < 1 ms），例如简单的元素加法。
2. 在循环中启动该 kernel 1000 次，用 `cudaEventRecord` 计时总耗时。这是 baseline（每次 launch overhead）。
3. 用 `cudaStreamBeginCapture(stream, cudaStreamCaptureModeGlobal)` 开始捕获，在同一条 stream 上启动同一个 kernel 1000 次，再用 `cudaStreamEndCapture(stream, &graph)` 结束捕获，获得 graph 对象。
4. 用 `cudaGraphInstantiate(&graphExec, graph, NULL, NULL, 0)` 编译 graph 为可执行形式。
5. 在循环中用 `cudaGraphLaunch(graphExec, stream)` 重放该 graph 100 次（共执行 100,000 个 kernel 启动），用 event 计时。
6. 对比两个版本的总耗时；预期 graph 版本快 1-3 个数量级（因为消除了 100,000 × launch overhead）。
7. 打印出单个 kernel 的执行时间（应该很短），以及平均 launch overhead 的估计（总耗时 / 启动次数）。

### 进阶任务

- 增加 graph 的复杂度：不只是 kernel launch，还包含 `cudaMemcpy` 和依赖关系；观察 capture 是否仍然有效。
- 在 capture 期间故意做一些 host-side 操作（例如日志输出），观察它是否被忽略或导致 capture 失败。
- 尝试在 capture 过程中调用 `cudaStreamSynchronize`；观察是否报错或出现不期望的行为。

### 验收点

- 你能用秒表数据证明：graph replay 版本比逐个 launch 版本快至少 10 倍。
- `cudaStreamEndCapture` 成功返回 `cudaSuccess`。
- `cudaGraphLaunch` 执行正确，结果与逐个 launch 版本一致（可以验证最终输出）。
- 你能估算出单个 launch 的平均开销（以微秒计）。

### 观察点

- graph capture 的核心是"记录一系列 GPU 操作"，而不真的执行它们；执行延迟到 `cudaGraphLaunch` 时。
- `cudaStreamBeginCapture` 必须与 `cudaStreamEndCapture` 配对，中间不能有 host-side 同步操作（如 `cudaStreamSynchronize`）。
- 一旦 graph 被 instantiate，就变成一个不可变的对象 `cudaGraphExec_t`；每次 launch 都复用相同的图结构。
- graph 的威力不在单个 launch，而在"批量重放"——你花一次捕获成本，换来千次快速 replay。

### 常见坑

1. 在 capture 期间调用 `cudaStreamSynchronize`，导致 capture 失败或行为不对。
2. 忘记 `cudaGraphInstantiate` 直接用 graph 对象 launch；这不会编译失败，但运行会出错。
3. 修改了 kernel 的参数（例如 block size 或输入数据指针）后没有重新 capture 或 instantiate，还期望新参数生效。
4. graph 中包含了 host callback，导致 graph 无法高效重放（host callback 会打破 GPU 管道）。
5. 创建 graph 后没有 `cudaGraphDestroy`。
6. 在 graph 执行期间修改 graph 的内容（如果你存了 graph 对象的引用），导致数据竞争。
7. 以为 capture 一定比手工逐个 launch 更快；实际要取决于 kernel 执行时间和 launch 数量比例。
8. 在 capture 模式下用了不支持 capture 的 API（例如某些 cuBLAS 调用），导致 capture 失败。

### 提示

- `cudaStreamCaptureModeGlobal` 会捕获该 stream 及所有使用了 default stream 的操作；`cudaStreamCaptureModeThreadLocal` 只捕获该 thread 的操作。
- 如果 capture 过程中 kernel 返回了错误，`cudaStreamEndCapture` 会返回非零值；检查返回值很重要。
- 可以用 `cudaGraphGetNodes` 和 `cudaGraphNodeGetType` 来检查生成的 graph 包含了哪些节点。

### 复盘问题

1. 为什么 CUDA Graph 能显著降低 launch overhead？
2. 如果你的 kernel 每次执行时间都是 100 ms，capture 和 graph launch 还有优势吗？
3. `cudaStreamCaptureModeGlobal` 和 `cudaStreamCaptureModeThreadLocal` 的适用场景分别是什么？
4. 一旦 instantiate，graph 就不能修改了吗？怎样在运行时更新 kernel 参数？

### 对应官方参考

- CUDA C++ Programming Guide：[CUDA Graphs](https://docs.nvidia.com/cuda/cuda-c-programming-guide/#cuda-graphs)
- CUDA Runtime API：`cudaStreamBeginCapture`, `cudaStreamEndCapture`, `cudaGraphInstantiate`, `cudaGraphLaunch`

---

## 练习 E4：显式图 API 与动态参数更新

### 目标

学习 explicit graph API 的方式构建 DAG，以及如何用 `cudaGraphExecKernelNodeSetParams` 在运行时修改 kernel 参数，而无需重新 capture 或 instantiate。这种动态更新对迭代算法（例如神经网络推理循环）非常有用。

### 前置理解

- 你已经完成了 E3，理解了 CUDA Graph 的基本概念。
- 你知道如何用 `cudaGraphAddKernelNode` 手工添加 kernel 节点。
- 你理解 DAG（有向无环图）的依赖关系。

### 必做任务

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

### 进阶任务

- 在 graph 中添加 `cudaGraphAddMemcpyNode` 节点，例如在 kernel C 执行后拷贝结果回 host。
- 用 `cudaGraphAddEmptyNode` 创建一个空节点作为同步点，再让其它节点依赖它。
- 尝试删除或修改已存在的节点（需要用 `cudaGraphRemoveNode` 或重建 graph）；观察 graph 结构如何变化。

### 验收点

- 三个 kernel 按照指定的依赖顺序执行（可以用 `__threadfence` 或原子操作验证执行顺序）。
- 动态参数更新成功，新数据被正确处理。
- 循环 10 次迭代，输出都正确无误。
- 能给出动态更新相比重新 capture 的性能改进数据（预期改进较小但存在）。

### 观察点

- explicit graph API 给你完全的 DAG 控制权——你可以精确指定节点和依赖，而不依赖 capture 的启发式推断。
- `cudaGraphExecKernelNodeSetParams` 的成本远低于重新 capture + instantiate。
- 即使参数变了，graph 的拓扑结构（节点和边）保持不变，只有参数值被更新。

### 常见坑

1. 忘记 `cudaGraphInstantiate` 直接修改 graph 对象，导致操作无效。
2. `cudaGraphExecKernelNodeSetParams` 时，参数结构体的内存布局必须与原始参数一致，否则数据混乱。
3. 修改了节点参数但没有 `cudaStreamSynchronize` 等待前一次 launch 完成，导致数据竞争。
4. 试图在 instantiate 后修改 graph 的拓扑（添加删除节点），这是不允许的；必须修改 graph 再重新 instantiate。
5. `cudaGraphRemoveNode` 后，依赖该节点的其它节点仍然保留（成为悬空依赖），导致行为不对。
6. 创建了大量 graph 对象但没有及时 `cudaGraphDestroy`。
7. 在 `cudaGraphAddDependencies` 时顺序搞反，例如应该是 B 依赖 A，结果写反了。

### 提示

- `cudaGraphAddKernelNode` 返回一个 node handle；保存这些 handle 以便后续修改。
- 参数结构体通常是 `cudaKernelNodeParams`；确保 kernel function pointer、grid size、block size、shared memory、args array 都设置正确。
- 如果 kernel 参数很多，考虑用一个结构体打包它们，再传指针，这样参数更新时更方便。

### 复盘问题

1. explicit graph API 相比 stream capture 的优势和劣势各是什么？
2. 为什么 `cudaGraphExecKernelNodeSetParams` 允许修改参数但不允许修改节点拓扑？
3. 如果你用 explicit API 构建一个 100 节点的 graph，每次都更新全部参数，性能会怎样？

### 对应官方参考

- CUDA C++ Programming Guide：[Creating a Graph Explicitly](https://docs.nvidia.com/cuda/cuda-c-programming-guide/#creating-a-graph-explicitly)
- CUDA Runtime API：`cudaGraphCreate`, `cudaGraphAddKernelNode`, `cudaGraphAddDependencies`, `cudaGraphExecKernelNodeSetParams`

---

## 练习 E5：Multi-Process Service 与并发客户端

### 目标

理解 Multi-Process Service（MPS）的基本概念、启动方式、以及在什么场景下开启它。通过观察单 GPU 多 process 并发时的 context switch overhead，体会 MPS 如何降低这种开销。

### 前置理解

- 你知道 CUDA context 是 device 上的虚拟执行环境，每个 process 默认有自己的 context。
- 你理解"context switch"会带来延迟（需要 flush GPU 管道）。
- 你有过在一个 GPU 上跑多个进程的经历，或者知道这种场景会变慢。

### 必做任务

1. 写一个轻量的 CUDA 程序 `worker.cu`，启动一个简单 kernel（例如 1-10 ms 执行时间），输出当前 process ID 和执行时间。
2. 不用 MPS：用命令行或脚本同时启动 4 个 `worker` 进程（在同一块 GPU 上）。观察它们的完成时间；由于 context switch 开销，总耗时应该明显大于单个 worker 的时间。
3. 启动 MPS daemon：`nvidia-cuda-mps-control -d`（需要 root 或特定权限）。或者在文档中说明如何启动（通常涉及设置环境变量）。
4. 再次同时启动 4 个 `worker` 进程。观察完成时间；预期会快于无 MPS 的情况（因为多个 process 共享同一个 GPU context）。
5. 记录有无 MPS 时的对比数据（总耗时、吞吐量）。
6. 用 `nvidia-smi -i <gpu_id>` 或 Nsight Systems 观察 GPU 的利用率变化。

### 进阶任务

- 尝试在 MPS daemon 启动后，设置 active thread percentage（`nvidia-cuda-mps-control` 的 `set_active_thread_percentage_default_nnodes` 命令），限制某些 process 的资源使用；观察吞吐变化。
- 对比多个小 kernel 的场景与单个大 kernel 的场景，观察 MPS 的收益是否随之变化。
- 如果有两块 GPU，在其中一块上启动 MPS，另一块不启用，对比两者在多 process 场景下的性能（概念理解即可）。

### 验收点

- 你能给出有无 MPS 时的定量对比（例如总耗时、平均 latency）。
- 4 个 process 在有 MPS 的情况下完成得更快（如果硬件和驱动支持的话）。
- 你理解了为什么 MPS 对多 process 场景有帮助。

### 观察点

- MPS 的核心是"多个 host process 共享一个 GPU context"，避免频繁的 context switch。
- 没有 MPS 时，context switch 可能导致 GPU 管道清空和重填，成本不菲。
- MPS 的资源隔离是软件级的（基于 thread block scheduling），而不是硬件级的；因此多个 process 仍然会竞争 SM。
- MPS 对 many small kernels 的收益比对 few large kernels 的收益更大（因为 context switch 开销的占比不同）。

### 常见坑

1. 启动 MPS daemon 需要 root 权限或特定用户，某些集群环境下不允许（文档应该说明）。
2. MPS daemon 启动后，忘记设置 `CUDA_MPS_PIPE_DIRECTORY` 环境变量，导致 client process 无法连接。
3. 在有 MPS 的情况下，多个 process 竞争资源时，某个 process 的 kernel 可能被其它 process 打断，导致执行顺序变化（需要理解这不是 bug）。
4. 假设 MPS 在所有场景下都快；实际上如果只有一个 process，MPS 可能略慢（因为多了一层中介）。
5. MPS 不支持 CUDA Graphs 的某些特性（这取决于 CUDA 版本和驱动版本）；混用时可能出错。
6. 关闭 MPS daemon 时没有 graceful shutdown，导致还在运行的 client process 出错。

### 提示

- `nvidia-cuda-mps-control` 通常在 CUDA Toolkit 的 `bin` 目录中。
- 设置 `CUDA_DEVICE_ORDER=PCI_BUS_ID` 和 `CUDA_VISIBLE_DEVICES=<id>` 以精确指定 GPU。
- 如果不能启动 MPS daemon（权限原因），可以在文档中详细说明启动流程和预期结果，然后用概念理解通过验收。

### 复盘问题

1. MPS 中的"multi-process"指的是什么？它如何不同于 CUDA Graph replay 的并发？
2. 为什么 MPS 的资源隔离是"软件级"而不是"硬件级"？这有什么含义？
3. 在 MPS 启用的情况下，如果两个 process 的 kernel 同时启动，它们会怎样执行？
4. MPS 对 stream 并发有什么影响？

### 对应官方参考

- NVIDIA CUDA Multi-Process Service Documentation：https://docs.nvidia.com/deploy/cuda-mps/
- CUDA Best Practices：多 process 场景

---

## 做完本模块后应达到的水平

至少把下面几句话说顺：

- 为什么默认 stream（legacy default stream）会导致全局同步，生产代码必须用非默认 stream。
- `cudaStreamWaitEvent` 如何在多 stream 间建立显式依赖，以及它与 `cudaDeviceSynchronize` 的区别。
- 异步内存分配（`cudaMallocAsync` + stream ordered memory allocator）相比 `cudaMalloc` 的低延迟优势来自哪里。
- CUDA Graph 通过消除 kernel launch 开销来加速重复执行模式，stream capture 与 explicit API 各自的场景。
- `cudaGraphExecKernelNodeSetParams` 如何允许在不重新 capture/instantiate 的情况下动态更新 kernel 参数。
- Multi-Process Service 为什么能降低多 process 竞争单 GPU 时的 context switch 开销。
- 如何用 Nsight Systems 时间线直观地观察 stream 并发、CUDA Graph 高效性、以及 MPS 的效果。

你应该能够：

- 设计一个多 stream 并发的 kernel 启动序列，使得数据传输与计算能够重叠。
- 用 stream capture 或 explicit graph API 打包一个复杂的执行图，并测量它相比逐个 launch 的加速倍数。
- 根据应用场景判断何时用 `cudaMalloc`、何时用 `cudaMallocAsync`。
- 用 Nsight Systems profiler 定位 stream 间同步不足或过度的问题。

</content>
