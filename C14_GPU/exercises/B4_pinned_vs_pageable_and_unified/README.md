# 练习 B4：pinned_vs_pageable_and_unified

## 目标

`[B4-T01]` (main.cu:2) 练习 B4：pinned_vs_pageable_and_unified。
`[B4-T02]` (main.cu:3) 理解 pinned memory（DMA 可用）、pageable memory（分页 malloc）、UVM（统一虚拟内存）的 H2D 带宽差异。
`[B4-T03]` (main.cu:6) 验收：pinned 版吞吐 >= 2x pageable 版；Nsight Systems 命令 `nsys profile --trace cuda ./B4_pinned_vs_pageable_and_unified.exe`。

理解 pinned memory（DMA 可用）、pageable memory（分页 malloc）、UVM（统一虚拟内存）的性能差异。用 Nsight Systems 观察 PCIe 传输的效果。

## 前置理解

- 完成 B1–B3。
- 你知道 host ↔ device 数据搬运经过 PCIe。
- 接受 pinned 内存比普通 malloc 更快。

## 必做任务

`[B4-T17]` (main.cu:74) 版本 1：pageable（普通 malloc）。
`[B4-T18]` (main.cu:76) TODO [必做] 步骤 1：用 malloc 分配主机内存并计时 H2D。
`[B4-T19]` (main.cu:81) TODO [必做] `float* h_pageable = (float*)std::malloc(DATA_SZ);`。
`[B4-T20]` (main.cu:82) TODO [必做] 初始化（防止 lazy allocation）。
`[B4-T21]` (main.cu:83) TODO [必做] `std::memset(h_pageable, 0, DATA_SZ);`。
`[B4-T22]` (main.cu:84) TODO [必做] `gbps_pageable = measure_h2d_gbps(h_pageable, d_buf, DATA_SZ, RUNS);`。
`[B4-T23]` (main.cu:85) TODO [必做] `std::free(h_pageable);`。
`[B4-T26]` (main.cu:97) 版本 2：pinned（cudaMallocHost）。
`[B4-T27]` (main.cu:99) TODO [必做] 步骤 3：用 cudaMallocHost 分配并计时。
`[B4-T28]` (main.cu:104) TODO [必做] `CUDA_CHECK(cudaMallocHost(&h_pinned, DATA_SZ));`。
`[B4-T29]` (main.cu:105) TODO [必做] `std::memset(h_pinned, 0, DATA_SZ);`。
`[B4-T30]` (main.cu:106) TODO [必做] `gbps_pinned = measure_h2d_gbps(h_pinned, d_buf, DATA_SZ, RUNS);`。
`[B4-T31]` (main.cu:107) TODO [必做] `CUDA_CHECK(cudaFreeHost(h_pinned));`。
`[B4-T33]` (main.cu:118) 版本 3：UVM（cudaMallocManaged）。
`[B4-T34]` (main.cu:120) TODO [必做] 步骤 5：用 cudaMallocManaged 并测量。
`[B4-T35]` (main.cu:125) TODO [必做] `CUDA_CHECK(cudaMallocManaged(&um_data, DATA_SZ));`。
`[B4-T36]` (main.cu:126) TODO [必做] host 端 memset 初始化。
`[B4-T37]` (main.cu:127) TODO [必做] 方式 A：lazy。
`[B4-T38]` (main.cu:128) TODO [必做] 方式 B：显式预取。
`[B4-T39]` (main.cu:129) TODO [必做] `int dev; CUDA_CHECK(cudaGetDevice(&dev));`。
`[B4-T40]` (main.cu:130) TODO [必做] `CUDA_CHECK(cudaMemPrefetchAsync(um_data, DATA_SZ, dev));`。
`[B4-T41]` (main.cu:131) TODO [必做] `CUDA_CHECK(cudaFree(um_data));`。
`[B4-T46]` (main.cu:163) TODO [必做] 步骤 7：运行 Nsight Systems。

1. 写一个程序，分配 256 MB 数据，进行 host → device 的 memcpy，版本 1 用普通 malloc（pageable）：

```cpp
// TODO [必做] // 版本1：普通 malloc（pageable）
// TODO [必做] float *h_data = (float *)malloc(size);
// TODO [必做] cudaMemcpy(d_data, h_data, size, cudaMemcpyHostToDevice);
```

2. 计时该版本的 memcpy，计算吞吐（应该约 6–10 GB/s）。
3. 写版本2：用 `cudaMallocHost` 分配 pinned 内存：

```cpp
// TODO [必做] float *h_data_pinned;
// TODO [必做] CUDA_CHECK(cudaMallocHost(&h_data_pinned, size));
// TODO [必做] cudaMemcpy(d_data, h_data_pinned, size, cudaMemcpyHostToDevice);
```

4. 计时该版本，应该显著更快（接近 PCIe 理论峰值）。
5. 写版本3：用 `cudaMallocManaged` 分配 UVM：

```cpp
// TODO [必做] float *um_data;
// TODO [必做] CUDA_CHECK(cudaMallocManaged(&um_data, size));
// TODO [必做] // 显式预取：cudaMemPrefetchAsync(um_data, size, device_id);
```

6. 对三个版本的吞吐进行对比，记录数值。
7. 用 Nsight Systems 观察三个版本的 PCIe 传输时间线：

```
// TODO [必做] nsys profile --trace cuda ./B4_pinned_vs_pageable_and_unified.exe
```

## 进阶任务

- 在 UVM 版本中，加入 `cudaMemPrefetchAsync`，预取数据到 GPU。观察性能提升。
- 对比 `cudaMemcpy` 和 `cudaMemcpyAsync`（异步版本，可以和 kernel 并发）。
- 在多 GPU 系统上试用 UVM 的 peer-access 功能。

## 验收点

`[B4-T04]` (main.cu:21) 常量段。
`[B4-T05]` (main.cu:23) DATA_MB 传输大小。
`[B4-T06]` (main.cu:24) DATA_SZ 字节数。
`[B4-T07]` (main.cu:25) RUNS 重复次数。
`[B4-T08]` (main.cu:30) 占位 kernel。
`[B4-T09]` (main.cu:31) 仅用于触发 GPU context 初始化。
`[B4-T10]` (main.cu:34) 计算 H2D 吞吐。
`[B4-T11]` (main.cu:37) 预热段。
`[B4-T12]` (main.cu:50) 公式 GB/s。
`[B4-T13]` (main.cu:55) main 入口。
`[B4-T15]` (main.cu:64) GPU warmup。
`[B4-T16]` (main.cu:68) 分配 device 端目标缓冲区。
`[B4-T24]` (main.cu:86) 强制物理分配。
`[B4-T42]` (main.cu:133) 预取方式（取消注释以启用）。
`[B4-T43]` (main.cu:135) UVM 没有传统 cudaMemcpy 计时方式；用 event 包裹 prefetch 来估计。
`[B4-T45]` (main.cu:155) 汇总对比段。

- 三个版本都编译通过，运行正确。
- pinned 版本吞吐至少 2 倍于 pageable，接近 PCIe 峰值。
- UVM 如果配合 `cudaMemPrefetchAsync`，吞吐接近 pinned。
- Nsight Systems 显示的时间线与吞吐数值一致。

## 观察点

- pageable memory（普通 malloc）在 GPU 访问时需要操作系统参与，往往需要额外的临时 copy。
- pinned memory（`cudaMallocHost`）被锁定在物理内存，GPU 可以直接用 DMA 访问，更快。
- UVM（`cudaMallocManaged`）虚拟统一寻址，host 和 device 可以用同一指针访问同一数据，但搬运是隐式的（可能慢）。
- `cudaMemPrefetchAsync` 显式预取数据到 GPU，避免 lazy 搬运的延迟。

## 常见坑

1. **pinned 内存过多导致系统变慢**：pinned 内存无法被操作系统分页，大量占用会影响系统稳定性。一般不超过系统 RAM 的 50%。
2. **UVM 误以为是"魔法"**：UVM 很方便但性能往往差于显式 cudaMemcpy + pinned memory。除非配合 `cudaMemPrefetchAsync`。
3. **没有释放 pinned 内存**：必须用 `cudaFreeHost`，而不是普通 `free`。
4. **memcpy 计时包括了 malloc/free 时间**：应该只计时 memcpy 本身（本例使用 `CudaEventTimer`）。
5. **在 memcpy 前没有 device warmup**：第一次 GPU 访问可能慢。至少跑一个 dummy kernel。

## 提示

- 计算 memcpy 吞吐：`(size_in_bytes) / time_in_ms / 1e6 = GB/s`。
- PCIe 3.0 理论峰值约 16 GB/s（双向），实际通常 70–90% 利用率。
- `cudaMemPrefetchAsync` 应该在 kernel 启动前调用，异步预取。
- 用 `cudaDeviceSynchronize()` 确保传输完成再计时结束。

## 复盘问题

1. pinned 内存为什么比 pageable 快？
2. UVM 的"lazy paging"是什么？它有什么优缺点？
3. `cudaMemPrefetchAsync` 在什么情况下有效？
4. 三种方式的适用场景各是什么？
5. 如果数据只需要读一次，用哪种方式最快？

## 对应官方参考

- CUDA Runtime API / cudaMallocHost / cudaMallocManaged: https://docs.nvidia.com/cuda/cuda-runtime-api/
- CUDA Programming Guide / Unified Memory: https://docs.nvidia.com/cuda/cuda-c-programming-guide/
- Nsight Systems: https://docs.nvidia.com/nsight-systems/

## 输出对照（printf / std::puts 原文）

- `[B4-T14]` (main.cu:60) 原文：`[B4] 数据大小: %zu MB  重复次数: %d` -> 现：`[B4] data size: %zu MB  runs: %d`
- `[B4-T25]` (main.cu:88) 原文：`[B4] pageable H2D:  %.1f GB/s` -> 现：保持英文不变
- `[B4-T32]` (main.cu:109) 原文：`[B4] pinned H2D:    %.1f GB/s` -> 现：保持英文不变
- `[B4-T44]` (main.cu:152) 原文：`[B4] UVM+prefetch H2D: %.1f GB/s` -> 现：保持英文不变
- `[B4-T47]` (main.cu:165) 原文：`观察三个版本的 PCIe 传输时间线。` -> 现：`Inspect the PCIe transfer timeline for the three versions.`
