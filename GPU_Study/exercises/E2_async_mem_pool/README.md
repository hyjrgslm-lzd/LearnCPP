# 练习 E2：异步内存分配与内存池

## 目标

`[E2-T01]` (main.cu:1) E2_async_mem_pool/main.cu。
`[E2-T02]` (main.cu:2) 练习目标：理解 `cudaMallocAsync` / `cudaFreeAsync`、流有序内存分配器（Stream Ordered Memory Allocator）；对比 `cudaMalloc` vs `cudaMallocAsync` 分配延迟；自定义 `cudaMemPoolCreate`。

理解 `cudaMallocAsync` 与 `cudaMallocManaged` 的区别、内存池（memory pool）的设计原理、以及异步分配相比同步 `cudaMalloc` 为什么更低延迟。通过对比释放延迟，感受流有序内存分配器的优势。

## 前置理解

- 你知道 `cudaMalloc` 是阻塞的——它会等待全部在途 GPU 操作完成后才返回。
- 你理解 stream 的基本概念。
- 你有过显存不足的困扰。

## 必做任务

`[E2-T03]` (main.cu:5) 模块 E 练习 E2 的必做任务清单。
`[E2-T08]` (main.cu:42) TODO [必做-3] 实现 `kernel_fill`：`out[idx] = val`（每个线程写一个元素）。
`[E2-T09]` (main.cu:44) TODO [必做-3] kernel 函数体内实现。
`[E2-T12]` (main.cu:60) TODO [必做-1] `cudaMalloc(&ptr, ALLOC_SIZE)`。
`[E2-T13]` (main.cu:61) TODO [必做-1] `cudaFree(ptr)`。
`[E2-T17]` (main.cu:84) TODO [必做-1] `cudaMallocAsync(&ptr, ALLOC_SIZE, stream)`。
`[E2-T18]` (main.cu:85) TODO [必做-1] `cudaFreeAsync(ptr, stream)`。
`[E2-T22]` (main.cu:106) TODO [必做-2] 创建自定义内存池（`cudaMemPoolCreate`）。
`[E2-T23]` (main.cu:117) TODO [必做-2] `cudaMallocFromPoolAsync(&ptr, ALLOC_SIZE, pool, stream)`。
`[E2-T24]` (main.cu:118) TODO [必做-2] `cudaFreeAsync(ptr, stream)`。
`[E2-T26]` (main.cu:128) TODO [必做-2] `cudaMemPoolDestroy(pool)`。
`[E2-T29]` (main.cu:148) TODO [必做-3] `cudaMallocAsync(&d_buf, ALLOC_SIZE, stream)`。
`[E2-T30]` (main.cu:150) TODO [必做-3] 启动 `kernel_fill`。
`[E2-T31]` (main.cu:153) TODO [必做-3] `cudaFreeAsync(d_buf, stream)`。
`[E2-T35]` (main.cu:179) TODO [必做-4] `cudaMalloc(&d_buf, ALLOC_SIZE)`。
`[E2-T36]` (main.cu:181) TODO [必做-4] 启动 `kernel_fill`。
`[E2-T37]` (main.cu:184) TODO [必做-4] `cudaFree(d_buf)`。

1. 写一个测试程序，分别在循环中调用 `cudaMalloc` 和 `cudaMallocAsync`（使用默认内存池）各 100 次，每次分配 64 MB。用 `std::chrono` 计时。对比两者的分配延迟。
2. 在 default memory pool 的基础上，用 `cudaMemPoolCreate` 创建一个自定义内存池，再用 `cudaMallocAsync(..., stream, pool_handle)` 在该池上分配内存。观察分配延迟是否变化（通常不大，因为内存池管理的是复用策略）。
3. 在 `cudaMallocAsync` 分配内存后，立刻启动一个 kernel，中途不做任何同步操作，再释放内存。用 `cudaFreeAsync` 释放；观察是否编译通过（应该通过）。
4. 设计一个对比实验：在同一条 stream 上，循环做"分配 -> kernel -> 释放"各 10 次；用 `cudaStreamSynchronize` 在循环外计时总耗时。对比用 `cudaMalloc/cudaFree` 和用 `cudaMallocAsync/cudaFreeAsync` 两种方式。
5. 用 Nsight Compute 采样一个使用 `cudaMallocAsync` 的 kernel，观察 "Memory Workload Analysis" 中是否能看到内存池的复用（通常表现为分配/释放操作的减少）。

## 进阶任务

`[E2-T04]` (main.cu:12) 进阶任务清单（TODO [进阶]）。
`[E2-T45]` (main.cu:222) TODO [进阶-B] 设置内存池释放阈值（`cudaMemPoolSetAttribute`）。
`[E2-T46]` (main.cu:229) TODO [进阶-C] 对比 `cudaMallocManaged` page fault 行为。

- 尝试创建多个内存池，分别用于不同大小的分配；观察池隔离对碎片化的影响（概念理解即可，实测困难）。
- 用 `cudaMemPoolSetAttribute` 设置内存池的释放阈值（threshold），让 GPU 释放不是立刻执行而是延迟。观察在内存紧张场景下的回收行为（需要造成内存压力）。
- 对比 `cudaMallocAsync`（stream ordered）与 `cudaMallocManaged`（统一内存）：在同一个程序中分别测试，观察 page fault 行为（用 compute-sanitizer 可能能看到端倪）。

## 验收点

`[E2-T05]` (main.cu:28) 常量定义。
`[E2-T06]` (main.cu:36) Kernel（TODO 区域）。
`[E2-T07]` (main.cu:40) `kernel_fill`：简单元素写操作，让分配的内存被实际使用。
`[E2-T10]` (main.cu:48) 测试 A：`cudaMalloc` / `cudaFree` 循环（baseline）。
`[E2-T15]` (main.cu:72) 测试 B：`cudaMallocAsync` / `cudaFreeAsync`（默认内存池）。
`[E2-T20]` (main.cu:96) 测试 C：`cudaMemPoolCreate` 自定义内存池。
`[E2-T27]` (main.cu:132) 测试 D：无中间同步的 alloc -> kernel -> free 流水线。
`[E2-T33]` (main.cu:164) 测试 E：对比 `cudaMalloc` 版本的相同流水线。
`[E2-T39]` (main.cu:195) `main` 入口。
`[E2-T41]` (main.cu:204) 打印当前可用显存。
`[E2-T43]` (main.cu:211) 创建工作 stream。
`[E2-T44]` (main.cu:214) 运行各测试。
`[E2-T47]` (main.cu:230) 清理。

- 你能给出定量数据：`cudaMallocAsync` 的分配延迟相比 `cudaMalloc` 的改进百分比。
- 你的代码编译通过，且在运行时不触发任何 CUDA 错误。
- 在 kernel 与显存拷贝之间使用 `cudaFreeAsync` 时，没有出现数据竞争或 use-after-free。
- Nsight Compute 报告显示内存操作得到了适当的重排或优化。

## 观察点

- `cudaMalloc` 导致的阻塞来自它必须等待所有在途操作完成（device synchronization）；`cudaMallocAsync` 避免了这一点。
- 内存池的核心是"复用已分配的内存块"，而不是每次都向 OS 申请新块。
- Stream ordered allocator 允许在同一条 stream 上的后续操作"看到"分配结果，即使 host 代码还没看到分配返回。
- `cudaFreeAsync` 不会立刻返还显存给 OS，而是还给内存池等待后续复用。

## 常见坑

1. 忘记在 `cudaMallocAsync` 时指定 stream；会使用默认 stream（通常是 stream 0），可能导致不期望的同步。
2. 释放了属于某个内存池的指针，但没有通过 `cudaFreeAsync(..., stream, pool)` 指定正确的池；导致释放失败或错误。
3. 跨 stream 使用 `cudaMallocAsync` 分配的内存但没有 `cudaStreamWaitEvent` 进行同步，导致数据竞争。
4. 创建内存池后没有 `cudaMemPoolDestroy`。
5. 忘记 `cudaMallocAsync` 返回的是 `void**`，而不是直接的 `void*`；如果用 `cudaMalloc` 的写法会导致类型错误。实际上新 API 也是 `void*` 返回，但要记清楚。
6. 在 CPU-only 环境没有设置 CUDA_VISIBLE_DEVICES，导致程序试图分配却失败。
7. 异步分配不代表"无需等待"，只是说"等待延迟更低"；关键操作前仍需 `cudaStreamSynchronize`。
8. 内存池的释放阈值设置得太高，导致显存泄漏现象（其实是延迟释放）。

## 提示

- `cudaMallocAsync(ptr, size, stream)` 和 `cudaMallocAsync(ptr, size, stream, pool)` 是两个重载；后者指定自定义池。
- 在分配之前，建议打印当前 free/total 显存以 baseline。
- 如果要对比多个分配策略，用一个简单的 struct 包装分配函数，方便 A/B test。

## 复盘问题

1. 为什么说 `cudaMallocAsync` 比 `cudaMalloc` 快，而本质上两者都是显存申请？
2. 内存池的"stream ordered"是什么意思？为什么顺序很重要？
3. 如果你在一条 stream 上用 `cudaMallocAsync` 分配、在另一条 stream 上访问该内存，会发生什么？
4. `cudaMemPoolSetAttribute` 的释放阈值设得很高（比如 100%）有什么坏处？

## 对应官方参考

- CUDA C++ Programming Guide：[Stream Ordered Memory Allocator](https://docs.nvidia.com/cuda/cuda-c-programming-guide/#stream-ordered-memory-allocator)
- CUDA Runtime API：`cudaMallocAsync`, `cudaFreeAsync`, `cudaMemPoolCreate`, `cudaMemPoolSetAttribute`

## 输出对照（printf / std::puts 原文）

- `[E2-T11]` (main.cu:53) 原文：`--- cudaMalloc benchmark (... iters x ... MB) ---` -> 现：保持英文不变
- `[E2-T14]` (main.cu:67) 原文：`总耗时 = %.3f ms, 平均 = %.3f ms/alloc` -> 现：`total = %.3f ms, avg = %.3f ms/alloc`
- `[E2-T16]` (main.cu:77) 原文：`--- cudaMallocAsync (default pool) benchmark ... ---` -> 现：保持英文不变
- `[E2-T19]` (main.cu:91) 原文：`总耗时 = %.3f ms, 平均 = %.3f ms/alloc` -> 现：`total = %.3f ms, avg = %.3f ms/alloc`
- `[E2-T21]` (main.cu:101) 原文：`--- cudaMallocAsync (custom pool) benchmark ... ---` -> 现：保持英文不变
- `[E2-T25]` (main.cu:124) 原文：`总耗时 = %.3f ms, 平均 = %.3f ms/alloc` -> 现：`total = %.3f ms, avg = %.3f ms/alloc`
- `[E2-T28]` (main.cu:137) 原文：`--- 流水线 alloc->kernel->free (%d 轮) ---` -> 现：`--- pipeline alloc->kernel->free (%d rounds) ---`
- `[E2-T32]` (main.cu:159) 原文：`cudaMallocAsync 流水线总耗时 = %.3f ms` -> 现：`cudaMallocAsync pipeline total = %.3f ms`
- `[E2-T34]` (main.cu:169) 原文：`--- 流水线 alloc->kernel->free (cudaMalloc 版, %d 轮) ---` -> 现：`--- pipeline alloc->kernel->free (cudaMalloc version, %d rounds) ---`
- `[E2-T38]` (main.cu:190) 原文：`cudaMalloc 流水线总耗时 = %.3f ms` -> 现：`cudaMalloc pipeline total = %.3f ms`
- `[E2-T40]` (main.cu:202) 原文：`=== E2: 异步内存分配与内存池 ===` -> 现：`=== E2: async memory allocation and memory pools ===`
- `[E2-T42]` (main.cu:207) 原文：`显存：可用 %.1f GB / 总计 %.1f GB` -> 现：`device memory: free %.1f GB / total %.1f GB`
- `[E2-T48]` (main.cu:233) 原文：`提示：用 'ncu --set full -o e2_report ...' 采集` -> 现：`hint: capture with 'ncu --set full -o e2_report ...'`
- `[E2-T49]` (main.cu:235) 原文：`查看 Memory Workload Analysis 中的池复用情况` -> 现：`check Memory Workload Analysis for pool reuse`
