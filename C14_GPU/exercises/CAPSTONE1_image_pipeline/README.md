# 结课项目 1：数据并行管道与性能基线

> **对应文档**：`P:\C++Code\C14_GPU\08-结课项目1-数据并行管道与性能基线.md`
> **所属阶段**：Stage 1（模块 A–F 出口）
> **目录**：`exercises/CAPSTONE1_image_pipeline/`

---

## 项目目标

`[CAP1-T01]` (main.cu:2) 结课项目 1：数据并行管道与性能基线。
`[CAP1-T02]` (main.cu:3) 本文件是完整的脚手架（scaffold）；所有 kernel 体内均有 TODO 标记，学员需要填写实际算法；Host 端代码可直接编译运行；kernel 输出为全零占位；验收断言会可见地失败，提示学员填写正确实现。

这是 Stage 1 的出口。把模块 A–F 学过的所有技术串联起来，完成一条从"生成图像数据"到"输出统计结果"的端到端 GPU 管道，并用 Nsight 工具链深度剖析性能。

核心任务：**写一个多步骤的数据处理管道，每步都是独立的 kernel，用多 stream 实现步骤间的并发与数据重叠，用 CUDA Graph 打包整个管道消除 launch 开销，最后用 Nsight Systems 和 Nsight Compute 完整剖析性能瓶颈，绘制 roofline 图，给出基准数据。**

---

## 前置知识

- **必读**：模块 A–F 全部完成且理解。
- 理解 kernel fusion 动机：多个小 kernel 可能被拆成独立步骤（便于维护），也可以合并（节省 launch 开销）。
- 能用 CMake 组织 CUDA 项目。
- 有兴趣在 Nsight 中花时间搞懂性能数据。

---

## 硬件与工具链要求

| 项目 | 最低要求 | 推荐 |
|------|----------|------|
| Compute Capability | CC 8.0（Ampere） | CC 9.0a（Hopper） |
| CUDA Toolkit | 13.x | 13.x 最新 |
| GPU 显存 | 2 GB（低分辨率） | 4 GB（4K 测试） |
| Nsight Systems | `nsys` 2024.x+ | 同 |
| Nsight Compute | `ncu` 2024.x+ | 同 |
| compute-sanitizer | 随 CUDA Toolkit | 同 |

---

## 交付物清单

完成此项目后，应交付以下文件：

1. **可运行的 CUDA 代码**：`main.cu`（kernel 实现 + pipeline 组织）
2. **CMake 构建脚本**：`CMakeLists.txt`
3. **Nsight Systems 报告**：`nsight_systems_trace.qdrep`（或导出的 CSV/JSON）
4. **Nsight Compute per-kernel 报告**：5 个 `.ncu-rep` 文件（每个 kernel 一个）
5. **Roofline 图**：手工绘制或工具导出（PNG 或 PDF），标注每个 kernel 的位置
6. **观察与分析文档**：`ANALYSIS.md`（5–10 行观察 + 性能数据 + 优化建议）
7. **本 README**：`README.md`（题面复述 + 验收点 + 复盘问题）
8. **参考答案**（可选但推荐）：`reference/reference.cu`（完整实现）

---

## 综合任务：图像统计管道

### 任务概述

`[CAP1-T03]` (main.cu:8) 管道步骤：Stage 1 normalize / Stage 2 histogram / Stage 3 conv2d / Stage 4 scan / Stage 5 radix_sort。
`[CAP1-T04]` (main.cu:16) 编译命令（`cmake --preset vs2026` + `cmake --build build-vs2026 --config Release --target CAPSTONE1_image_pipeline`）。
`[CAP1-T05]` (main.cu:18) 运行示例（`--width 4096 --height 4096`，可加 `--use-graph --iters 50`）。

**场景**：从 host 端生成一张大尺寸 2D 图像（float32，分辨率 4K/8K/16K 可选，RGB 三通道），对其进行一系列数据处理操作，输出统计结果。

**关键约束**：
- 每个处理步骤用独立的 kernel
- 用多 stream 并发执行相邻步骤
- 用 CUDA Graph 打包整个管道，支持重放测峰值吞吐
- 每步用 NVTX range 标注，便于 Nsight Systems 观察
- 每步用 `cudaEvent` 计时

### 管道步骤（共 5 大步）

`[CAP1-T06]` (main.cu:38) 编译期常量段（CHANNELS / HIST_BINS / BLOCK_DIM 等）。
`[CAP1-T07]` (main.cu:40) `CHANNELS = 3` 表示 RGB 三通道。
`[CAP1-T08]` (main.cu:41) `HIST_BINS = 256` 表示直方图 bin 数。
`[CAP1-T09]` (main.cu:42) `BLOCK_DIM_X = 16`：conv2d / 通用 2D tile 宽。
`[CAP1-T10]` (main.cu:43) `BLOCK_DIM_Y = 16`：conv2d / 通用 2D tile 高。
`[CAP1-T11]` (main.cu:44) `BLOCK_1D = 256`：1D kernel 默认 block 大小。
`[CAP1-T12]` (main.cu:45) `CONV_KERNEL_R = 1`：卷积核半径，3x3 ⇒ R=1。
`[CAP1-T13]` (main.cu:47) `NORM_EPS = 1e-6f`：归一化分母保护项。
`[CAP1-T14]` (main.cu:48) `RADIX_BITS = 4`：每趟基数排序处理位数。
`[CAP1-T15]` (main.cu:51) 3x3 Gaussian 卷积核（host 端预定义，拷贝到 device constant memory 或直接传参）。
`[CAP1-T16]` (main.cu:62) CLI 参数结构体（`CliArgs`）。
`[CAP1-T17]` (main.cu:73) 各阶段性能记录结构体（`StagePerf`）。
`[CAP1-T18]` (main.cu:77) `bytes_moved` 字段：读+写字节数。
`[CAP1-T19]` (main.cu:78) `gbs` 字段：有效带宽 GB/s。
`[CAP1-T20]` (main.cu:79) `ai` 字段：算术强度 FLOP/byte（roofline 辅助函数填写）。

**Step 1：归一化（Normalize）**
- 输入：原始浮点图像（H × W × C，RGB 打包格式）
- 操作：按通道计算均值和方差（Welford 在线算法），然后 `(pixel - mean) / sqrt(var + eps)`
- 输出：归一化后的图像（同大小）
- Kernel 骨架：`normalize_kernel(float* in, float* out, int H, int W, int C, float* channel_mean, float* channel_var)`

`[CAP1-T32]` (main.cu:142) Stage 1：归一化 kernel（Welford 在线算法）头部注释。
`[CAP1-T33]` (main.cu:159) TODO [必做] 步骤 2-a：确定当前线程负责的像素坐标 (row, col, ch)。
`[CAP1-T34]` (main.cu:164) TODO [必做] 步骤 2-b：单 pass Welford reduce — 每个线程持有局部 (count, mean, M2)。
`[CAP1-T35]` (main.cu:176) TODO [必做] 步骤 2-c：`__syncthreads()` 后对每个像素执行归一化。
`[CAP1-T36]` (main.cu:180) TODO [进阶] FP16 重写：用 `__half` / `__half2` 替换 float，观察吞吐变化。
`[CAP1-T37]` (main.cu:182) 占位：输出全零（学员填写前验收断言会失败）。

**Step 2：直方图统计（Histogram）**
- 输入：归一化后的图像
- 操作：统计像素值分布，实现两版对比：
  - **V1**：朴素版，全局内存 `atomicAdd`（容易成为 bottleneck）
  - **V2**：优化版，每个 block 维护本地直方图，最后 atomic merge 到全局
- 输出：256 bin 的直方图
- Kernel 骨架：`histogram_atomic_kernel(...)` 和 `histogram_privatized_kernel(...)`

`[CAP1-T38]` (main.cu:187) Stage 2-V1：直方图 atomic 版（全局内存 atomicAdd，性能基线）头部注释。
`[CAP1-T39]` (main.cu:198) TODO [必做] 步骤 3-a：计算全局线程索引 idx。
`[CAP1-T40]` (main.cu:202) TODO [必做] 步骤 3-b：将 float 像素映射到 bin 索引。
`[CAP1-T41]` (main.cu:207) TODO [必做] 步骤 3-c：全局 atomicAdd 累加。
`[CAP1-T42]` (main.cu:216) Stage 2-V2：直方图 privatized 版（per-block shared mem + 最终 atomic merge）头部注释。
`[CAP1-T43]` (main.cu:227) TODO [必做] 步骤 3-d：在 shared memory 中声明 per-block 局部直方图。
`[CAP1-T44]` (main.cu:231) TODO [必做] 步骤 3-e：初始化 s_hist 为 0。
`[CAP1-T45]` (main.cu:236) TODO [必做] 步骤 3-f：每个线程对 s_hist 做 atomicAdd。
`[CAP1-T46]` (main.cu:238) TODO [必做] 步骤 3-g：`__syncthreads()` 后将 s_hist merge 到全局 hist。

**Step 3：2D 卷积（Convolution）**
- 输入：前一步的输出
- 操作：3×3 高斯卷积，使用 shared memory tile + halo 加载，避免 global memory 重复读取
- 输出：卷积结果（边界处理：zero-padding）
- Kernel 骨架：`conv2d_kernel(float* in, float* out, int H, int W, float* kernel_weights, int K_radius)`

`[CAP1-T47]` (main.cu:247) Stage 3：2D 卷积 kernel（3x3 Gaussian，shared-mem halo tile）头部注释。
`[CAP1-T48]` (main.cu:259) TODO [必做] 步骤 4-a：计算 tile 尺寸（含 halo）。
`[CAP1-T49]` (main.cu:266) TODO [必做] 步骤 4-b：将 halo + tile 内像素协作加载到 shared mem。
`[CAP1-T50]` (main.cu:270) TODO [必做] 步骤 4-c：`__syncthreads()` 后执行 3x3 卷积。
`[CAP1-T51]` (main.cu:278) TODO [进阶] 5x5 Gaussian：K_radius=2，调整 shared mem 大小，观察 smem 占用变化。

**Step 4：Inclusive Scan（扫描）**
- 输入：卷积结果（展平为 1D）
- 操作：计算前缀和，实现两版对比：
  - **V1**：Hillis-Steele（并行度高，O(n log n) work）
  - **V2**：Blelloch（work-efficient，O(n) work，实现更复杂）
- 输出：前缀和数组（同大小）
- Kernel 骨架：`scan_hillis_steele_kernel(...)` 和 `scan_blelloch_kernel(...)`

`[CAP1-T52]` (main.cu:284) Stage 4-V1：Hillis-Steele inclusive scan（O(n log n) work，并行度高）头部注释。
`[CAP1-T53]` (main.cu:294) TODO [必做] 步骤 5-a：每个 block 处理一段数据；这里仅示范单 block 版本。
`[CAP1-T54]` (main.cu:297) TODO [必做] 步骤 5-b：加载数据到 shared memory。
`[CAP1-T55]` (main.cu:302) TODO [必做] 步骤 5-c：Hillis-Steele 迭代。
`[CAP1-T56]` (main.cu:312) TODO [必做] 步骤 5-d：写回 out。
`[CAP1-T57]` (main.cu:315) TODO [进阶] 多 block 版本：用 auxiliary array 存储每个 block 的总和，再 scan，最后 scatter 回各 block。
`[CAP1-T58]` (main.cu:321) Stage 4-V2：Blelloch work-efficient inclusive scan（O(n) work）头部注释。
`[CAP1-T59]` (main.cu:331) TODO [必做] 步骤 5-e：加载数据到 shared mem（同 Hillis-Steele）。
`[CAP1-T60]` (main.cu:333) TODO [必做] 步骤 5-f：上扫（reduce）阶段。
`[CAP1-T61]` (main.cu:341) TODO [必做] 步骤 5-g：将根节点置为 identity（exclusive scan）或保留（inclusive scan）。
`[CAP1-T62]` (main.cu:345) TODO [必做] 步骤 5-h：下扫（distribution）阶段。
`[CAP1-T63]` (main.cu:356) TODO [必做] 步骤 5-i：写回 out（inclusive = exclusive + 原始值）。

**Step 5：排序（Radix Sort）**
- 输入：前缀和数组（视为 32-bit key-value 对）
- 操作：基数排序，每趟处理 4 位（共 8 趟）
  - digit histogram → scan → scatter
  - 或使用 CUB `cub::DeviceRadixSort::SortPairs`
- 输出：排序后的 key-value 对
- Kernel 骨架：`radix_digit_hist_kernel(...)` 和 `radix_scatter_kernel(...)`

`[CAP1-T64]` (main.cu:361) Stage 5：Radix Sort — digit histogram kernel（第一步）头部注释。
`[CAP1-T65]` (main.cu:372) TODO [必做] 步骤 6-a：每个线程读取一个 key，提取当前趟的 digit。
`[CAP1-T66]` (main.cu:377) TODO [必做] 步骤 6-b：per-block shared mem 直方图（类似 Stage 2 privatized）。
`[CAP1-T67]` (main.cu:384) TODO [必做] 步骤 6-c：将 block 局部 histogram merge 到全局；digit_hist 形状 `[gridDim.x, RADIX_BUCKETS]`，按 block 分段存储。
`[CAP1-T68]` (main.cu:392) Stage 5：Radix Sort — scatter kernel（第三步）头部注释。
`[CAP1-T69]` (main.cu:403) `prefix_sums` 参数：scan 结果（每个 digit 的全局起始位置）。
`[CAP1-T70]` (main.cu:406) TODO [必做] 步骤 6-d：每个线程读取 key，得到 digit，查询 prefix_sums[digit] 获得 bucket 起始偏移，atomicAdd 占槽位。
`[CAP1-T71]` (main.cu:410) TODO [必做] 步骤 6-e：写入 keys_out / vals_out（scatter）。
`[CAP1-T72]` (main.cu:413) TODO [进阶] 使用 CUB `cub::DeviceRadixSort::SortPairs` 替换手工实现，对比性能并分析为何库版本通常更快。

---

### 必做任务

1. **搭建项目骨架**
   - 创建目录，确认 `CMakeLists.txt` 能编译 CUDA 代码
   - 包含 `common/cuda_check.cuh`, `common/timer.cuh`, `common/nvtx_range.cuh`

2. **实现 Normalize kernel**（`TODO [必做] 步骤 2-a/b/c/d`）
   - 读入浮点数组，用 Welford 在线算法计算均值方差
   - 输出归一化结果
   - 填写 `verify_normalize_cpu(...)` 验证正确性（相对误差 ≤ 1e-5）

`[CAP1-T73]` (main.cu:421) CPU 端正确性验证（验收点 3）头部注释。
`[CAP1-T74]` (main.cu:431) TODO [必做] 步骤 2-d（验收）：CPU 双 pass 计算 mean/var，归一化，与 gpu_output 逐元素对比，相对误差 ≤ tol。

3. **实现 Histogram kernel（两版）**（`TODO [必做] 步骤 3-a～h`）
   - atomic 版：直接写全局内存（baseline）
   - privatized 版：shared memory 本地累加，最后 atomic merge
   - 填写 `verify_histogram_cpu(...)` 验证精确性

`[CAP1-T76]` (main.cu:455) TODO [必做] 步骤 3-h（验收）：CPU 端统计直方图，与 gpu_hist 精确对比，验证 `sum(gpu_hist) == N_elements`。

4. **实现 Conv2D kernel**（`TODO [必做] 步骤 4-a/b/c`）
   - 3×3 高斯卷积，shared memory tile + halo
   - 验证与 CPU 卷积结果一致

5. **实现 Scan kernel（两版）**（`TODO [必做] 步骤 5-a～i`）
   - Hillis-Steele：O(n log n) work
   - Blelloch：O(n) work，更 work-efficient
   - 对比两版吞吐

6. **实现 Radix Sort**（`TODO [必做] 步骤 6-a～f`）
   - 手工实现 digit histogram + scan + scatter
   - 或用 CUB `cub::DeviceRadixSort::SortPairs` 封装
   - 填写 `verify_sort_cpu(...)` 验证有序性

`[CAP1-T78]` (main.cu:466) TODO [必做] 步骤 6-f（验收）：检查 `keys_sorted[i] <= keys_sorted[i+1]` 对所有 i 成立。

7. **组织管道与并发**（`TODO [必做] 步骤 7-a～h`）
   - 用多 stream 启动各 kernel，用 event 建立依赖
   - 用 CUDA Graph capture 打包管道
   - 测 replay 10–100 次的总耗时和平均 throughput

`[CAP1-T96]` (main.cu:592) 单次管道执行：使用多 stream 并发组织。stream 0：normalize→histogram→conv2d→scan→sort；stream 1：可选并发 checksum kernel。
`[CAP1-T97]` (main.cu:603) `perfs` 参数说明：输出各阶段性能，长度 5。
`[CAP1-T98]` (main.cu:605) TODO [必做] 步骤 7-a：创建 stream_count 个 stream。
`[CAP1-T99]` (main.cu:610) 占位：使用默认流，让 host 编译通过；学员填好后替换为真实 stream。
`[CAP1-T100]` (main.cu:617) Stage 1 — Normalize 子段。
`[CAP1-T101]` (main.cu:623) Launch config（block / grid 配置）。
`[CAP1-T102]` (main.cu:640) 读：HWC float；写：HWC float + 2*C float（mean/var 极小，忽略）。
`[CAP1-T103]` (main.cu:642) 每个像素：约 7 FLOP（Welford 更新），TODO 学员填写精确值。
`[CAP1-T104]` (main.cu:647) TODO [必做] 步骤 7-b：在 Stage 1 完成后插入 event，让 Stage 2 等待。
`[CAP1-T105]` (main.cu:653) Stage 2 — Histogram（先 V1，再 V2；计时取 V2 作为"优化版"代表）。
`[CAP1-T106]` (main.cu:660) 归一化后像素在 [-3, 3] 左右，展平为 1D 对所有通道做直方图。
`[CAP1-T107]` (main.cu:666) V1 启动段。
`[CAP1-T108]` (main.cu:672) V2 计时段。
`[CAP1-T109]` (main.cu:682) FLOPs 估算注释：映射 + clamp + atomicAdd（近似）。
`[CAP1-T110]` (main.cu:686) Stage 3 — Conv2D（对第 0 通道做单通道卷积）。
`[CAP1-T111]` (main.cu:696) 动态 shared mem：tile with halo。
`[CAP1-T112]` (main.cu:704) 取第 0 通道（stride=CHANNELS，通道交错格式）。
`[CAP1-T113]` (main.cu:713) 每个输出像素读取 (3x3) 个输入像素 + 写 1 个输出。
`[CAP1-T114]` (main.cu:715) MAC 注释。
`[CAP1-T115]` (main.cu:719) Stage 4 — Scan（先 Hillis-Steele，再 Blelloch；取 Blelloch 计时）。
`[CAP1-T116]` (main.cu:726) 单 block 版本：仅对 min(HW, BLOCK_1D) 个元素做 scan 演示。
`[CAP1-T117]` (main.cu:727) TODO [必做]：实现多 block 版本（使用辅助数组）以支持 4K×4K。
`[CAP1-T118]` (main.cu:732) Hillis-Steele 启动。
`[CAP1-T119]` (main.cu:737) Blelloch 计时。
`[CAP1-T120]` (main.cu:745) bytes：读 + 写。
`[CAP1-T121]` (main.cu:746) Blelloch O(n) 近似 FLOP 估算注释。
`[CAP1-T122]` (main.cu:750) Stage 5 — Radix Sort（对卷积输出 float 按位模式排序）。
`[CAP1-T123]` (main.cu:757) 将 float 重解释为 uint32 用于排序（仅演示；正确处理需考虑符号位）。
`[CAP1-T124]` (main.cu:758) TODO [必做]：转换负数 float 的 bit 模式以保持正确排序顺序。
`[CAP1-T125]` (main.cu:765) 每趟处理 RADIX_BITS 位，共 32/RADIX_BITS = 8 趟。
`[CAP1-T126]` (main.cu:769) 第一步：digit histogram（每个 block）。
`[CAP1-T127]` (main.cu:776) 第二步：scan（对 digit_hist 全局求前缀和）。
`[CAP1-T128]` (main.cu:777) TODO [必做]：调用 scan kernel，在 d_prefix_sums 中存储每个 digit 的全局起始偏移。
`[CAP1-T129]` (main.cu:780) 第三步：scatter。
`[CAP1-T130]` (main.cu:787) 交换 in/out 指针（ping-pong buffer）。
`[CAP1-T131]` (main.cu:788) TODO [必做]：swap d_sort_keys_in / d_sort_keys_out 等。
`[CAP1-T132]` (main.cu:793) bytes：读 + 写。
`[CAP1-T133]` (main.cu:794) hist + scan + scatter 近似 FLOP 估算注释。
`[CAP1-T134]` (main.cu:798) TODO [必做] 步骤 7-c：销毁 stream 和 event。
`[CAP1-T135]` (main.cu:805) CUDA Graph 封装管道：stream capture + 重放。
`[CAP1-T137]` (main.cu:822) TODO [必做] 步骤 7-d：在 capture_stream 上开始 capture。
`[CAP1-T138]` (main.cu:826) TODO [必做] 步骤 7-e：在 capture_stream 上依次 launch 所有 kernel；capture 期间不能调用 host-sync。
`[CAP1-T139]` (main.cu:829) TODO [必做] 步骤 7-f：结束 capture，获取 graph。
`[CAP1-T140]` (main.cu:833) TODO [必做] 步骤 7-g：实例化 graph（JIT 编译为可执行对象）。
`[CAP1-T141]` (main.cu:837) TODO [必做] 步骤 7-h：replay iters 次，测量总耗时。
`[CAP1-T142]` (main.cu:847) TODO [进阶] 分支 C：改用 explicit graph API（cudaGraphCreate / cudaGraphAddKernelNode）。
`[CAP1-T143]` (main.cu:850) TODO [进阶] 分支 D：用 cub::DeviceRadixSort 替换 Stage 5。

8. **Nsight 采样与分析**
   - `nsys profile` 采集时间线
   - `ncu --set full` 对每个 kernel 采集详细指标
   - 对至少 3 个 kernel 计算 roofline 位置

`[CAP1-T80]` (main.cu:472) Roofline 辅助：计算每阶段算术强度并打印对比表（头部注释）。
`[CAP1-T81]` (main.cu:480) 理论 FP32 峰值（TFLOPS）：从设备属性获取；这里用近似值或让学员填写。
`[CAP1-T82]` (main.cu:481) TODO [必做] 步骤 8（Roofline）：从 cudaDeviceProp 计算 multiProcessorCount * clockRate * 2 (FMA) * 32 (warp)。
`[CAP1-T83]` (main.cu:484) 占位：Hopper H100 约 60 TFLOPS FP32。
`[CAP1-T84]` (main.cu:486) Roofline 表头打印（阶段 / 耗时 / GB/s / %带宽峰值 / AI / 约束类型）。
`[CAP1-T85]` (main.cu:493) 简单判断：AI < ridge_point 则 memory-bound，否则 compute-bound。
`[CAP1-T86]` (main.cu:508) 输出"理论峰值带宽"和"理论 FP32 峰值"行。
`[CAP1-T87]` (main.cu:510) 输出"Roofline ridge point"行。

---

### 进阶任务

**分支 A：FP16 重写与 TensorCore 利用**（`TODO [进阶]`）
- 用 `__half` / `__half2` 重写 normalize、conv2d
- 对比 FP32 vs FP16 的性能和精度差异
- 用 Nsight Compute 测量 Tensor Core 吞吐利用率

`[CAP1-T168]` (main.cu:987) TODO [进阶] 分支 A：FP16 重写。

**分支 B：多 GPU 版本**（`TODO [进阶]`）
- 用 NCCL 或手工 peer-to-peer 显存拷贝，图像分块在多 GPU 上处理
- 每个 GPU 处理一部分，GPU-0 汇总结果
- 测量多 GPU 加速比（受通信开销制约）

`[CAP1-T169]` (main.cu:991) TODO [进阶] 分支 B：多 GPU 版本（cudaMemcpyPeerAsync / NCCL）。

**分支 C：端到端 explicit CUDA Graph**（`TODO [进阶]`）
- 用 `cudaGraphCreate` / `cudaGraphAddKernelNode` 手工构造完整 DAG
- 用 `cudaGraphExecKernelNodeSetParams` 动态更新分辨率参数
- 测量参数更新成本

`[CAP1-T170]` (main.cu:995) TODO [进阶] 分支 C：端到端 explicit CUDA Graph。

**分支 D：Thrust/CUB 对比**（`TODO [进阶]`）
- 用 `thrust::transform` + `thrust::reduce` 重写 normalize
- 用 `cub::DeviceRadixSort::SortPairs` 替换 Stage 5
- 对比手工 kernel 与库函数的性能差异，分析原因

`[CAP1-T171]` (main.cu:999) TODO [进阶] 分支 D：Thrust/CUB 对比。

---

### 验收点

`[CAP1-T88]` (main.cu:514) 设备端指针集合（管道各阶段输入/输出）头部注释。
`[CAP1-T89]` (main.cu:518) `d_input` 字段：原始图像 [H*W*C]（其余字段同段注释）。
`[CAP1-T90]` (main.cu:542) 图像缓冲分配段。
`[CAP1-T91]` (main.cu:548) 直方图缓冲分配段。
`[CAP1-T92]` (main.cu:552) 卷积缓冲分配段（对第 0 通道做单通道卷积）。
`[CAP1-T93]` (main.cu:556) Scan 缓冲分配段。
`[CAP1-T94]` (main.cu:560) Radix sort 缓冲分配段。
`[CAP1-T95]` (main.cu:566) digit histogram 分配：最多 65536 blocks * RADIX_BUCKETS。

以下条件**必须同时满足**，缺一则项目未通过：

1. 代码编译通过（VS2026 Release 配置）
2. 程序运行无错误（无 CUDA 错误、无 segfault）
3. 输出正确性：
   - normalize 结果与 CPU 对比相对误差 ≤ 1e-5
   - histogram 计数精确（`sum(hist) == N_elements`）
   - sort 结果有序（`keys[i] <= keys[i+1]` 对所有 i 成立）
4. 性能数据可交付：5 个 kernel 各自的执行时间 + 实测 throughput（GB/s 或 GFLOPS）
5. Roofline 图完整：至少 3 个 kernel 的算术强度和吞吐落在图上
6. 带宽利用率达标：**整个管道实测带宽 ≥ 理论峰值的 65%**（至少一个 kernel 接近顶线）
7. 文档完整：README + ANALYSIS.md 清晰说明各步功能、性能数据、优化点

---

### 观察点

需要在 `ANALYSIS.md` 中记录至少以下 6 项（每项 2–3 行）：

1. **多 stream 并发效果**：有无多 stream 时整体耗时如何变化？哪些步骤能并发？
2. **CUDA Graph vs 逐个 launch**：graph replay 相比逐个启动的加速倍数？消除了多少 launch 开销？
3. **直方图两版对比**：atomic vs privatized 性能差异多大？为什么？
4. **Scan 两版对比**：Hillis-Steele vs Blelloch 的吞吐、内存访问模式有何不同？
5. **Roofline 位置**：三个主要 kernel 各自是 memory-bound 还是 compute-bound？距离 roof 还差几个百分点？
6. **瓶颈定位**：用 Nsight Compute 指标，明确整个管道最大的瓶颈；下一步优化应针对哪里？

---

### 常见坑

1. **分辨率选择不当**：太小（512×512）无法充分填满 GPU；太大（16K×16K）显存不足。从 4K 开始试。
2. **Welford 算法实现错误**：单 pass 计算均值方差容易数值不稳定；必须用 Welford 或两 pass reduce。
3. **Histogram bin 范围不匹配**：假设像素在 [0, 256) 但实际输入是归一化后的 [-3, 3]，导致 bin 溢出或空 bin。
4. **Shared memory 不足**：conv2d halo tile 若 block size 太大会超出 shared memory 限制；需平衡 block size。
5. **Scan 中 `__syncthreads` 位置错误**：多 stage scan 中同步位置不对会导致数据混乱。
6. **Stream 依赖漏掉**：某 stream 上的 kernel 需要另一条 stream 的结果，但没有 `cudaStreamWaitEvent`，导致数据竞争。
7. **Graph capture 期间有 host-sync**：在 `cudaStreamBeginCapture` 和 `cudaStreamEndCapture` 之间调用 `cudaStreamSynchronize` 会导致 capture 失败。
8. **Nsight Compute 报告不准确**：kernel 运行时间 < 1 ms 时采样噪声大；或未用 `--set full` 获取完整指标。
9. **Roofline 单位混淆**：算术强度单位是 FLOP/byte，不是 FLOP/bit；理论吞吐单位是 FLOP/s 或 GB/s。
10. **CUB 头文件缺失**：使用 CUB radix sort 需要 CCCL/CUTLASS FetchContent 正确配置。
11. **Event 计时包含 synchronize 开销**：计时窗口意外包含了同步延迟；需用异步计时或后处理数据。
12. **忘记 CUDA_CHECK**：kernel 或显存拷贝失败时无报错，导致后续数据混乱而不知原因。

---

### 提示

`[CAP1-T21]` (main.cu:84) 打印使用说明（`print_usage`）头部注释。
`[CAP1-T29]` (main.cu:99) CLI 参数解析（`parse_args`）头部注释。
`[CAP1-T31]` (main.cu:127) Host 端随机数据生成（`generate_image`），生成 H×W×C 的 float32 图像，值域 [0, 1)。

- **调试首先保证正确性**：先写各 kernel 的 naive 版本并对标 CPU，再做性能优化。
- **分阶段集成**：先单独测每个 kernel，再组织多 stream，再用 graph。不要一上来就全做。
- **Nsight 采样技巧**：
  - 小数据量先快速采样（`--set speedoflight`）
  - 确认没问题后再采集完整报告（`--set full`）
  - 记录关键指标到 CSV 便于后续分析
- **性能对标**：查 cuDNN/cuBLAS 文档，看它们对应操作的带宽/吞吐，作为对标目标。
- **Roofline 绘制**：可以用 Python matplotlib、Excel 或在线工具。关键是清晰标出每个点和边界线。
- **Welford 参考**：
  ```
  count += 1
  delta  = x - mean
  mean  += delta / count
  delta2 = x - mean
  M2    += delta * delta2
  // 方差 = M2 / count
  ```
- **Blelloch scan 两阶段**：上扫（reduce）建树；根节点置 0；下扫（distribution）分发结果。

---

### 复盘问题

`[CAP1-T145]` (main.cu:858) `main` 入口注释。
`[CAP1-T146]` (main.cu:862) 解析参数子段。
`[CAP1-T153]` (main.cu:878) 设备信息子段。
`[CAP1-T154]` (main.cu:884) Host 端数据生成子段。
`[CAP1-T156]` (main.cu:897) 设备缓冲分配子段。
`[CAP1-T157]` (main.cu:902) H2D 拷贝原始图像。
`[CAP1-T158]` (main.cu:906) 上传卷积核。
`[CAP1-T159]` (main.cu:910) 初始化排序输入：将归一化图像 float bit 模式重解释为 uint32。
`[CAP1-T160]` (main.cu:912) TODO [必做]：用 normalize_kernel 的输出做真实排序输入；当前用原始图像整数化占位。
`[CAP1-T161]` (main.cu:917) 初始化索引值数组 0,1,2,…。
`[CAP1-T162]` (main.cu:927) 运行管道子段。
`[CAP1-T163]` (main.cu:942) 等待所有 GPU 工作完成。
`[CAP1-T164]` (main.cu:945) 正确性验证子段（学员填写 verify 函数体后可见结果）。
`[CAP1-T165]` (main.cu:950) D2H 拷回归一化结果。
`[CAP1-T166]` (main.cu:973) 性能报告子段。
`[CAP1-T167]` (main.cu:977) 清理子段。

1. 多 stream 并发相比单 stream 串行的加速比是多少？什么因素决定了能否达到理论并发？
2. CUDA Graph replay 消除了多少 launch 开销？用什么方法测量的？
3. Histogram atomic vs privatized 两版为什么性能不同？从内存访问角度解释。
4. Conv2D 用 shared memory tile + halo 相比直接用 global memory 快了多少倍？为什么？
5. Scan 的 Hillis-Steele 和 Blelloch 算法分别的 work complexity 是多少？这如何影响性能？
6. 根据 roofline 分析，哪个 kernel 是最大瓶颈？下一步优化应从哪个方向入手？
7. FP32 vs FP16 性能对比（若做了分支 A）：性能提升是多少？精度变化如何？
8. 多 GPU 版本（若做了分支 B）：通信开销占比是多少？加速比与 GPU 数量的关系是否线性？
9. 整个管道与单步 normalize + histogram + conv2d + scan + sort 各自的总耗时对比，差异来自哪里？
10. 如果要把这个管道部署到生产环境，还需要补充哪些功能（错误处理、batch 支持、内存管理等）？

---

### 对应官方参考

- CUDA C++ Programming Guide：Streams, Events, Graphs
- Nsight Systems：<https://docs.nvidia.com/nsight-systems/>
- Nsight Compute：<https://docs.nvidia.com/nsight-compute/>
- CUTLASS（含 CUB）：<https://github.com/NVIDIA/cutlass>
- CUB 文档：<https://nvlabs.github.io/cub/>
- CUDA Best Practices：<https://docs.nvidia.com/cuda/cuda-c-best-practices-guide/>
- GPU 架构指南（Hopper）：<https://docs.nvidia.com/cuda/hopper-tuning-guide/>

---

## 编译与运行

### 1. 编译

```bash
# 从 C14_GPU/exercises/ 根目录
cmake --preset vs2026
cmake --build build-vs2026 --config Release --target CAPSTONE1_image_pipeline
```

### 2. 基本运行

```bash
# 默认 4K x 4K，10 次迭代，不使用 CUDA Graph
build-vs2026\Release\CAPSTONE1_image_pipeline.exe

# 指定分辨率和参数
build-vs2026\Release\CAPSTONE1_image_pipeline.exe --width 4096 --height 4096

# 启用 CUDA Graph 封装
build-vs2026\Release\CAPSTONE1_image_pipeline.exe --width 4096 --height 4096 --use-graph

# 高分辨率 + 多次重放（测峰值吞吐）
build-vs2026\Release\CAPSTONE1_image_pipeline.exe --width 8192 --height 8192 --use-graph --iters 100

# 查看所有选项
build-vs2026\Release\CAPSTONE1_image_pipeline.exe --help
```

### 3. Nsight Systems 采集

```bash
# 采集完整时间线（CUDA + NVTX trace）
nsys profile -o cap1 --stats=true ^
    build-vs2026\Release\CAPSTONE1_image_pipeline.exe --iters 50

# 多次重放以获取稳定数据
nsys profile -o cap1_graph --stats=true --trace=cuda,nvtx ^
    build-vs2026\Release\CAPSTONE1_image_pipeline.exe --use-graph --iters 100

# 在 GUI 中打开
nsys-ui cap1.qdrep
```

### 4. Nsight Compute 逐 kernel 分析

```bash
# 全量指标（较慢，用于最终分析）
ncu --set full -o cap1_ncu ^
    build-vs2026\Release\CAPSTONE1_image_pipeline.exe --iters 5

# Speed-of-light 快速扫描（用于初步排查）
ncu --set speedoflight -o cap1_sol ^
    build-vs2026\Release\CAPSTONE1_image_pipeline.exe --iters 3

# 指定单个 kernel（仅采集 normalize）
ncu --set full --kernel-name normalize_kernel -o cap1_normalize ^
    build-vs2026\Release\CAPSTONE1_image_pipeline.exe --iters 3
```

### 5. compute-sanitizer 正确性检查

```bash
# 内存越界 + 竞争检测
compute-sanitizer --tool memcheck ^
    build-vs2026\Release\CAPSTONE1_image_pipeline.exe --width 512 --height 512

# 竞争条件检测（较慢）
compute-sanitizer --tool racecheck ^
    build-vs2026\Release\CAPSTONE1_image_pipeline.exe --width 512 --height 512
```

---

## 输出对照（printf / std::puts 原文）

- `[CAP1-T22]` (main.cu:86) 原文：`用法: %s [选项]` → 现：`Usage: %s [options]`
- `[CAP1-T23]` (main.cu:88) 原文：`--width  N        图像宽度（像素），默认 4096` → 现：`--width  N        image width in pixels, default 4096`
- `[CAP1-T24]` (main.cu:89) 原文：`--height N        图像高度（像素），默认 4096` → 现：`--height N        image height in pixels, default 4096`
- `[CAP1-T25]` (main.cu:90) 原文：`--iters  N        重复执行次数（用于 CUDA Graph replay），默认 10` → 现：`--iters  N        repeat count (used for CUDA Graph replay), default 10`
- `[CAP1-T26]` (main.cu:91) 原文：`--use-graph       启用 CUDA Graph 封装管道（默认 false）` → 现：`--use-graph       wrap pipeline in a CUDA Graph (default false)`
- `[CAP1-T27]` (main.cu:92) 原文：`--stream-count N  并发 stream 数量，默认 2` → 现：`--stream-count N  concurrent stream count, default 2`
- `[CAP1-T28]` (main.cu:93) 原文：`示例:` → 现：`Examples:`
- `[CAP1-T30]` (main.cu:121) 原文：`[警告] 未知参数: %s` → 现：`[warning] unknown argument: %s`
- `[CAP1-T75]` (main.cu:444) 原文：`[verify_normalize] TODO [必做]：填写 CPU 参考实现，当前跳过验证。` → 现：`[verify_normalize] TODO [REQUIRED]: fill in the CPU reference; verification skipped.`
- `[CAP1-T77]` (main.cu:457) 原文：`[verify_histogram] TODO [必做]：填写 CPU 参考实现，当前跳过验证。` → 现：`[verify_histogram] TODO [REQUIRED]: fill in the CPU reference; verification skipped.`
- `[CAP1-T79]` (main.cu:467) 原文：`[verify_sort] TODO [必做]：填写有序性验证，当前跳过。` → 现：`[verify_sort] TODO [REQUIRED]: fill in the ordering check; verification skipped.`
- `[CAP1-T147]` (main.cu:868) 原文：`=== CAPSTONE1 图像统计管道 ===` → 现：`=== CAPSTONE1 image statistics pipeline ===`
- `[CAP1-T148]` (main.cu:869) 原文：`分辨率: %d x %d  (%.1f MP)` → 现：`resolution:  %d x %d  (%.1f MP)`
- `[CAP1-T149]` (main.cu:872) 原文：`通道数: %d (RGB)` → 现：`channels:    %d (RGB)`
- `[CAP1-T150]` (main.cu:874) 原文：`迭代次数: %d` → 现：`iterations:  %d`
- `[CAP1-T151]` (main.cu:875) 原文：`CUDA Graph: %s（启用/关闭）` → 现：`CUDA Graph:  %s (on/off)`
- `[CAP1-T152]` (main.cu:876) 原文：`Stream 数: %d` → 现：`streams:     %d`
- `[CAP1-T155]` (main.cu:892) 原文：`[Host] 分配并生成随机图像 %.2f MB...` → 现：`[Host] allocating and generating random image %.2f MB...`
- `[CAP1-T136]` (main.cu:817) 原文：`[Graph] 开始 stream capture...` → 现：`[Graph] beginning stream capture...`
- `[CAP1-T144]` (main.cu:852) 原文：`[Graph] TODO [必做]：填写 stream capture 逻辑。当前回退为顺序执行。` → 现：`[Graph] TODO [REQUIRED]: fill in stream capture; falling back to sequential execution.`
- `[CAP1-T172]` (main.cu:1005) 原文：`[CAPSTONE1] 完成。` → 现：`[CAPSTONE1] done.`
- `[CAP1-T173]` (main.cu:1006) 原文：`建议用 nsys profile --trace=cuda,nvtx 查看时间线，` → 现：`Tip: nsys profile --trace=cuda,nvtx for a full timeline,`
- `[CAP1-T174]` (main.cu:1007) 原文：`用 ncu --set full 对每个 kernel 采集完整性能指标。` → 现：`and ncu --set full to collect detailed metrics per kernel.`

---

*本脚手架由 C14_GPU 课程自动生成。所有 `TODO [必做]` 标记均对应 `08-结课项目1-数据并行管道与性能基线.md` 中的必做步骤编号。*
