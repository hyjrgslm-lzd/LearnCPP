# 练习 B2：shared_memory_basics

## 目标

`[B2-T01]` (main.cu:2) 练习 B2：shared_memory_basics。
`[B2-T02]` (main.cu:3) 学会在 block 内声明和使用 shared memory：对比静态/动态 shared memory；写 block 级 reduce kernel。
`[B2-T03]` (main.cu:5) 验收：shared memory 版 reduce 吞吐 > 全局内存版；Nsight Compute 命令 `ncu --set full -o b2.ncu-rep ./B2_shared_memory_basics.exe`。

学会在 block 内声明和使用 shared memory。理解静态和动态 shared memory 的差异。写一个在 shared memory 上做 reduce 的 kernel，观察带宽和延迟的改善。

## 前置理解

- 完成 B1。
- 你知道 shared memory 是 block 内的快速 on-chip 存储。
- 接受 shared memory 访问需要 bank 意识（后面 B3 详细讲）。

## 必做任务

`[B2-T06]` (main.cu:29) Kernel 1：纯全局内存 reduce（baseline）。
`[B2-T07]` (main.cu:31) TODO [必做] 步骤 1：实现 `reduce_global_only`，每线程读一个元素，atomicAdd 到 output[blockIdx.x]。
`[B2-T08]` (main.cu:39) TODO [必做] `int idx = blockIdx.x * blockDim.x + threadIdx.x;`。
`[B2-T09]` (main.cu:40) TODO [必做] 用寄存器累加，再 atomicAdd 到 output[blockIdx.x]。
`[B2-T10]` (main.cu:46) Kernel 2：shared memory reduce（动态 smem）。
`[B2-T11]` (main.cu:48) TODO [必做] 步骤 3：实现 `reduce_with_smem`：coalesced load -> __syncthreads -> tree-reduce -> 写回。
`[B2-T13]` (main.cu:62) TODO [必做] `sdata[tid] = (idx < n) ? input[idx] : 0;`。
`[B2-T14]` (main.cu:63) TODO [必做] `__syncthreads();`。
`[B2-T16]` (main.cu:69) TODO [必做] tree-reduce 循环：`for (int s = blockDim.x/2; s > 0; s >>= 1) ...`。
`[B2-T17]` (main.cu:75) TODO [必做] `if (tid == 0) output[blockIdx.x] = sdata[0];`。
`[B2-T28]` (main.cu:163) TODO [必做] 步骤 2：计时全局内存版本。
`[B2-T33]` (main.cu:188) TODO [必做] 步骤 4：在 launch 第三个参数指定动态 smem 字节数。
`[B2-T36]` (main.cu:213) TODO [必做] 步骤 7：运行 Nsight Compute。

1. 写一个 kernel `reduce_global_only`，用全局内存做 reduce（baseline）：

```cpp
// TODO [必做] __global__ void reduce_global_only(const int *input, int *output, int n) {
//     // 每线程读一个元素到寄存器，累加
//     // 结果 atomicAdd 到 output[blockIdx.x]
// }
```

2. 启动 kernel 处理 1M 个整数，计时，计算吞吐。
3. 写一个 kernel `reduce_with_smem`，把输入加载到 shared memory，再在 shared memory 里累加：

```cpp
// TODO [必做] // 第一步：coalesced global read 到 shared memory
// TODO [必做] sdata[threadIdx.x] = input[idx];
// TODO [必做] __syncthreads();
// TODO [必做] // 第二步：在 shared memory 里 tree-reduce
// TODO [必做] //   for (int s = blockDim.x/2; s > 0; s >>= 1) {
// TODO [必做] //       if (tid < s) sdata[tid] += sdata[tid + s];
// TODO [必做] //       __syncthreads();
// TODO [必做] //   }
// TODO [必做] // 第三步：一个线程把结果写回全局内存
```

4. 在 launch 时指定动态 shared memory 大小：

```cpp
// TODO [必做] kernel<<<grid, block, block.x * sizeof(int)>>>();
```

5. 用相同输入大小和配置，计时该版本。
6. 对比全局 only 和 shared memory 版本的吞吐。shared memory 版本应该快 2–5 倍。
7. 在两个版本上运行 Nsight Compute，对比 achieved occupancy、寄存器使用、shared memory 的利用率。

## 进阶任务

`[B2-T18]` (main.cu:81) Kernel 3：静态 shared memory 版本（进阶对比）。
`[B2-T19]` (main.cu:83) TODO [进阶] 实现静态 shared memory 版本并对比性能。
`[B2-T20]` (main.cu:88) 静态声明 `__shared__ int sdata[BLOCK_SZ]`，大小编译期确定。

- 实现一个 reduce 的完整版本（分步：第一步各线程读一个元素，第二步 stride=1 循环相邻累加，第三步结果写回），并在 shared memory 里完成。
- 对比静态和动态 shared memory 的声明方式和性能（通常无差异）。
- 在 kernel 中用 `printf` 打印 blockDim、gridDim、occupied smem，观察资源占用。

## 验收点

`[B2-T04]` (main.cu:21) 常量段。
`[B2-T05]` (main.cu:23) N_ELEM = 1M 整数。
`[B2-T12]` (main.cu:59) `extern __shared__ int sdata[]` 大小由 launch 指定。
`[B2-T15]` (main.cu:66) 占位符（学生填写后替换）。
`[B2-T21]` (main.cu:104) host 端：对 output 数组做最终求和。
`[B2-T22]` (main.cu:113) 计时辅助。
`[B2-T23]` (main.cu:131) main 入口。
`[B2-T24]` (main.cu:140) 分配 host 数据。
`[B2-T25]` (main.cu:144) 全 1 输入，期望 reduce 结果 = N_ELEM。
`[B2-T26]` (main.cu:149) 分配设备内存。
`[B2-T27]` (main.cu:158) Part 1：全局内存 reduce baseline。
`[B2-T29]` (main.cu:168) 只读，无写回（简化计算）。
`[B2-T30]` (main.cu:171) 取回结果验证。
`[B2-T32]` (main.cu:182) Part 2：shared memory reduce。
`[B2-T35]` (main.cu:206) Nsight Compute 提示段。
`[B2-T38]` (main.cu:218) 清理段。

- 两个版本都编译通过，运行结果正确（reduce 结果等于 N_ELEM = 1M）。
- shared memory 版本的吞吐高于全局 only 版本。
- Nsight Compute 显示 shared memory 版本的 L1 hit rate 显著提升。
- 程序无越界或同步错误。

## 观察点

- shared memory 是 block 内的快速存储（Hopper 最多 228 KB/SM），延迟约 20 ns（vs global 约 200 ns）。
- 加载到 shared memory 需要一次 coalesced global read，但随后的访问都很快。
- shared memory 被 block 内所有线程共享，访问需要谨慎（避免 bank conflict，见 B3）。
- 动态 shared memory 大小在 launch 时通过第三个参数 `<<<grid, block, smem_bytes>>>` 指定。

## 常见坑

1. **忘记 `__syncthreads()`**：写到 shared memory 后，其他线程读之前必须 sync，否则读到旧值。
2. **动态 shared memory 大小指定错误**：launch 时的 `smem_bytes` 和 kernel 内 extern 的使用必须匹配。
3. **shared memory 溢出**：shared memory 最多 96 KB / block（opt-in 最大值见 `print_device_info()` 输出）。
4. **没有初始化 shared memory**：共享内存初值不确定。需要显式写入后再读取。
5. **跨 block 访问 shared memory**：shared memory 只在 block 内有效，其他 block 无法访问。

## 提示

- `extern __shared__ int sdata[]` 声明一个大小动态的共享数组。
- `__syncthreads()` 是 block 级 barrier，所有线程必须到达才能继续。不要在分支中调用。
- shared memory 的 bank conflict 规则见 B3，这里先用简单的访问模式（如 stride-1）避免。
- 计算 shared memory 需求：`smem_bytes = blockDim.x * sizeof(datatype)`。

## 复盘问题

1. 动态和静态 shared memory 的差异是什么？
2. 为什么说 `__syncthreads()` 必须在所有线程都能到达的地方？
3. shared memory 和 L1 cache 在硬件上是同一块存储吗？
4. 如果 block 大小是 256，shared memory 声明为 1 MB，会发生什么？
5. 在 shared memory 上做 reduce 为什么比全局内存快？

## 对应官方参考

- CUDA C++ Programming Guide / Shared Memory: https://docs.nvidia.com/cuda/cuda-c-programming-guide/
- cuda-samples `reduction`: https://github.com/NVIDIA/cuda-samples

## 输出对照（printf / std::puts 原文）

- `[B2-T31]` (main.cu:175) 原文：`[B2] 全局 reduce: %.1f GB/s  result=%d  %s` 与 `WRONG(TODO 未填写？)` -> 现：`[B2] global reduce: %.1f GB/s  result=%d  %s` 与 `WRONG(TODO unfilled?)`
- `[B2-T34]` (main.cu:199) 原文：`[B2] SMEM reduce:  %.1f GB/s  result=%d  %s` 与 `WRONG(TODO 未填写？)` -> 现：`[B2] SMEM reduce:   %.1f GB/s  result=%d  %s` 与 `WRONG(TODO unfilled?)`
- `[B2-T37]` (main.cu:215) 原文：`对比两个 kernel 的 achieved occupancy、smem 利用率。` -> 现：`Compare achieved occupancy and smem utilization between the two kernels.`
