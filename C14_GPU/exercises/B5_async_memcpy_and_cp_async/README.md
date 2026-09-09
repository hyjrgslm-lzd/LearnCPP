# 练习 B5：async_memcpy_and_cp_async

## 目标

`[B5-T01]` (main.cu:2) 练习 B5：async_memcpy_and_cp_async。
`[B5-T02]` (main.cu:3) 用 cudaMemcpyAsync + stream 实现双缓冲，让 H2D 传输与 kernel 计算重叠；掌握 sm_80+ 的 `cuda::memcpy_async`。
`[B5-T03]` (main.cu:7) 验收：双缓冲版比顺序版快 1.5-2x；Nsight Systems 命令 `nsys profile --trace cuda,nvtx ./B5_async_memcpy_and_cp_async.exe`。

学会用 `cudaMemcpyAsync` 和 stream 实现 host ↔ device 数据搬运与 kernel 计算的并发。掌握 sm_80+ 的 `cp.async` 指令（CUDA C++ `cuda::memcpy_async`），实现 producer-consumer 双缓冲。

## 前置理解

- 完成 B4。
- 你知道 CUDA stream 是异步执行队列。
- 接受"多个 kernel 在不同 stream 上可以并发执行"。

## 必做任务

`[B5-T08]` (main.cu:35) Kernel：占用 GPU 的计算 kernel（用于让 H2D 与计算重叠）。
`[B5-T09]` (main.cu:37) TODO [必做] 步骤 1：理解此 kernel 的作用——消耗计算资源。
`[B5-T10]` (main.cu:46) TODO [必做] 做迭代 sin/cos 计算让 GPU 忙碌。
`[B5-T12]` (main.cu:71) TODO [必做] 顺序：先同步 memcpy。
`[B5-T15]` (main.cu:96) TODO [必做] 步骤 2：创建两个 stream。
`[B5-T16]` (main.cu:107) TODO [必做] 步骤 3：双缓冲主循环。
`[B5-T17]` (main.cu:117) stream1 异步搬运当前批数据。
`[B5-T18]` (main.cu:124) stream2 对上一批数据做计算（第 0 批没有"上一批"）。
`[B5-T22]` (main.cu:175) TODO [必做] 步骤 5（sm_80+ 硬件）：取消注释并填写 cp.async kernel。
`[B5-T24]` (main.cu:184) TODO [必做] 用 `cuda::memcpy_async` 异步加载到 smem。
`[B5-T34]` (main.cu:230) TODO [必做] 步骤 4：运行 Nsight Systems。
`[B5-T36]` (main.cu:234) TODO [必做] 步骤 5（sm_80+）：取消注释 cp.async kernel 并测试。

1. 理解 `compute_kernel`（已在 `main.cu` 中给出）：做 sin/cos 迭代计算，占用 GPU 资源：

```cpp
// TODO [必做] __global__ void compute_kernel(float *output, int n, int iterations) {
//     int idx = blockIdx.x * blockDim.x + threadIdx.x;
//     float val = 1.0f;
//     for (int i = 0; i < iterations; i++) {
//         val = sin(val) * cos(val);  // 占用计算资源
//     }
//     output[idx] = val;
// }
```

2. 在 host 上，创建两个 stream：`stream1` 和 `stream2`：

```cpp
// TODO [必做] cudaStream_t stream1, stream2;
// TODO [必做] CUDA_CHECK(cudaStreamCreate(&stream1));
// TODO [必做] CUDA_CHECK(cudaStreamCreate(&stream2));
```

3. 实现双缓冲策略：

```cpp
// TODO [必做] for (int batch = 0; batch < num_batches; batch++) {
//     // stream1: memcpy 第 batch 批数据
//     CUDA_CHECK(cudaMemcpyAsync(d_batch, h_batch, batch_size, cudaMemcpyHostToDevice, stream1));
//     // stream2: kernel 处理前一批
//     if (batch > 0) {
//         compute_kernel<<<grid, block, 0, stream2>>>(d_prev_batch, n, iterations);
//     }
// }
```

4. 计算总时间。对比顺序版本（先 memcpy 再 kernel）和双缓冲版本的时间。双缓冲应该快接近 2 倍。
5. （仅 sm_80+ 硬件）用 `cuda::memcpy_async` 实现 shared memory 的异步加载：

```cpp
// TODO [必做] #include <cuda/pipeline>
// TODO [必做] // 在 kernel 中：
// TODO [必做] auto pipe = cuda::make_pipeline();
// TODO [必做] cuda::memcpy_async(smem + tid, global_ptr + idx, sizeof(float), pipe);
// TODO [必做] pipe.producer_commit();
// TODO [必做] // ... 其他计算 ...
// TODO [必做] pipe.consumer_wait();
```

6. 运行程序，验证 `cp.async` 版本的 kernel 和计算能重叠。

## 进阶任务

- 在双缓冲中加入第三个流，进行 device→host memcpy（把结果回传），形成"三管"并行。
- 用 `cudaStreamSynchronize` 显式同步某个 stream，观察对性能的影响。
- 在 B5 中加入 `NVTX_RANGE("memcpy")` 等标记，用 Nsight Systems 查看三个流的时间线并发情况。

## 验收点

`[B5-T04]` (main.cu:16) sm_80+ cp.async 头文件提示。
`[B5-T05]` (main.cu:27) 常量段。
`[B5-T06]` (main.cu:29) 每批 1M floats = 4MB。
`[B5-T07]` (main.cu:33) ITERATIONS 内层循环放大计算量。
`[B5-T11]` (main.cu:60) 顺序版本 baseline 段。
`[B5-T13]` (main.cu:75) 默认 stream 串行 kernel。
`[B5-T14]` (main.cu:87) 双缓冲版本段。
`[B5-T19]` (main.cu:135) 处理最后一批（stream2 上处理第 num_batches-1 批）。
`[B5-T20]` (main.cu:139) 等待 stream1 的最后一次 memcpy 完成。
`[B5-T21]` (main.cu:154) cp.async 内核骨架（sm_80+）。
`[B5-T23]` (main.cu:167) 每个 block 处理 blockDim.x 个元素。
`[B5-T25]` (main.cu:189) main 入口。
`[B5-T26]` (main.cu:199) 分配 pinned host 内存。
`[B5-T27]` (main.cu:204) 分配 device 缓冲区。
`[B5-T28]` (main.cu:213) 顺序版调用。
`[B5-T30]` (main.cu:218) 双缓冲版调用。
`[B5-T33]` (main.cu:228) Nsight Systems 提示段。

- 双缓冲版本比顺序版本快 1.5–2 倍。
- `cp.async` 版本编译通过（sm_80+）且运行正确。
- 没有 stream 同步错误。
- Nsight Systems 时间线显示流之间有并发。

## 观察点

- `cudaMemcpyAsync` 立刻返回，不等待搬运完成。host 可以继续执行。
- stream 是任务队列。同一 stream 内任务按顺序执行；不同 stream 的任务可以并发。
- `cp.async` 是 device 端的异步 memcpy，从全局内存异步加载到 shared memory，不阻塞当前 warp。
- 双缓冲的关键是"当 stream2 在计算时，stream1 在搬运"，两者并发。

## 常见坑

1. **忘记创建 stream**：如果不创建，默认是 stream 0（同步 stream），不能并发。
2. **同一 stream 内多个 memcpy 后立刻 host 访问**：需要 `cudaStreamSynchronize` 确保完成。
3. **两个 stream 之间没有依赖管理**：如果 stream2 的 kernel 需要 stream1 的 memcpy 数据，需要显式用 event 依赖。
4. **没有释放 stream**：需要 `cudaStreamDestroy`。
5. **编译 `cp.async` 时缺少头文件**：需要 `<cuda/pipeline>` 或 `<cuda/barrier>`（随 CUDA Toolkit cccl 附带）。

## 提示

- stream 之间的依赖可以用 `cudaStreamWaitEvent` 显式建立。
- `cudaMemcpyAsync` 的 pinned memory 版本性能最好（本例已使用 `cudaMallocHost`）。
- `cp.async` 需要 `pipe.consumer_wait()` 或 `__syncthreads()` 等待完成。
- Nsight Systems 命令：`nsys profile --trace cuda,nvtx ./exe` 可视化流的时间线。

## 复盘问题

1. `cudaMemcpyAsync` 和同步版本的区别是什么？
2. 双缓冲为什么能加快速度？
3. stream 之间可以并发执行的条件是什么？
4. `cp.async` 和 `cudaMemcpyAsync` 的区别（位置、性能、接口）？
5. 如果没有 `cudaStreamSynchronize`，host 能否安全读取 device 结果？

## 对应官方参考

- CUDA Runtime API / Stream: https://docs.nvidia.com/cuda/cuda-runtime-api/
- CUDA C++ Programming Guide / Asynchronous Copies: https://docs.nvidia.com/cuda/cuda-c-programming-guide/
- libcu++ API: https://nvidia.github.io/cccl/

## 输出对照（printf / std::puts 原文）

- `[B5-T29]` (main.cu:215) 原文：`[B5] 顺序版总时:     %.1f ms` -> 现：`[B5] sequential total:    %.1f ms`
- `[B5-T31]` (main.cu:223) 原文：`[B5] 双缓冲版总时:   %.1f ms` -> 现：`[B5] double-buffer total: %.1f ms`
- `[B5-T32]` (main.cu:226) 原文：`[B5] 加速比:         %.2fx (期望 1.5-2x)` -> 现：`[B5] speedup:             %.2fx (expected 1.5-2x)`
- `[B5-T35]` (main.cu:232) 原文：`观察两个流的时间线，确认双缓冲时 H2D 与 kernel 重叠。` -> 现：`Inspect the two streams' timeline; confirm H2D overlaps with kernel.`
