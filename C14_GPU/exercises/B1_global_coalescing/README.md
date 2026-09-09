# 练习 B1：global_coalescing

## 目标

`[B1-T01]` (main.cu:2) 练习 B1：global_coalescing。
`[B1-T02]` (main.cu:3) 观察合并访问（coalescing）对全局内存吞吐的影响：对比顺序访问与步长访问的带宽差异，以及 AoS vs SoA 布局。
`[B1-T03]` (main.cu:5) 验收：顺序吞吐 >> stride-32 吞吐（5-10 倍以上）；Nsight Compute 命令 `ncu --set memory_l1_l2 -o b1.ncu-rep ./B1_global_coalescing.exe`。

观察合并访问（coalescing）对全局内存吞吐的影响。对比顺序访问、步长访问、随机访问的带宽差异。用 Nsight Compute 的 Memory Workload Analysis 确认。

## 前置理解

- 完成 A1–A2。
- 你知道 GPU 内存事务是以 128 字节为单位的。
- 接受"同一 warp 的线程访问相邻地址能合并，不相邻则分散"。

## 必做任务

`[B1-T10]` (main.cu:46) Kernel 1：顺序合并读（coalesced）。
`[B1-T11]` (main.cu:48) TODO [必做] 步骤 1：实现顺序读 kernel。
`[B1-T12]` (main.cu:55) TODO [必做] `int idx = blockIdx.x * blockDim.x + threadIdx.x;`。
`[B1-T13]` (main.cu:56) TODO [必做] `if (idx < n) { data_out[idx] = data_in[idx]; }`。
`[B1-T14]` (main.cu:60) Kernel 2：步长-32 访问（不合并，分散）。
`[B1-T15]` (main.cu:62) TODO [必做] 步骤 3：实现 stride-32 读 kernel。
`[B1-T16]` (main.cu:69) TODO [必做] `int idx = (blockIdx.x * blockDim.x + threadIdx.x) * 32;`。
`[B1-T17]` (main.cu:70) TODO [必做] `if (idx < n) { data_out[idx] = data_in[idx]; }`。
`[B1-T18]` (main.cu:74) Kernel 3：AoS 读（只取 .x 字段，非合并）。
`[B1-T19]` (main.cu:76) TODO [必做] 步骤 7（AoS）：warp 内地址间距 = sizeof(ParticleAoS) = 16 字节，非合并。
`[B1-T20]` (main.cu:84) TODO [必做] `out[idx] = particles[idx].x;`。
`[B1-T21]` (main.cu:88) Kernel 4：SoA 读（只取 .x 字段，合并）。
`[B1-T22]` (main.cu:90) TODO [必做] 步骤 7（SoA）：warp 内线程访问 x[i], x[i+1] ... 连续，合并。
`[B1-T23]` (main.cu:99) TODO [必做] `out[idx] = soa_x[idx];`。
`[B1-T35]` (main.cu:174) TODO [必做] 步骤 2：填写 `coalesce_read_sequential` 并计时。
`[B1-T39]` (main.cu:184) TODO [必做] 步骤 4：填写 `coalesce_read_strided` 并计时。
`[B1-T42]` (main.cu:194) TODO [必做] 步骤 5：记录并对比 seq/stride 比值。
`[B1-T45]` (main.cu:200) TODO [必做] 步骤 7：AoS vs SoA 对比。
`[B1-T51]` (main.cu:226) TODO [必做] 步骤 6：运行 Nsight Compute 查看带宽指标。

1. 写一个 kernel `coalesce_read_sequential`，让 N 个线程顺序读一个数组：

```cpp
// TODO [必做] int idx = blockIdx.x * blockDim.x + threadIdx.x;
// TODO [必做] if (idx < N) { data_out[idx] = data_in[idx]; }  // 合并读
```

2. 分配一个约 64 MB 的 device 数组（16M float），启动足够的 block/thread 使得所有元素都被读。用 `CudaEventTimer` 计时，计算吞吐：`GBps = (N * sizeof(float) * 2) / time_ms / 1e6`。

```cpp
// TODO [必做] 用 timer.cuh 中的 CudaEventTimer 计时，打印吞吐 GB/s
```

3. 写一个 kernel `coalesce_read_strided`，使用 stride-32 访问（warp 内线程跳跃访问）：

```cpp
// TODO [必做] int idx = (blockIdx.x * blockDim.x + threadIdx.x) * 32;
// TODO [必做] if (idx < N) { data_out[idx] = data_in[idx]; }  // 不合并，分散
```

4. 用相同输入大小和 kernel 配置，计时该版本的吞吐。
5. 记录两个版本的 GB/s 值，对比（stride 版本应该低 5–10 倍）。
6. 用 Nsight Compute 查看两个版本的 L1 cache 命中率、L2 miss 率、HBM 带宽利用率：

```
// TODO [必做] ncu --set memory_l1_l2 -o b1.ncu-rep ./B1_global_coalescing.exe
```

7. 在同一个程序中，添加 AoS vs SoA 对比：AoS（Array of Structures，交错存储）vs SoA（Structure of Arrays，分离存储）。观察哪种访问模式更合并。

## 进阶任务

`[B1-T24]` (main.cu:107) 进阶：随机访问 kernel。
`[B1-T25]` (main.cu:109) TODO [进阶] 实现随机访问 kernel，观察吞吐下降。

- 写一个 `coalesce_read_random` 版本，随机访问数组（用 `threadIdx.x * 12345 % N` 之类）。观察吞吐掉多少。
- 对比 float（4B）和 float4（16B）访问的合并特性。
- 在 kernel 中加入 `NVTX_RANGE("coalesce")`，用 Nsight Systems 查看 kernel 的时间线。

## 验收点

`[B1-T04]` (main.cu:21) 常量。
`[B1-T05]` (main.cu:25) WARMUP 预热次数。
`[B1-T06]` (main.cu:27) AoS（Array of Structures）布局。
`[B1-T07]` (main.cu:31) AoS 交错存储，访问 .x 时步长为 sizeof(ParticleAoS)=16。
`[B1-T08]` (main.cu:34) SoA（Structure of Arrays）布局。
`[B1-T09]` (main.cu:38) SoA 所有粒子的 x 连续存放，合并访问。
`[B1-T26]` (main.cu:118) 计时辅助：运行 N_RUNS 次返回平均 GB/s。
`[B1-T27]` (main.cu:124) 预热阶段。
`[B1-T28]` (main.cu:133) 公式 GB/s = bytes / ms / 1e6。
`[B1-T29]` (main.cu:138) main 入口。
`[B1-T31]` (main.cu:148) 分配设备内存。
`[B1-T32]` (main.cu:154) 初始化输入。
`[B1-T33]` (main.cu:165) stride-32 grid 较小。
`[B1-T34]` (main.cu:172) 顺序访问计时。
`[B1-T36]` (main.cu:179) 读+写算 2 倍字节。
`[B1-T38]` (main.cu:183) stride-32 计时。
`[B1-T40]` (main.cu:187) stride-32 只访问 N_ELEM/32 个元素。
`[B1-T44]` (main.cu:199) AoS vs SoA 对比段。
`[B1-T46]` (main.cu:206) 粒子数。
`[B1-T50]` (main.cu:223) Nsight Compute 提示段。

- 顺序访问的吞吐至少达到理论峰值带宽的 70%（`print_device_info()` 会打印峰值）。
- stride-32 版本的吞吐掉到顺序版本的 10% 以下。
- Nsight Compute 显示顺序版本的 L1 命中率 > 80%，stride 版本 < 20%。
- 程序运行无错。

## 观察点

- 合并访问使得同一 warp 的 32 个线程的读写能压缩成少量内存事务（往往 1–2 个 128B 事务）。
- 分散访问导致每个线程的读写几乎都需要一个单独事务，带宽掉到理论值的 1/32。
- L1 缓存命中率是指标，但 GPU 缓存不像 CPU 那样重要（因为 GPU 依赖吞吐而非低延迟）。
- AoS 和 SoA 的本质是"如何在内存中排列数据以对齐硬件访问边界"。

## 常见坑

1. **没有足够的线程覆盖数据**：如果启动的线程数 < 数据元素数，某些元素没被访问，计算的吞吐虚高。
2. **计时包括了 memcpy**：如果不分开计时 memcpy 和 kernel，吞吐会被 PCIe 传输拉低。用 `CudaEventTimer` 只计时 kernel 本身。
3. **没有 warmup**：第一次 kernel launch 通常较慢（JIT 或 cache 预热）。至少运行 3 次再计时。
4. **忽视了 warp 大小**：GPU warp 是 32 个线程，一次合并访问的粒度就是一个 warp。block size 如果不是 32 的倍数，可能造成部分浪费。

## 提示

- 计算吞吐的标准公式：`字节数 / 执行时间(ms) / 1e6 = GB/s`（读+写算 2 倍字节）。
- Nsight Compute 命令：`ncu --set full -o result.ncu-rep ./exe`（详细模式并保存为 GUI 可读的 .ncu-rep 文件）。
- Bank ID 公式（全局内存事务）：`transaction_id = address / 128`，同一 warp 的线程访问同一 128B 对齐块 -> 合并。
- 如果只关心带宽，`ncu --set memory_l1_l2` 是最快的选项。

## 复盘问题

1. 为什么说 128 字节是合并的单位？128 字节能装多少个 float？
2. 如果一个 warp 的 32 个线程访问 32 个不相邻的 float，需要多少个 128B 事务？
3. stride-1 和 stride-32 访问在 warp 内的线程编号上有什么差别？
4. AoS 和 SoA 哪种更容易 coalesce？为什么？
5. Nsight Compute 里的"Memory Workload Analysis"怎么看出合并情况？

## 对应官方参考

- CUDA C++ Best Practices Guide / Global Memory: https://docs.nvidia.com/cuda/cuda-c-best-practices-guide/
- Nsight Compute: https://docs.nvidia.com/nsight-compute/
- cuda-samples `bandwidthTest`: https://github.com/NVIDIA/cuda-samples

## 输出对照（printf / std::puts 原文）

- `[B1-T30]` (main.cu:142) 原文：`[B1] 理论峰值带宽: %d GB/s` -> 现：`[B1] theoretical peak bandwidth: %d GB/s`
- `[B1-T37]` (main.cu:177) 原文：`[B1] 顺序访问吞吐:   %.1f GB/s` -> 现：`[B1] sequential throughput:  %.1f GB/s`
- `[B1-T41]` (main.cu:190) 原文：`[B1] stride-32 吞吐: %.1f GB/s` -> 现：`[B1] stride-32 throughput:   %.1f GB/s`
- `[B1-T43]` (main.cu:195) 原文：`[B1] 顺序/步长 比值:  %.1fx` 与 `[B1] 顺序 vs 峰值: %.0f%%` -> 现：`[B1] seq / stride ratio:     %.1fx` 与 `[B1] seq vs peak:            %.0f%%`
- `[B1-T47]` (main.cu:218) 原文：`[B1] AoS 读 .x 吞吐: %.1f GB/s` -> 现：`[B1] AoS read .x throughput: %.1f GB/s`
- `[B1-T48]` (main.cu:220) 原文：`[B1] SoA 读 .x 吞吐: %.1f GB/s` -> 现：`[B1] SoA read .x throughput: %.1f GB/s`
- `[B1-T49]` (main.cu:222) 原文：`[B1] SoA/AoS 比值:   %.1fx` -> 现：`[B1] SoA / AoS ratio:        %.1fx`
- `[B1-T52]` (main.cu:230) 原文：`观察 L1 命中率、L2 miss 率、HBM 带宽利用率。` -> 现：`Inspect L1 hit rate, L2 miss rate, HBM bandwidth utilization.`
