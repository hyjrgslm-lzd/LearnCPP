# 练习 A1：hello_device

## 目标

`[A1-T01]` (main.cu:2) 练习 A1：hello_device。
`[A1-T02]` (main.cu:3) 写出第一个 kernel：感受 host/device 代码分离，
`[A1-T03]` (main.cu:4) 以及 kernel launch 的最小形式。

写出第一个 kernel：单线程在 device 上打印 `blockIdx` 和 `threadIdx`。感受 host/device 代码分离，以及 kernel launch 的最小形式。

## 前置理解

- 你知道 C 的 `printf` 怎么用。
- 你接受 CUDA kernel 就是一个"在 GPU 上跑很多次"的函数。
- 你能区分 host 代码（在 CPU 上执行）和 device 代码（在 GPU 上执行）。

## 必做任务

`[A1-T06]` (main.cu:27) 1. 定义 `hello_kernel`。
`[A1-T07]` (main.cu:29) TODO [必做] 步骤 1：用 `__global__` 修饰下面的函数，使其成为一个 kernel。
`[A1-T08]` (main.cu:31) TODO [必做] 步骤 2：在函数体内用 `printf` 打印
  `"Device: blockIdx=(%d,%d,%d), threadIdx=(%d,%d,%d)\n"`，以及对应的 `blockIdx.x/y/z` 和 `threadIdx.x/y/z`。
`[A1-T09]` (main.cu:38) TODO [必做] kernel 函数体内的 `printf` 语句模板。
`[A1-T17]` (main.cu:81) TODO [必做] 步骤 3：在此处 `launch hello_kernel<<<1, 1>>>()`，grid 和 block 都是 1D，大小都是 1。
`[A1-T19]` (main.cu:91) TODO [必做] 在 launch 处实际调用 `hello_kernel<<<grid, block>>>();`。
`[A1-T20]` (main.cu:94) TODO [必做] 步骤 4：用 `CUDA_CHECK(cudaGetLastError())` 检查启动错误。
`[A1-T21]` (main.cu:98) TODO [必做] 步骤 4：用 `CUDA_CHECK(cudaDeviceSynchronize())` 等待完成。

1. 在 `main.cu` 中，用 `__global__` 修饰函数 `hello_kernel()`。
2. 在 kernel 内部，用 `printf` 打印 `"Device: blockIdx=(%d,%d,%d), threadIdx=(%d,%d,%d)\n"` 和对应值。
3. 在 `main()` 里调用 `hello_kernel<<<1, 1>>>()`（grid 和 block 都是 1D，大小都是 1）。
4. 用 `CUDA_CHECK(cudaDeviceSynchronize())` 等待 kernel 完成，再打印 host 侧完成信息。
5. 编译并运行，观察输出。

```cpp
// TODO [必做] 在这里定义 __global__ hello_kernel()，内部调用 printf
// TODO [必做] 在 main() 里 hello_kernel<<<1,1>>>() 并 sync
```

## 进阶任务

`[A1-T04]` (main.cu:14) 辅助：计算全局线程 ID（进阶任务示例，供学生参考或填写）。
`[A1-T05]` (main.cu:18) TODO [进阶] 把下面的 `__device__` 函数补全，在 `hello_kernel` 里调用。
`[A1-T10]` (main.cu:53) 进阶：带 shared memory 的版本骨架。
`[A1-T11]` (main.cu:55) TODO [进阶] 增加一个 `hello_kernel_shared`：声明 `__shared__` 数组，每个线程写入自己的 `threadIdx.x`，`__syncthreads()` 后再读取并打印。
`[A1-T12]` (main.cu:60) 假设 block ≤ 32 线程。
`[A1-T13]` (main.cu:64) 只让 thread 0 打印。
`[A1-T23]` (main.cu:106) 进阶：2×2 grid，2×2 block（总 16 个线程）。
`[A1-T24]` (main.cu:108) TODO [进阶] 把下面的 `#if 0` 改为 `#if 1`，观察 16 个线程的输出。
`[A1-T25]` (main.cu:114) 4 个 block。
`[A1-T26]` (main.cu:116) 每 block 4 个线程 ⇒ 总 16 线程。
`[A1-T29]` (main.cu:131) 进阶：shared memory 版本。
`[A1-T30]` (main.cu:133) TODO [进阶] 把下面的 `#if 0` 改为 `#if 1`。

- 把 kernel 改成 `hello_kernel<<<2, 2>>>`（grid 2×2，block 2×2），观察有多少个线程实际运行。
- 增加一个 `__device__` 辅助函数，计算全局线程编号 `gid = blockIdx.x * blockDim.x + threadIdx.x`，在 kernel 里调用。
- 在 kernel 里增加一个 `__shared__` 数组，让同一 block 的线程写入各自的值，再用 `__syncthreads()` 读回并 print（体会 shared memory 的概念）。

## 验收点

`[A1-T14]` (main.cu:73) `main` 入口。
`[A1-T15]` (main.cu:78) 设备信息。
`[A1-T16]` (main.cu:80) 步骤 3：最小 launch。
`[A1-T18]` (main.cu:84) launch config。
`[A1-T32]` (main.cu:147) 验收断言。
`[A1-T33]` (main.cu:148) 程序能运行到此处说明无 CUDA runtime error。

- 程序编译通过，运行时没有 CUDA runtime error。
- `printf` 输出被完整捕获（没有因为 buffer limit 被截断）。
- 你能从输出中确认有多少个线程执行了 kernel（线程数 = `gridDim × blockDim`）。
- `cudaDeviceSynchronize()` 后输出了"kernel 完成"的消息。

## 观察点

- 一个 kernel launch（`<<<1,1>>>`）不等于"一个线程"——它是"启动参数"，告诉 GPU 要生成多少个线程。
- `printf` 从 device 输出时，可能被缓冲；`cudaDeviceSynchronize()` 才能保证之前的输出被 flush。
- `blockIdx` 和 `threadIdx` 都是 `uint3` 类型（3D 坐标），即使 kernel 是 1D，也可以访问 `.x` / `.y` / `.z`（后两个为 0）。

## 常见坑

1. **忘记 `cudaDeviceSynchronize()`**：kernel 是异步启动的，host 代码立刻继续执行。如果不 sync，可能 host 侧 printf 比 device 侧先输出，或者根本看不到 device 输出。
2. **`printf` 行数限制**：GPU printf buffer 有限（通常 1 MB），超过会丢失。这题输出少，但后续练习要注意。
3. **kernel 执行出错但没报错**：CUDA runtime 不会主动 throw exception。必须用 `CUDA_CHECK` 宏包裹每个 API 调用，或者用 `cudaGetLastError()` 显式查询。
4. **混淆 grid/block 维度**：`<<<gridDim, blockDim>>>` 里的两个数字分别代表 grid 大小和 block 大小，都可以是 1D/2D/3D（`dim3` 类型）。这题用标量（自动转为 `dim3(x,1,1)`）。

## 提示

- `dim3` 是 CUDA 的 3D 坐标类型。`dim3(2)` 等同 `dim3(2, 1, 1)`。
- `blockIdx` 和 `threadIdx` 在 kernel 内部是内置变量，无需显式参数传入。
- 想看 device 端崩溃信息，用 `CUDA_CHECK(cudaGetLastError())` 在 launch 后立刻查询。
- 如果怀疑 kernel 没有执行，可以在 host 端和 device 端各打一条独特的 printf，对比输出顺序。

## 复盘问题

1. `__global__` 和 `__device__` 的区别是什么？哪个可以从 host 调用？
2. 为什么 `hello_kernel<<<1,1>>>()` 启动 1 个线程，而 `hello_kernel<<<1,32>>>()` 启动 32 个线程？
3. 如果启动 `<<<10, 32>>>`，总共有多少个线程？它们如何分组到不同的 block？
4. `cudaDeviceSynchronize()` 的作用是什么？如果不调用会怎样？
5. device 端 `printf` 和 host 端 `printf` 有什么区别（除了执行位置）？

## 对应官方参考

- CUDA C++ Programming Guide: https://docs.nvidia.com/cuda/cuda-c-programming-guide/
- CUDA Runtime API: https://docs.nvidia.com/cuda/cuda-runtime-api/（`cudaDeviceSynchronize` / `cudaGetLastError`）
- cuda-samples: https://github.com/NVIDIA/cuda-samples（查找 `vectorAdd` 等基础例子）

## 输出对照（printf / std::puts 原文）

- `[A1-T22]` (main.cu:103) 原文：`[A1] 最小 launch (1x1) 完成` → 现：`[A1] minimal launch (1x1) done`
- `[A1-T27]` (main.cu:118) 原文：`[launch 进阶] grid=(...) block=(...)` → 现：`[launch ADV] grid=(...) block=(...)`
- `[A1-T28]` (main.cu:124) 原文：`[A1] 进阶 launch (2x2 grid, 2x2 block) 完成` → 现：`[A1] advanced launch (2x2 grid, 2x2 block) done`
- `[A1-T31]` (main.cu:142) 原文：`[A1] 进阶 launch (shared memory) 完成` → 现：`[A1] advanced launch (shared memory) done`
- `[A1-T34]` (main.cu:150) 原文：`[A1_hello_device] PASSED (no CUDA runtime error)` → 现：保持英文不变
