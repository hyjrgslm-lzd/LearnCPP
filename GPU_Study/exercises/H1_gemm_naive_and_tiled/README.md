# 练习 H1：gemm_naive_and_tiled

## 1. 目标

`[H1-T01]` (main.cu:3) 练习 H1：GEMM Naive 与 Shared-Memory Tiled 实现对比。
`[H1-T02]` (main.cu:5) 目标：从最简单的 naive GEMM（每个 thread 计算 C 的一个元素）开始，逐步优化到每个 block tile 32×32 的共享内存版本，对比带宽与 TFLOPS。
`[H1-T03]` (main.cu:9) 编译要求：sm_70+，CUDA 13.x，C++20 device / C++26 host。

从最简单的 naive GEMM（每个 thread 计算 C 的一个元素）开始，逐步优化到每个 block tile 32×32 的共享内存版本。通过对比两个版本的吞吐与 Nsight Compute roofline，深入理解为什么 naive 实现即使在 GPU 上也会被内存瓶颈完全主导。

## 2. 前置知识

- 理解 GEMM 的基本算法：C = A × B（M×K × K×N → M×N）
- 理解“每个 thread 计算 C 的一个元素”意味着每个 thread 需要从 global memory 加载 K 个 A 元素与 K 个 B 元素
- 理解 shared memory 作为 L1.5 cache 的角色：block 内所有 thread 共享，延迟低于 global memory 约 10-100 倍

## 3. 硬件要求

- sm_70+（Volta/Turing/Ampere/Hopper 均可）
- CUDA Toolkit 13.x+

## 4. 文件说明

| 文件 | 说明 |
|------|------|
| `main.cu` | 练习主体：两个 kernel + 性能测量框架 |
| `CMakeLists.txt` | 构建配置，CUDA_ARCHITECTURES = 70;80;86;89;90a |
| `README.md` | 本文件 |

## 5. 必做任务

### 步骤 1 — Naive GEMM kernel

`[H1-T04]` (main.cu:30) 常量与超参。
`[H1-T05]` (main.cu:32) `BLOCK_SIZE`：blockDim.x = blockDim.y。
`[H1-T06]` (main.cu:33) `TILE_SIZE`：smem tile 边长。
`[H1-T07]` (main.cu:34) `TILE_K_SIZE`：K 维度 tile 宽度。
`[H1-T08]` (main.cu:35) `WARMUP_ITERS`：预热次数（舍弃）。
`[H1-T09]` (main.cu:36) `BENCH_ITERS`：正式测量次数。
`[H1-T10]` (main.cu:39) 版本 1：Naive GEMM。
`[H1-T11]` (main.cu:43) `A`：`[M x K]`，行主。
`[H1-T12]` (main.cu:44) `B`：`[K x N]`，行主。
`[H1-T13]` (main.cu:45) `C`：`[M x N]`，行主。
`[H1-T14]` (main.cu:48) TODO [必做] 步骤 1：实现 naive GEMM。
`[H1-T15]` (main.cu:54) 提示：先计算 `row / col`，越界时直接 return。
`[H1-T16]` (main.cu:61) TODO [必做] 步骤 1（续）：补全内层累加循环，将结果写入 `C[row * N + col]`。
`[H1-T17]` (main.cu:64) stub：产生零输出，供 CPU 参考检测不一致。

```
gridDim  = (ceil(N/BLOCK_SIZE), ceil(M/BLOCK_SIZE))
blockDim = (BLOCK_SIZE, BLOCK_SIZE)   // BLOCK_SIZE = 32
```

每个 thread `(tx, ty)` 计算 `C[blockIdx.y*BLOCK_SIZE + ty][blockIdx.x*BLOCK_SIZE + tx]`。内层循环：`for k in 0..K`，读 `A[row][k]` 和 `B[k][col]`，累加到局部变量，最后写入 global C。

### 步骤 2 — cudaEvent 计时（Naive）

`[H1-T34]` (main.cu:202) TODO [必做] 步骤 2：测量 Naive 版本。
`[H1-T35]` (main.cu:204) 预热。
`[H1-T36]` (main.cu:210) 正式计时。

用 `CudaEventTimer` 测量 M=N=K=1024 的执行时间，计算：

```
TFLOPS = 2 * M * N * K / (time_ms * 1e-3) / 1e12
```

### 步骤 3 — Shared-Memory Tiled GEMM kernel

`[H1-T18]` (main.cu:67) 版本 2：Shared-Memory Tiled GEMM。
`[H1-T19]` (main.cu:79) TODO [必做] 步骤 3：声明共享内存 tile。
`[H1-T20]` (main.cu:90) TODO [必做] 步骤 3（续）：外层循环 `tile_k = 0 .. K/TILE_K_SIZE`，协作加载 A/B tile 到 smem 后做累加。
`[H1-T21]` (main.cu:97) 注意：边界检查（`row < M`, `col < N`, tile 内坐标 < K）。
`[H1-T22]` (main.cu:101) stub：产生零输出。

block 处理 `TILE_SIZE × TILE_SIZE`（32×32）的 C tile，循环 `K/TILE_K_SIZE` 次。每次：

1. 协作加载 `A tile`（`TILE_SIZE × TILE_K_SIZE`）到 `sA`
2. 协作加载 `B tile`（`TILE_K_SIZE × TILE_SIZE`）到 `sB`
3. `__syncthreads()` 确保数据就位
4. 内层循环累加 `sA × sB` 的贡献
5. `__syncthreads()` 确保计算完成再加载下一 tile

### 步骤 4 — cudaEvent 计时（Tiled）

`[H1-T37]` (main.cu:222) TODO [必做] 步骤 4：测量 Tiled 版本。

相同矩阵大小，测量 shared memory 版本吞吐。预期比 naive 快 5-10 倍。

### 步骤 5 — 性能对比表

`[H1-T40]` (main.cu:247) TODO [必做] 步骤 5：性能对比表。
`[H1-T41]` (main.cu:252) 理论峰值（FP32，查设备属性，此处简化为 1.5 TFLOPS 占位）。
`[H1-T42]` (main.cu:253) TODO [必做] 步骤 5（续）：用 `cudaDeviceGetAttribute` 查实际 FP32 峰值代替硬编码。
`[H1-T43]` (main.cu:254) Hopper H100 SXM FP32 单卡峰值（估算）。

输出格式：

```
variant              |       ms |   TFLOPS |    % peak
naive                |  xxx.xxx |   x.xxxx |     x.xx%
tiled (smem 32x32)   |  xxx.xxx |   x.xxxx |     x.xx%
```

### 步骤 6 — Nsight Compute profiling

`[H1-T50]` (main.cu:289) TODO [必做] 步骤 6：用 Nsight Compute `--set full` 抓两个版本 profile，记录 “Memory Bound” vs “Compute Bound”、Achieved Occupancy、L1/L2 Cache Hit Rate，绘制 roofline 图。

```bash
ncu --set full -o h1_naive ./H1_gemm_naive_and_tiled
ncu --set full -o h1_tiled ./H1_gemm_naive_and_tiled
```

## 6. 进阶任务

`[H1-T47]` (main.cu:277) TODO [进阶] 在共享内存版本基础上用 padding 消除 bank conflict。
`[H1-T48]` (main.cu:281) TODO [进阶] 尝试不同 `BLOCK_SIZE` / `TILE_K_SIZE`，找性能最优点。
`[H1-T49]` (main.cu:285) TODO [进阶] 手动 unroll 最内层 K 循环（`#pragma unroll`）。

- 在 shared memory 版本基础上，用 padding（`__shared__ float sA[TILE_SIZE][TILE_K_SIZE + 1]`）消除 bank conflict
- 尝试不同 `BLOCK_SIZE`（128、256）与 `TILE_K_SIZE`（8、16、32），找性能最优点
- 手动 `#pragma unroll` 展开最内层循环（K 维），减少循环开销

## 7. 验收标准

- naive 和 shared memory 两个版本编译通过，结果逐元素一致（FLT_EPSILON 容差）
- naive 吞吐远低于峰值（typically < 1% peak），shared memory 版本达到 roofline memory-bound 线（通常 10-30% peak）
- Nsight Compute 报告 naive 为“严重内存访问未合并”，shared memory 版本 coalescing 提升明显
- 能在 roofline 图上指出两个版本的算术强度（FLOPs/Byte）与理论位置

## 8. 常见坑

| 坑 | 说明 |
|----|------|
| coalescing 失败 | thread 排列不当导致相邻 thread 访问非连续地址 |
| bank conflict | 多个 thread 同时写 smem 相邻地址映射到同一 bank（32-bank，4B 对齐） |
| smem 溢出 | `TILE_SIZE × TILE_K_SIZE × 2 tile × 4B` 超过 48 KB，occupancy 下降 |
| `__syncthreads()` 位置 | 必须在计算前后各一次；缺少任何一次都可能产生数据竞争 |
| 未做 warmup | 第一次 kernel 执行受初始化影响，结果偏慢 |

## 9. 观察点

- naive GEMM 算术强度仅 0.25 FLOPs/Byte（FP32：每次 2 FLOP，读 8B），远低于 GPU roofline
- shared memory tile 通过让多个 thread 复用 smem 中的数据，将算术强度提升到 2-4 FLOPs/Byte（取决于 tile 大小）
- shared memory 版本的瓶颈可能转移到 smem 吞吐或 bank conflict，而非 global memory
- occupancy 不是决定性因素：naive 版本 occupancy 可能不低，但吞吐仍极低

## 10. 复盘问题

1. naive GEMM 为什么会被内存瓶颈支配？算术强度是多少？
2. shared memory tile 如何提升算术强度？trade-off 是什么？
3. 如果增加 TILE_SIZE 从 32 到 64，对 occupancy、寄存器压力、shared memory 占用各有什么影响？
4. roofline 图上，naive 与 optimized 版本分别对应什么位置？

## 参考资料

- CUDA C++ Best Practices Guide Chapter “Maximize Throughput” → “Maximize Memory Throughput”
- Nsight Compute documentation “Roofline Analysis”
- cuda-samples `matrixMul` / `matrixMulCUBLAS` examples

## 输出对照（printf / std::puts 原文）

- `[H1-T23]` (main.cu:107) 原文：`CPU 参考实现（三重循环，用于正确性验证）` → 现：`CPU reference (triple loop, used for correctness check).`
- `[H1-T24]` (main.cu:124) 原文：`正确性检查（逐元素比对，容差 1e-3 相对误差）` → 现：`Correctness check (element-wise, 1e-3 relative tolerance).`
- `[H1-T25]` (main.cu:139) 原文：`最大相对误差 ... 不一致元素` → 现：`max_rel_err ... mismatch`
- `[H1-T26]` (main.cu:145) 原文：`计算 TFLOPS` → 现：`TFLOPS calculation.`
- `[H1-T27]` (main.cu:152) `main` 入口。
- `[H1-T28]` (main.cu:161) 原文：`问题规模：M=%d  N=%d  K=%d` → 现：`Problem size: M=%d  N=%d  K=%d`
- `[H1-T29]` (main.cu:165) 分配主机内存。
- `[H1-T30]` (main.cu:169) 随机初始化（固定种子，方便复现）。
- `[H1-T31]` (main.cu:174) 原文：`正在计算 CPU 参考（可能需要数秒）...` → 现：`Computing CPU reference (may take several seconds)...`
- `[H1-T32]` (main.cu:178) 分配设备内存。
- `[H1-T33]` (main.cu:187) 启动配置。
- `[H1-T38]` (main.cu:240) 原文：`正确性检查（kernel stub 产生零，CPU ref 非零 → 报 FAIL，符合预期）` → 现：`Correctness check (stub kernels emit zero ... FAIL is expected).`
- `[H1-T39]` (main.cu:242) 原文：`── 正确性检查（stub 阶段预期 FAIL）──` → 现：`-- Correctness check (FAIL expected at stub stage) --`
- `[H1-T44]` (main.cu:256) 原文：`── 性能汇总 ──...` → 现：`-- Performance summary --`
- `[H1-T45]` (main.cu:269) 原文：`内存带宽利用率粗估` → 现：`Rough memory-bandwidth utilization estimate.`
- `[H1-T46]` (main.cu:274) 原文：`Naive 估算读带宽（下界）: %.2f GB/s` → 现：`Naive estimated read BW (lower bound): %.2f GB/s`
- `[H1-T51]` (main.cu:294) 原文：`── 清理 ──` → 现：`-- cleanup --`
- `[H1-T52]` (main.cu:299) 原文：`[H1] 完成。` → 现：`[H1] done.`
