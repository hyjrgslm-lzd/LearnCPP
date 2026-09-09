# 练习 H4：gemm_hopper_warp_specialized

## 1. 目标

`[H4-T01]` (main.cu:3) 练习 H4：Hopper Warp-Specialized GEMM — producer warp (TMA) + consumer warps (wgmma) + cluster launch。
`[H4-T02]` (main.cu:6) 目标：集成模块 G 的 wgmma、TMA、mbarrier、warp specialization 到完整 GEMM kernel。producer warp 用 TMA bulk copy 搬运 A/B tile，consumer warps 用 wgmma 计算；cluster launch `__cluster_dims__(2,1,1)` 可选。在 sm_90a 上达到接近库级性能（目标 70-85% Hopper FP32 峰值）。
`[H4-T03]` (main.cu:13) 编译要求：sm_90a，CUDA 13.x，C++20 device / C++26 host。

集成模块 G 的所有技术（wgmma、TMA、mbarrier、warp specialization）到一个完整 GEMM kernel：producer warp 用 TMA bulk copy 搬运 A/B tile，consumer warp 组用 wgmma 计算。在 sm_90a 上达到接近库级性能（目标 70-85% Hopper FP32 峰值）。同时引入 cluster launch（`__cluster_dims__(2,1,1)`），让多个 block 共享 L2 以减少 global memory 压力。

## 2. 前置知识

- 完成模块 G1-G5（wmma、wgmma、TMA bulk copy、mbarrier、warp specialization）
- 完成 H1-H3
- 理解 Hopper cluster 的分布式 shared memory（DSMEM）
- 理解 `cudaLaunchKernelEx` 与 `cudaLaunchConfig_t` 的用法

## 3. 硬件要求

- **sm_90a 必需**（wgmma / TMA / cluster launch 仅在 Hopper 上可用）
- 非 Hopper GPU：程序在运行时检测后跳过 kernel，打印提示信息
- CUDA Toolkit 13.x+

## 4. 文件说明

| 文件 | 说明 |
|------|------|
| `main.cu` | Hopper warp-specialized GEMM skeleton，含运行时 Hopper 检测 |
| `CMakeLists.txt` | CUDA_ARCHITECTURES = "90a" |
| `README.md` | 本文件 |

## 5. 必做任务

### 步骤 1 — 添加 `__cluster_dims__` 声明

`[H4-T04]` (main.cu:25) `cuda/ptx` 提供 wgmma / cp.async.bulk 等 Hopper PTX 绑定。
`[H4-T05]` (main.cu:38) Hopper 运行时检测宏。
`[H4-T06]` (main.cu:48) 超参。
`[H4-T07]` (main.cu:51) `BM`：block tile M。
`[H4-T08]` (main.cu:52) `BN`：block tile N。
`[H4-T09]` (main.cu:53) `BK`：block tile K。
`[H4-T10]` (main.cu:54) `NUM_STAGES`：pipeline 级数。
`[H4-T11]` (main.cu:58) Hopper Warp-Specialized GEMM kernel。
`[H4-T12]` (main.cu:73) NOTE：wgmma / TMA 指令仅在 sm_90a 设备代码路径编译。

```cpp
__global__
__cluster_dims__(2, 1, 1)
__launch_bounds__(256)
void gemm_hopper_warp_specialized(...) { ... }
```

2×1×1 cluster 表示 2 个 block 组成一个 cluster，共享 L2 流量。

### 步骤 2 — kernel 参数设计

`[H4-T13]` (main.cu:80) `A`：`[M x K]` 行主。
`[H4-T14]` (main.cu:81) `B`：`[K x N]` 行主。
`[H4-T15]` (main.cu:82) `C`：`[M x N]` 行主。
`[H4-T16]` (main.cu:83) `descA`：TMA tensor descriptor（cuTensorMap）。
`[H4-T17]` (main.cu:87) Shared memory 布局。
`[H4-T18]` (main.cu:91) mbarrier：每个 stage 一个，用于 producer-consumer 握手。
`[H4-T19]` (main.cu:97) mbarrier 初始化（仅 thread 0 执行）。
`[H4-T20]` (main.cu:100) TODO [必做] 步骤 1（续）：初始化 mbarrier，expect count = 1。
`[H4-T21]` (main.cu:104) stub。

```cpp
(const __half* A, const __half* B, float* C,
 const void* __grid_constant__ descA,   // TMA cuTensorMap
 const void* __grid_constant__ descB,
 int M, int N, int K)
```

`__grid_constant__` 修饰的 tensor descriptor 存放在常量内存，避免每 block 复制。

### 步骤 3 — blockDim 与 warp 分工

`[H4-T22]` (main.cu:109) block 负责的 C tile 起始坐标。
`[H4-T23]` (main.cu:114) 累加寄存器（consumer warp 使用）。
`[H4-T24]` (main.cu:117) stub 大小，实际依 wgmma shape。

```
blockDim = (256, 1, 1)  →  8 个 warp
  warp 0       (thread   0-31)：producer  — 负责 TMA bulk copy
  warp 1-7     (thread  32-255)：consumer  — 负责 wgmma 计算
```

### 步骤 4 — producer 逻辑（TMA + mbarrier）

`[H4-T25]` (main.cu:119) TODO [必做] 步骤 2：producer 逻辑（warp 0）。

```
初始化 mbarrier[NUM_STAGES]
预加载 stage 0：cp.async.bulk.tensor A/B tile
arrive mbarrier[0]

for k = 1 .. num_k_tiles:
  s = k % NUM_STAGES
  等待 consumer 释放 stage s（mbarrier::arrive_and_wait）
  cp.async.bulk.tensor 加载 tile k 到 stage s
  arrive mbarrier[s]（通知 consumer 数据就位）
```

### 步骤 5 — consumer 逻辑（wgmma）

`[H4-T26]` (main.cu:137) TODO [必做] 步骤 3：consumer 逻辑（warp 1-7）。
`[H4-T27]` (main.cu:151) TODO [必做] 步骤 4：epilogue — 将 fragC 写回 global C。
`[H4-T28]` (main.cu:154) stub：清零输出。

```
for k = 0 .. num_k_tiles:
  s = k % NUM_STAGES
  等待 producer 完成 stage s（mbarrier::arrive_and_wait）
  wgmma.mma_async.sync.aligned.m64n64k16.f32.f16.f16 (fragC, sA[s], sB[s])
  通知 producer 可以复用 stage s
```

### 步骤 6 — cudaLaunchKernelEx（cluster launch）

`[H4-T29]` (main.cu:165) 非 sm_90a 编译路径：空 kernel 占位，避免链接错误。
`[H4-T40]` (main.cu:264) TODO [必做] 步骤 6：用 `cudaLaunchKernelEx` 指定 cluster size。

```cpp
cudaLaunchConfig_t cfg{};
cfg.gridDim  = grid;
cfg.blockDim = dim3(256);
cudaLaunchAttribute attrs[1];
attrs[0].id = cudaLaunchAttributeClusterDimension;
attrs[0].val.clusterDim = {2, 1, 1};
cfg.attrs    = attrs;
cfg.numAttrs = 1;
cudaLaunchKernelEx(&cfg, gemm_hopper_warp_specialized,
                   dA, dB, dC, &tmaA, &tmaB, M, N, K);
```

### 步骤 7 — 测量 TFLOPS

`[H4-T49]` (main.cu:312) TODO [必做] 步骤 7：Nsight Compute 验证 cluster 与 warp specialization。

目标达到 Hopper FP32 峰值的 70-85%。

## 6. 进阶任务

`[H4-T50]` (main.cu:318) TODO [进阶] 集成 split-K（多 block 分割 K 维，再 reduce）。
`[H4-T51]` (main.cu:322) TODO [进阶] 尝试不同 tile size（32×32, 64×64, 128×128）。
`[H4-T52]` (main.cu:326) TODO [进阶] Blackwell sm_100a 对比测试。

- 集成 split-K（多 block 分割 K 维，再 reduce），支持更大矩阵
- 尝试不同 tile size（32×32, 64×64, 128×128），找到性能最优点
- Blackwell sm_100a 对比：观察新 MMA 指令形式的差异

## 7. 验收标准

- kernel 在 sm_90a 硬件上编译通过并运行无错误
- GEMM 结果与 CPU 参考逐元素一致（FP16 输入，FP32 累加，容差 1e-2）
- TFLOPS 达到 Hopper peak 的 70-85%
- Nsight Compute 中 cluster 被正确启动（block distribution 指标）
- Nsight Systems 中 warp specialization 时间线可见（producer 与 consumer 错开）

## 8. 常见坑

| 坑 | 说明 |
|----|------|
| cluster 大小超限 | 某些 Hopper 配置 cluster 最大 8 block；`cudaFuncGetAttributes` 可查 |
| wgmma 需要完整 warp group | blockDim 设置错误导致某些 warp 缺失，wgmma 行为未定义 |
| TMA swizzle 不匹配 | smem layout 与 TMA descriptor 中的 swizzle 模式不一致，数据乱序 |
| 非 tile 倍数矩阵 | 需要 padding 或边界检查，否则越界访问 |
| mbarrier 计数错误 | expect_tx 计数与实际 TMA 完成事件数不匹配，导致死锁 |

## 9. 观察点

- TMA 硬件加速使 global memory 搬运从 SM 卸载，SM 专注计算
- warp specialization 让 producer 和 consumer 独立前进，最大化重叠
- cluster launch 让多个 block 共享 L2，减少 global memory 访问
- 组合这些技术后，GEMM 吞吐可以逼近 GPU 理论峰值

## 10. 复盘问题

1. Hopper warp-specialized GEMM 相比 H3 pipeline 版本快多少倍，为什么？
2. TMA 的卸载如何减少 SM 压力，整体吞吐如何改善？
3. cluster 与 split-K 在什么场景下各自最优？

## 参考资料

- Hopper Tuning Guide Section “Warp Specialization”
- Hopper Tuning Guide Section “Tensor Memory Accelerator”
- CUTLASS 3.x examples `examples/48_hopper_warp_specialized_gemm/`
- NVIDIA Blog “Hopper GPU Performance” series

## 输出对照（printf / std::puts 原文）

- `[H4-T30]` (main.cu:174) CPU 参考实现。
- `[H4-T31]` (main.cu:211) `main` 入口。
- `[H4-T32]` (main.cu:220) Hopper 运行时检测。
- `[H4-T33]` (main.cu:223) 原文：`问题规模：M=%d  N=%d  K=%d` → 现：`Problem size: M=%d  N=%d  K=%d`
- `[H4-T34]` (main.cu:227) 主机内存。
- `[H4-T35]` (main.cu:235) 原文：`正在计算 CPU 参考（可能需要数十秒）...` → 现：`Computing CPU reference (may take tens of seconds)...`。原文：`[H4] 当前 GPU 不支持 Hopper (sm_90a) 特性，跳过 kernel 执行。` → 现：`[H4] Current GPU lacks Hopper (sm_90a) features; skipping kernel.`
- `[H4-T36]` (main.cu:238) 设备内存。
- `[H4-T37]` (main.cu:245) TODO [必做] 步骤 1：创建 TMA cuTensorMap。
- `[H4-T38]` (main.cu:255) stub：传 nullptr，kernel 中不使用。
- `[H4-T39]` (main.cu:259) 启动配置。
- `[H4-T41]` (main.cu:279) 预热。
- `[H4-T42]` (main.cu:284) 正式计时。
- `[H4-T43]` (main.cu:294) 正确性检查。
- `[H4-T44]` (main.cu:296) 原文：`── 正确性检查（stub 阶段预期 FAIL）──` → 现：`-- Correctness check (FAIL expected at stub stage) --`
- `[H4-T45]` (main.cu:300) 性能汇总。
- `[H4-T46]` (main.cu:303) Hopper H100 SXM FP32 理论峰值说明。
- `[H4-T47]` (main.cu:305) 原文：`占位；实际应从 cudaDeviceGetAttribute 推算` → 现：`placeholder; should derive from cudaDeviceGetAttribute`。
- `[H4-T48]` (main.cu:306) 原文：`── 性能汇总 ──...` → 现：`-- Performance summary --`
- `[H4-T53]` (main.cu:333) 原文：`[H4] 完成。` → 现：`[H4] done.`
