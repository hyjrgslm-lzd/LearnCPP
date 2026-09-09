# 练习 A2：index_mapping

## 目标

`[A2-T01]` (main.cu:2) 练习 A2：index_mapping。
`[A2-T02]` (main.cu:3) 掌握 1D/2D grid 和 block 的索引映射。
`[A2-T03]` (main.cu:4) 学会把二维问题分解为线程索引，
`[A2-T04]` (main.cu:5) 反之亦然。

掌握 1D/2D grid 和 block 的索引映射。学会把二维问题（如矩阵）分解成一维线程索引，反之亦然。

## 前置理解

- 完成了 A1。
- 你知道线性数组和二维矩阵的内存布局（行优先）。
- 接受 GPU 线程往往以 1D 线性方式分配，即使问题是 2D 的。

## 必做任务

`[A2-T09]` (main.cu:26) Kernel 1：1D 线程索引映射。
`[A2-T10]` (main.cu:28) TODO [必做] 步骤 1：实现 `map_index_1d`，计算 `idx = blockIdx.x * blockDim.x + threadIdx.x`，并在 `idx < n` 时令 `output[idx] = idx`。
`[A2-T11]` (main.cu:34) TODO [必做] kernel 内：`int idx = blockIdx.x * blockDim.x + threadIdx.x;`。
`[A2-T12]` (main.cu:35) TODO [必做] kernel 内：`if (idx < n) { output[idx] = idx; }`。
`[A2-T14]` (main.cu:41) Kernel 2：2D 线程索引映射。
`[A2-T15]` (main.cu:43) TODO [必做] 步骤 5：实现 `map_index_2d`（行优先），并加边界检查 `row < rows && col < cols`。
`[A2-T16]` (main.cu:49) TODO [必做] `int row = blockIdx.y * blockDim.y + threadIdx.y;`。
`[A2-T17]` (main.cu:50) TODO [必做] `int col = blockIdx.x * blockDim.x + threadIdx.x;`。
`[A2-T18]` (main.cu:51) TODO [必做] `if (row < rows && col < cols) {`。
`[A2-T19]` (main.cu:52) TODO [必做] `output[row * cols + col] = row * cols + col;`。
`[A2-T20]` (main.cu:53) TODO [必做] `}`。
`[A2-T28]` (main.cu:113) TODO [必做] 步骤 2：在 host 分配并零初始化输出数组。
`[A2-T31]` (main.cu:122) TODO [必做] 步骤 3：启动 `<<<32, 32>>>`（总 1024 线程）。
`[A2-T32]` (main.cu:131) TODO [必做] 步骤 4：`cudaMemcpy` 回 host，逐元素验证。
`[A2-T36]` (main.cu:159) TODO [必做] 步骤 6：用 `<<<dim3(8,4), dim3(32,8)>>>` 启动，处理 256x32 矩阵。

1. 在 `main.cu` 中，写一个 kernel `map_index_1d`，计算全局线程编号：

```cpp
// TODO [必做] int idx = blockIdx.x * blockDim.x + threadIdx.x;
// TODO [必做] 如果 idx < N，令 output[idx] = idx
```

2. 在 `main()` 里用 `cudaMalloc` 分配一个大小为 N=1024 的数组到 device。
3. 启动 kernel 为 `<<<32, 32>>>`（总 1024 个线程），传入 N=1024。
4. 用 `cudaMemcpy` 把结果复制回 host，验证每个线程都访问了对应的数组元素。

```cpp
// TODO [必做] 声明 output 设备端数组，大小 N
// TODO [必做] kernel 计算 int idx = ...; output[idx] = idx;
```

5. 再写一个 kernel `map_index_2d`，处理二维网格（如 32x32 矩阵）：

```cpp
// TODO [必做] int row = blockIdx.y * blockDim.y + threadIdx.y;
// TODO [必做] int col = blockIdx.x * blockDim.x + threadIdx.x;
// TODO [必做] int idx = row * cols + col;（行优先）
```

6. 用 `<<<dim3(8,4), dim3(32,8)>>>` 启动，处理 256x32 矩阵。验证没有越界访问。

```cpp
// TODO [必做] 检查 row < rows && col < cols，避免越界
```

## 进阶任务

`[A2-T22]` (main.cu:59) 进阶 Kernel：grid stride loop。
`[A2-T23]` (main.cu:61) TODO [进阶] 实现 `map_index_grid_stride`：一个线程处理多个元素，grid size 可以比数据小。
`[A2-T39]` (main.cu:194) TODO [进阶] grid stride loop 版本（解除 `#if 0`）。
`[A2-T40]` (main.cu:201) 启动远少于 `N_1D` 的线程，由 grid stride loop 自动覆盖。
`[A2-T41]` (main.cu:205) ... 验证 ...

- 写一个 `map_index_grid_stride` kernel，使用"grid stride loop"（loop over all elements）而不是"一线程一元素"。观察它如何处理 grid 比 data 小的情况。
- 对比一维和二维启动方式下，索引计算公式。写下两种方式的优缺点。
- 增加一个边界检查版本，处理 grid/block 过大导致索引超出 data size 的情况。

## 验收点

`[A2-T05]` (main.cu:17) 常量定义。
`[A2-T06]` (main.cu:18) `N_1D = 1024` 1D 数组大小。
`[A2-T07]` (main.cu:19) `ROWS = 256` 2D 矩阵行数。
`[A2-T08]` (main.cu:20) `COLS = 32` 2D 矩阵列数。
`[A2-T13]` (main.cu:36) 占位符（学生填写后删除）。
`[A2-T21]` (main.cu:54) 占位符（学生填写后删除）。
`[A2-T24]` (main.cu:74) 主机验证 1D 映射结果。
`[A2-T25]` (main.cu:88) 主机验证 2D 映射结果。
`[A2-T26]` (main.cu:103) `main` 入口。
`[A2-T27]` (main.cu:110) Part 1：1D 索引映射。
`[A2-T29]` (main.cu:115) host 数组初始为 0。
`[A2-T30]` (main.cu:118) device 端分配。
`[A2-T35]` (main.cu:147) Part 2：2D 索引映射 256x32 矩阵。

- 1D 版本：所有 1024 个元素被正确填充，没有越界或遗漏。
- 2D 版本：256x32 矩阵所有元素被正确映射，行列顺序无误。
- 输出数组与索引逐一核对，证明映射正确。
- 没有 CUDA runtime error。

## 观察点

- 索引映射公式 `idx = blockIdx.x * blockDim.x + threadIdx.x` 是一维线性化的核心。
- 二维映射需要同时处理 x/y（列/行），行优先 `idx = row * cols + col` 是标准约定。
- grid 和 block 的大小是独立选择的，不一定能完美覆盖数据。边界检查是必须的。
- "grid stride loop"（一个线程处理多个元素）可以消除对 grid size 的依赖，代价是每线程代码更复杂。

## 常见坑

1. **忘记边界检查**：如果 grid/block 大小不能整除数据大小，某些线程会越界。必须写 `if (idx < N)` 或 `if (row < rows && col < cols)`。
2. **行优先 vs 列优先混淆**：C/C++ 数组默认行优先。`matrix[row][col]` 对应 `flat[row * cols + col]`，不是 `row + col * rows`。
3. **blockDim 和 gridDim 位置颠倒**：`<<<gridDim, blockDim>>>` 顺序固定，反了会导致大量线程或少量线程。
4. **2D block 中只用了 x 维**：有时候 block 声明为 2D（`dim3(32, 8)`）但代码只用 `threadIdx.x`，会导致线程浪费。

## 提示

- 索引计算 `idx = blockIdx.x * blockDim.x + threadIdx.x` 是 1D 线性化的标准公式，记住它。
- 对于 2D 问题，先确定行列大小，再选择 grid/block 形状使得覆盖整个矩阵。
- 用 `__host__ __device__` 修饰辅助函数，可在 host 和 device 都调用，便于验证。
- 测试时，用一个小数据（如 16x16）快速验证正确性，再扩到大数据。

## 复盘问题

1. 如果启动 `<<<dim3(2,3), dim3(32,16)>>>`，总共生成多少个线程？
2. 对于 1024 个元素，如何选择 grid 和 block 大小？有多少种选法？
3. 为什么说"grid stride loop"比"一线程一元素"更灵活？
4. 在二维索引映射中，哪个维度对应行，哪个对应列？如果搞反了会怎样？
5. 边界检查 `if (idx < N)` 为什么不能省略？

## 对应官方参考

- CUDA C++ Programming Guide / Thread Hierarchy: https://docs.nvidia.com/cuda/cuda-c-programming-guide/
- cuda-samples `vectorAdd`: https://github.com/NVIDIA/cuda-samples

## 输出对照（printf / std::puts 原文）

- `[A2-T33]` (main.cu:138) 原文："[A2] 1D 映射验证通过" -> 现："[A2] 1D mapping verification PASSED"
- `[A2-T34]` (main.cu:142) 原文："[A2] 1D 映射验证失败（TODO 未填写？）" -> 现："[A2] 1D mapping verification FAILED (TODO not filled in?)"
- `[A2-T37]` (main.cu:179) 原文："[A2] 2D 映射验证通过" -> 现："[A2] 2D mapping verification PASSED"
- `[A2-T38]` (main.cu:182) 原文："[A2] 2D 映射验证失败（TODO 未填写？）" -> 现："[A2] 2D mapping verification FAILED (TODO not filled in?)"
