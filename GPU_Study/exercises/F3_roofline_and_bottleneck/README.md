# 练习 F3：Roofline 模型与性能分析

## 目标

`[F3-T01]` (main.cu:2) 练习 F3：Roofline 模型与性能分析。
`[F3-T02]` (main.cu:3) 学习目标：3 个对比 kernel（K1 低 AI 内存密集 / K2 中等 AI / K3 高 AI 计算密集），加上延迟密集的 K4 atomic histogram；手工计算或从 Nsight Compute 提取每个 kernel 的算术强度；在 roofline 图上标注 4 个点观察距离 roof 的差距；根据 roofline 位置给出优化建议。
`[F3-T03]` (main.cu:9) 编译命令。
`[F3-T04]` (main.cu:10) 采集命令。
`[F3-T05]` (main.cu:11) 运行命令。

理解 roofline 模型的概念、如何从 Nsight Compute 的 roofline 图中读出算术强度和吞吐、以及如何用 roofline 推导优化方向。通过把多个 kernel 绘制在同一个 roofline 图上，直观看出性能差距和优化潜力。

## 前置理解

- 你知道算术强度的定义：`(计算次数) / (字节搬运)`。
- 你理解 GPU 既有计算峰值，也有带宽上限，二者共同约束吞吐。
- 你对 F2 中的 Nsight Compute 使用有基本认识。

## 编译

```bash
cmake --build build --target F3_roofline_and_bottleneck
```

## 采集命令

```bash
# 全量采集（获取所有 roofline 相关指标）
ncu --set full -o f3_report ./F3_roofline_and_bottleneck

# 分别采集各 kernel
ncu --set full --kernel-name k1_copy_low_ai        -o f3_k1 ./F3_roofline_and_bottleneck
ncu --set full --kernel-name k2_fma_chain_medium_ai -o f3_k2 ./F3_roofline_and_bottleneck
ncu --set full --kernel-name k3_matmul_high_ai      -o f3_k3 ./F3_roofline_and_bottleneck
ncu --set full --kernel-name k4_atomic_hist_latency -o f3_k4 ./F3_roofline_and_bottleneck
```

## 必做任务

`[F3-T06]` (main.cu:23) 常量定义。
`[F3-T07]` (main.cu:24) 4M 元素（K1 / K4）。
`[F3-T08]` (main.cu:25) matmul 尺寸（K2 / K3）。
`[F3-T09]` (main.cu:30) K1：低 AI copy（内存密集），AI = 2 ops / 8 bytes = 0.25 FLOP/byte，预期落在 roofline 内存斜线附近。
`[F3-T10]` (main.cu:33) TODO [必做-1] 完成 kernel 主体。
`[F3-T11]` (main.cu:43) TODO [必做-1] `out[tid] = in[tid] * 1.0f + 0.0f;`，AI 约 2 FLOP / 8 bytes = 0.25 FLOP/byte（内存密集）。
`[F3-T12]` (main.cu:50) K2：中等 AI — vector fused multiply-add（MAD 链）。每线程执行 `CHAIN_LEN` 次 FMA，读写 2 个 float，AI = 2*CHAIN_LEN ops / 8 bytes 约 8 FLOP/byte（中等），预期落在内存斜线与计算 roof 交叉点附近。
`[F3-T13]` (main.cu:54) TODO [必做-1] 完成 kernel 主体。
`[F3-T14]` (main.cu:57) AI = 2*32/8 = 8 FLOP/byte。
`[F3-T15]` (main.cu:69) TODO [必做-1] 展开 FMA 链，人为提高计算量。
`[F3-T16]` (main.cu:75) K3：高 AI — naive matmul 128x128（计算密集），AI 约 2*N^3 / (N^2*12 bytes) 约 21 FLOP/byte（高），预期落在计算 roof 附近。
`[F3-T17]` (main.cu:79) TODO [必做-1] 完成 kernel 主体。
`[F3-T18]` (main.cu:91) TODO [必做-1] 内积循环。
`[F3-T19]` (main.cu:99) K4：延迟密集 atomic histogram（随机写），AI 约 1 FLOP / 4 bytes = 0.25 FLOP/byte（与 K1 相近），但不是带宽受限而是 atomic 竞争 + 随机 L2 miss 导致的延迟受限，预期 AI 低但吞吐也低，落在两条线以下。
`[F3-T20]` (main.cu:103) TODO [必做-1] 完成 kernel 主体。
`[F3-T21]` (main.cu:115) TODO [必做-1] `atomicAdd(&hist[in[tid] % bins], 1);`，随机写地址 -> 高 L2 miss -> 高延迟 -> 延迟密集。
`[F3-T28]` (main.cu:222) TODO [必做-2] ncu 采集后记录实测 AI 和吞吐，填写注释。
`[F3-T30]` (main.cu:241) TODO [必做-2] 记录实测数据。
`[F3-T32]` (main.cu:264) TODO [必做-2] 记录实测数据。
`[F3-T34]` (main.cu:285) TODO [必做-2] 记录实测数据；注意 K4 AI 与 K1 相近但吞吐明显低 -> 延迟密集。
`[F3-T35]` (main.cu:289) TODO [必做-3] 手工绘制 roofline 图（Python matplotlib 或 Excel）：横轴算术强度（FLOP/byte，对数刻度）；纵轴实现吞吐（GFLOP/s，对数刻度）；画两条边界线（compute roof 水平 / memory roof 斜线）；将 K1/K2/K3/K4 四个点标注在图上；标注每个点距离 roof 的差距（优化空间）。
`[F3-T36]` (main.cu:296) TODO [必做-4] 为每个 kernel 给出 roofline 位置与优化建议：K1 内存密集，接近内存 roof，可通过向量化 / 预取进一步逼近峰值；K2 中等 AI，介于两条线之间，可通过增加 CHAIN_LEN 提高计算密度；K3 计算密集，可通过 shared memory tiling 减少全局内存访问；K4 延迟密集，roofline 无法直接指导，需要减少 atomic 竞争（shared mem 局部直方图）。
`[F3-T37]` (main.cu:302) TODO [必做-5] 从 Nsight Compute 报告中验证 AI 与理论值的差距（cache 效应）。

1. 完成四个 kernel 的主体实现（K1/K2/K3/K4）。
2. 分别对四个 kernel 用 `ncu --set full` 采样。
3. 对每个报告，手工计算或从 Nsight Compute 报告中提取算术强度（FLOP/byte）和实现吞吐（GFLOP/s 或 GB/s）。
4. 在一张 roofline 图上绘制四个点，横轴为算术强度，纵轴为实现吞吐。同时画出两条边界线：计算 roof（`吞吐 = 计算峰值`，水平线）；内存 roof（`吞吐 = 带宽 × 算术强度`，斜线）。
5. 观察四个点各自落在哪个象限（内存绑定还是计算绑定），以及距离 roof 还差多远。
6. 基于 roofline 位置，给出每个 kernel 的优化建议。

## 四个 kernel 的理论 AI

| Kernel | 算术强度（理论） | 预期瓶颈类型 |
|---|---|---|
| K1：`k1_copy_low_ai` | 0.25 FLOP/byte | memory-bound |
| K2：`k2_fma_chain_medium_ai`（CHAIN_LEN=32） | 8.0 FLOP/byte | 介于两线之间 |
| K3：`k3_matmul_high_ai`（128^3） | 约 21 FLOP/byte | compute-bound |
| K4：`k4_atomic_hist_latency` | 0.25 FLOP/byte | latency-bound（非带宽受限）|

> **注意**：K1 和 K4 的理论 AI 相近，但 K4 实际吞吐明显低于 K1，因为延迟是主要瓶颈（atomic 竞争 + 随机 L2 miss），roofline 无法直接表征这种情况。

## Roofline 图绘制参考

### GPU 理论峰值（需根据实际硬件查阅规格）

- **Hopper H100 SXM5**：FP32 峰值约 67 TFLOP/s，HBM3 带宽约 3.35 TB/s
- **Ampere A100 SXM4**：FP32 峰值约 19.5 TFLOP/s，HBM2e 带宽约 2 TB/s
- **Ada RTX 4090**：FP32 峰值约 82.6 TFLOP/s，GDDR6X 带宽约 1 TB/s

### 两条 roof 线方程

```
compute roof: y = peak_GFLOPS  (水平线)
memory  roof: y = peak_BW_GB_s * x  (斜线，x = AI in FLOP/byte)
交叉点 (ridge point): x_ridge = peak_GFLOPS / peak_BW_GB_s
```

### Python 绘图骨架（进阶）

```python
import matplotlib.pyplot as plt
import numpy as np

# 填入实测值
kernels = {
    "K1_copy":   (0.25, ???),  # (AI, GFLOP/s)
    "K2_fma":    (8.0,  ???),
    "K3_matmul": (21.0, ???),
    "K4_hist":   (0.25, ???),
}

peak_gflops = ???   # 查阅硬件规格
peak_bw     = ???   # GB/s

x = np.logspace(-2, 3, 500)
compute_roof = np.full_like(x, peak_gflops)
memory_roof  = peak_bw * x

plt.figure(figsize=(10, 6))
plt.loglog(x, np.minimum(compute_roof, memory_roof), 'k-', label='Roofline')
for name, (ai, tput) in kernels.items():
    plt.loglog(ai, tput, 'o', markersize=10, label=name)
plt.xlabel('Arithmetic Intensity (FLOP/byte)')
plt.ylabel('Throughput (GFLOP/s)')
plt.legend()
plt.grid(True, which='both', linestyle='--', alpha=0.5)
plt.title('Roofline Model -- F3')
plt.savefig('f3_roofline.png', dpi=150)
```

## 观察点

`[F3-T22]` (main.cu:121) 辅助：打印 roofline 参数。
`[F3-T24]` (main.cu:144) 分配内存。
`[F3-T25]` (main.cu:163) 初始化。
`[F3-T26]` (main.cu:185) 理论 AI 估算（用于绘制 roofline 参考点）。
`[F3-T27]` (main.cu:201) 运行 K1：低 AI copy（内存密集）。
`[F3-T29]` (main.cu:222) 运行 K2：中等 AI FMA 链。
`[F3-T31]` (main.cu:243) 运行 K3：高 AI naive matmul（计算密集）。
`[F3-T33]` (main.cu:267) 运行 K4：延迟密集 atomic histogram。

- roofline 模型把性能分析简化为"吞吐约束"的问题。
- 低算术强度的 kernel（内存绑定）优化的方向是"降低数据搬运"或"增加局部计算"。
- 高算术强度的 kernel（计算绑定）优化的方向是"更好地利用计算单元"或"降低指令延迟"。
- 如果一个 kernel 已经接近计算 roof，进一步优化的收益很小；应该考虑算法替代。
- K4（latency-bound）的点在两条线以下，说明问题不是带宽也不是计算峰值——是延迟。

## 常见坑

1. 算术强度计算错误（忘记考虑缓存效果或多次访问）。
2. 单位混淆（FLOP/s vs GB/s，或者 bits vs bytes）。
3. 理论峰值填错（查错硬件规格或 GPU 规格）。
4. roofline 斜线的斜率不对（应该 = 带宽 GB/s）。
5. 选择的 kernel 算术强度差距太小，无法清晰展示 roofline 的两个区域。

## 进阶任务

`[F3-T38]` (main.cu:307) TODO [进阶-1] 优化 K4（shared memory 局部直方图），重新测量 roofline 位置。
`[F3-T39]` (main.cu:308) TODO [进阶-2] 在 sm_80 和 sm_90a 上分别绘制 roofline，对比 roof 高度差异。
`[F3-T40]` (main.cu:309) TODO [进阶-3] 用 Nsight Compute 内置 roofline chart，与手工图对比。

- 尝试优化 K4（shared memory 局部直方图），重新采样并绘制新的点，观察在 roofline 上的位移。
- 如果有多块硬件（例如 Hopper 和 Ampere），分别绘制两个 roofline，对比相同 kernel 在两者上的位置。
- 用 Nsight Compute 的内置 roofline chart（如果可用），对比自己手工绘制的结果。

## 验收点

`[F3-T23]` (main.cu:137) `main` 入口。
`[F3-T41]` (main.cu:313) 清理。
`[F3-T42]` (main.cu:325) 完成提示。

- 四个 kernel 的 roofline 图完整，标注清楚。
- 你能准确说出每个 kernel 的算术强度和实现吞吐。
- 你能解释为什么某个 kernel 被内存 roof 或计算 roof 约束。
- 至少给出一条可行的优化建议，并说明原理。

## 复盘问题

1. roofline 图的两条边界线各代表什么约束？
2. 如果一个 kernel 的点在 roofline 图上正好在两条边界线的交点，说明什么？
3. 如何从 roofline 位置推导优化方向？
4. 两个 kernel 算术强度相同但吞吐不同，可能的原因是什么？

## 对应官方参考

- Roofline 论文：S. Williams et al., "Roofline: An Insightful Visual Performance Model for Floating-Point Programs"
- Nsight Compute Roofline：https://docs.nvidia.com/nsight-compute/

## 输出对照（printf / std::puts 原文）

- `[F3-T26]` (main.cu:188) 原文：`--- Roofline 参数：理论算术强度（AI）估算 ---` -> 现：`--- Roofline parameters: theoretical arithmetic intensity (AI) ---`
- K4 注释 `但延迟密集（atomic + 随机写）` -> `but latency-bound (atomic + random write)`。
- `[F3-T27]` (main.cu:204) 原文：`--- K1: copy（memory-bound，AI≈0.25）---` -> 现：`--- K1: copy (memory-bound, AI~0.25) ---`
- `[F3-T29]` (main.cu:225) 原文：`--- K2: FMA chain（medium AI≈8）---` -> 现：`--- K2: FMA chain (medium AI~8) ---`
- `[F3-T31]` (main.cu:246) 原文：`--- K3: naive matmul %dx%d（compute-bound，AI≈21）---` -> 现：`--- K3: naive matmul %dx%d (compute-bound, AI~21) ---`
- `[F3-T33]` (main.cu:270) 原文：`--- K4: atomic hist（latency-bound，AI≈0.25 但非带宽受限）---` -> 现：`--- K4: atomic hist (latency-bound, AI~0.25 but not BW-bound) ---`
- 启动信息 `启动:` -> `launch:`。
- `[F3-T42]` (main.cu:328) 原文：`[F3] 完成。用 ncu --set full -o f3_report 采集后绘制 roofline 图。` -> 现：`[F3] done. ncu --set full -o f3_report and plot the roofline.`
