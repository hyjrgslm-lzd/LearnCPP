# 14 结课项目 2：源码精读与 mini 实现

## 项目目标

这是 Stage 2 的出口。你会在 CUTLASS、Flash-Attention、OptiX 中**选一条或全做**，完成"源码精读 + 手写简化复现"的双闭环。

核心任务是：**读透一个 SOTA 高性能算子的源码（5 层 hierarchy / on-chip 流水 / 数值稳定技巧），写一份 1500+ 字的精读笔记（含架构图、公式、关键设计决策），再用 ≤600–800 行裸 CUDA 或 cuTe 手写一个功能等价的简化版，用 Nsight Compute 对标性能基准，交付验证数据。**

选择哪一条分支不重要；重要的是"能讲清楚为什么这样设计""能预测改动的性能影响""能识别关键决策与权衡"。

## 前置知识

- **必读**：模块 A–F 全部完成（CUDA 基础 + 性能工程）。
- **强烈推荐**：模块 G–J 至少完成 4/5（TensorCore / GEMM 优化 / AI 算子 / 生态库），这样你对"为什么要这样做"的背景更清楚。
- **模块 K（OptiX）**：仅分支 C 需要；分支 A/B 不需要。
- 能独立阅读英文技术文档（GitHub README、Programming Guide、论文）。
- 有 4–8 小时持续专注的时间用于源码精读（不能碎片化）。

## 硬件与工具链要求

- **GPU 实机**：Hopper sm_90a **必须实机运行**（模拟和其他卡会导致性能数据无意义）；Blackwell sm_100a 可进阶验证。
- **CUDA Toolkit**：13.x 或更新。
- **CMake**：3.28+。
- **工具链**：
  - Nsight Compute（`ncu --set full` 完整采集，提取 FLOPS / 带宽 / Tensor Core 利用率）。
  - compute-sanitizer（验证无 race condition）。
  - Python 3.10+（用于数据对齐和可视化，可选但推荐）。
- **额外依赖**（分支相关）：
  - 分支 A：`CUTLASS 3.x`（FetchContent 自动拉取）。
  - 分支 B：`flash-attention` GitHub repo（git clone）；`torch` / `numpy`（验证精度）。
  - 分支 C：`OptiX SDK 8.x`（手动安装）；`stb_image_write.h`（图像输出）。

## 交付物清单

完成任意分支后，应交付以下物品：

| 项目 | 说明 |
|---|---|
| 源码精读笔记 | Markdown 格式，≥1500 字，含架构图/公式/决策分析 |
| 实现代码 | ≤600–800 行的 mini 版本（分支 A/B）或 ≤800 行的 Cornell Box（分支 C） |
| Nsight Compute 报告 | `.ncu-rep` 文件，导出 CSV 含 FLOPs / 吞吐 / TC 利用率 |
| 性能对标表格 | 性能数据与官方实现对比（表格形式）|
| 验证脚本 | Python 或 shell，验证精度 / 功能正确性 |
| README | 题面复述 + 验收点 + 复盘问题（100–150 字） |

---

## 分支 A：CUTLASS Kernel 精读 + 手写对应版 GEMM

### 目标

选择 CUTLASS `examples/` 下一个 SM90 warp-specialized GEMM（例如 `48_hopper_warp_specialized_gemm` 或 `55_hopper_mixed_dtype_gemm`），深入阅读其 5 层 hierarchy（Device / Kernel / Collective / Tiled MMA+Copy / Atom），理解 `CollectiveMainloop` 如何用 TMA + wgmma 流水消除延迟，画出完整的 data flow 图和同步点，再用裸 CUDA PTX + `CUtensorMap` 写一个功能等价的简化版（90% 性能）。

### 前置理解

- 完成模块 H（GEMM 优化阶梯、CUTLASS 基础、cuTe 概念）。
- 理解"warp specialization"：producer warp 负责 TMA 加载，consumer warp 负责 wgmma 计算，两者通过 barrier 同步。
- 知道"流水线"：第 N 阶段的 producer 在加载第 N+1 block tile 时，consumer 在计算第 N block tile 的 MMA。
- 理解 `CUtensorMap` 和 `cuTensorMapEncodeTiled` 如何定义 TMA 的索引坐标变换。

### 必做任务

1. **选定目标 example**：从 CUTLASS repo 的 `examples/` 下选一个 SM90 warp-specialized GEMM example（例如 `48_hopper_warp_specialized_gemm.cu`）。**选择标准**：≤600 行 device code、包含明确的 TMA + wgmma、有 mbarrier / async copy pipeline。
   ```
   // TODO [必做] 列出选定 example 的文件名和核心参数（M/N/K block size、数据类型）
   ```

2. **阅读 CUTLASS 源码框架**：系统地阅读 example：
   - Host 端：kernel launch、grid 配置、SBT-like tile scheduling。
   - Device 端：entry point → `CollectiveMainloop::operator()` → 内层 TMA/wgmma 循环。
   - 提炼 5 层 hierarchy：
     1. **Device Layer**：grid × block × warp layout（例如 1 CTA per tile）。
     2. **Kernel Layer**：kernel 入口函数签名、global memory 指针布局。
     3. **Collective Layer**：`CollectiveMainloop` 和 `CollectiveEpilogue` 的公开接口。
     4. **Tiled MMA + Copy Layer**：innermost loop，`tiled_mma` / `tiled_copy` 对象，一步一步的 async copy / barrier wait / mma 调用。
     5. **Atom Layer**：最底层的 PTX 原语（`wgmma.mma_async` 的签名、`cp.async.bulk.tensor` 的参数）。
   
   ```
   // TODO [必做] 绘制 5 层 hierarchy 图（纸质或 ASCII art），标注各层职责边界和数据流向
   ```

3. **定位 TMA + wgmma 核心循环**：在 `CollectiveMainloop::operator()` 中找到：
   - TMA descriptor 初始化（`cuTensorMapEncodeTiled`）。
   - Producer warp 的 loop：`cp.async.bulk.tensor` 加载 A/B tile → barrier arrive。
   - Consumer warp 的 loop：barrier wait → `wgmma.mma_async` 执行矩阵乘 → barrier sync 等待 producer。
   - Epilogue：处理尾部计算、应用 alpha/beta、写回 D。
   
   ```
   // TODO [必做] 在源码中标注 producer 和 consumer 的代码段行号
   // TODO [必做] 绘制 producer-consumer 流水线的时间线图（第 0/1/2 block tile 的执行重叠）
   ```

4. **分析数据流和同步**：
   - A tile 从 global → shared memory via TMA（多少字节、多少 steps、barrier 同步点）。
   - B tile 类似。
   - wgmma 消耗 shared memory 数据，输出到 registers（累加器）。
   - 绘制 shared memory 布局图（A tile 和 B tile 各占多少行列、bank conflict 是否被 swizzle 消除）。
   
   ```
   // TODO [必做] 绘制 shared memory layout 图，标注 A/B tile 的 offset、swizzle 规则、bank 占用
   ```

5. **提炼关键设计决策**：
   - **为什么用 warp specialization**：producer warp 不计算，全力加载数据（高 occupancy）；consumer warp 不管加载，全力计算（高 compute density）。相比于统一 warp 既加载又计算，这样能更好地隐藏延迟。
   - **为什么用 TMA 而非普通 `__global__` load**：TMA 硬件自动处理 tile 的多维索引变换、支持非连续内存、可以硬件流水（一个 TMA 请求未完成时下一个已发出），避免 warp 等待。
   - **为什么用 mbarrier + `wgmma.mma_async`**：异步执行，TMA 加载完后通知 mbarrier，wgmma 从 barrier 等待，不需要显式 `__syncthreads`；支持 pipeline 深度 > 1。
   - **Block tile 大小与 occupancy 的权衡**：大 tile（例如 M=128 N=256）需要更多 shared memory，可能降低 occupancy；小 tile（例如 M=64 N=128）节省 smem，允许更多 CTA 并发，但增加了 grid 维度可能导致 L2 缓冲压力。
   
   ```
   // TODO [必做] 写下 3–5 项关键设计决策与对应的性能影响
   ```

6. **精读笔记撰写**：1500+ 字的 Markdown，包含：
   - 概述：问题陈述、为什么这种设计、与朴素实现的对比。
   - 5 层 hierarchy：每层的职责、接口、数据流。
   - TMA + wgmma 详解：pipeline 概览、producer-consumer 同步机制、延迟隐藏的计算。
   - Shared memory layout：bank conflict 消除的 swizzle 技巧。
   - 性能模型：理论 peak FLOPS、实际达到 X%、瓶颈分析。
   - 关键参数调优：block tile 大小、pipeline 深度、warp 分配比例，对性能的影响。
   
   ```
   // TODO [必做] 输出精读笔记到 cutlass_analysis.md，≥1500 字
   ```

7. **手写简化版 GEMM**：用裸 CUDA PTX + cuTe（或纯 C++20 CUDA）实现一个 Hopper SM90 warp-specialized GEMM，功能等价于 CUTLASS example（矩阵乘 C = A @ B）：
   - 输入：A (M×K，FP16) / B (K×N，FP16) / D (M×N，FP32 输出）。
   - Block tile：选择一个相对小的尺寸（例如 M=64 N=128 K=64），简化实现。
   - Producer warp：加载 A/B tile 到 shared memory（可以用 `mma_pipelined` 或简化版的 `cp.async.bulk.tensor`）。
   - Consumer warp：执行 `wgmma.mma_async` 累加到 D_accum（register）。
   - Epilogue：把累加器写回 global memory。
   
   ```cuda
   // TODO [必做] 实现 kernel_warp_specialized_gemm_sm90(...)，≤600 行
   // 输入参数：A, B, D, M, N, K
   // 输出：D = A @ B (FP16 A/B -> FP32 D)
   ```

8. **编译与性能测试**：
   - 编译 CUTLASS example（baseline）和自己的 mini 版本。
   - 用 Nsight Compute 分别采集 baseline 和 mini 的性能数据（peak FLOPS、achieved FLOPS、Tensor Core 利用率、memory bandwidth）。
   - 提取关键指标对比（表格形式）。
   
   ```
   // TODO [必做] 生成性能对比表：baseline TFLOPS / mini TFLOPS / 比率 / 瓶颈分析
   ```

9. **验证脚本**：用 Python 验证矩阵乘的正确性（读 mini 版 GEMM 的输出，对标 cuBLAS 或 NumPy）。
   ```python
   # TODO [必做] Python 脚本：生成随机 A/B，调用 mini kernel，对标 np.dot / cuBLAS
   # 检查 max relative error < 1e-3（FP16 量化误差）
   ```

### 进阶任务

- 实现 Blackwell (sm_100/120) 版本的 warp-specialized GEMM（Hopper 的 wgmma 在 Blackwell 上有新特性）。
- 添加 epilogue（alpha/beta scaling、bias adding、activation fusion）。
- 支持 mixed-dtype 输入（FP8 A/B → FP32 D，体验 FP8 GEMM）。
- 比较不同 block tile 大小与 occupancy 的性能曲线。
- 实现 split-K 并行（多个 CTA 负责同一 (M, N) 区间的不同 K 片段，再 reduce）。

### 验收点

- 精读笔记 ≥1500 字，逻辑清晰，能讲清"为什么这样设计"。
- Mini 版 GEMM 编译通过，无 race condition（compute-sanitizer 验证）。
- Mini 版性能达到 CUTLASS example 的 80% 以上（例如 CUTLASS 200 TFLOPS，mini 达 160+ TFLOPS）。
- 矩阵乘结果正确（max relative error < 1e-3）。
- Nsight Compute 报告显示：
  - Tensor Core 利用率 ≥ 70%。
  - Warp 占用率接近 100%。
  - L2 缓冲命中率 > 80%（表明 tile 复用良好）。

### 观察点

- Warp specialization 在 Hopper 上的效果：producer 专注加载、consumer 专注计算，两者无竞争，并发度高。
- TMA 的硬件流水能力：单条 TMA 指令可以驱动多个 tile 的异步加载，不阻塞 warp。
- 异步流水 + mbarrier 的同步模式相比传统 `__syncthreads` 的优势：允许 pipeline 深度 > 1，消除更多延迟。

### 常见坑

1. **`CUtensorMap` 编码错误**：`cuTensorMapEncodeTiled` 的参数顺序、stride 计算容易出错。务必对照 Programming Guide。
2. **mbarrier initialize 没做**：如果未初始化 mbarrier，sync 操作会返回垃圾或挂起。确保 `__shared__` barrier 在 kernel 开始时被适当初始化。
3. **Producer/Consumer warp ID 分配错误**：通常前 32 个 lane 是 producer，后 32 个是 consumer（假设 64 threads/block）。错误的分配导致死锁。
4. **Pipeline 深度设置与 barrier 槽位数不匹配**：如果 pipeline 深度 = 3 但只分配 1 个 barrier，会冲突。
5. **Shared memory swizzle 遗漏**：Hopper shared memory 支持 128B swizzle 消除 bank conflict，但需要显式应用（通过 `__align__` 或 swizzle 宏）；如果忘记，bank conflict 会严重降低吞吐。
6. **wgmma 累加器寄存器溢出**：如果 block tile M×N 太大，累加器占用过多寄存器，导致 spill 到 local memory，性能崩溃。需要平衡 block size 和 occupancy。
7. **Global memory 带宽瓶颈未识别**：如果输入矩阵尺寸很小（例如 K=32），可能 TMA 加载成为瓶颈而非计算。
8. **对标性能时使用错误的 peak FLOPS**：Hopper sm_90a 的 FP16 Tensor Core peak 是 1456 TFLOPS（单 GPU），不要混淆与 peak memory bandwidth。
9. **Mini 版本没有处理非对齐尺寸**：如果 M/N/K 不是 block tile 大小的倍数，需要 tile remainder handling，容易遗漏。
10. **精读笔记过于冗长或过于简洁**：1500 字是下限，应该包含足够的细节（公式、数字、参数），但不是照搬源码注释。

### 提示

- CUTLASS 5 层 hierarchy 的框架图（ASCII art）：
  ```
  ┌─────────────────────────────────────────┐
  │  Device Layer: grid[x,y], block[x,y,z] │
  │  (CTA scheduling, block tile (M,N))     │
  └──────────────────┬──────────────────────┘
                     │
  ┌──────────────────┴──────────────────────┐
  │  Kernel Layer: entry_point(A,B,D,...)   │
  │  (thread layout, shared memory alloc)   │
  └──────────────────┬──────────────────────┘
                     │
  ┌──────────────────┴──────────────────────┐
  │  Collective Layer: CollectiveMainloop   │
  │  (producer-consumer warp specialization)│
  └──────────────────┬──────────────────────┘
                     │
  ┌──────────────────┴──────────────────────┐
  │  Tiled MMA + Copy: mma_pipelined        │
  │  (inner loop: TMA load → barrier        │
  │   → wgmma accumulate → barrier sync)    │
  └──────────────────┬──────────────────────┘
                     │
  ┌──────────────────┴──────────────────────┐
  │  Atom Layer: PTX primitives             │
  │  (cp.async.bulk.tensor, wgmma.mma_async)│
  └─────────────────────────────────────────┘
  ```

- Nsight Compute 导出 CSV 的命令：
  ```bash
  ncu --set full -o result.ncu-rep ./your_executable
  ncu --export csv result.ncu-rep > metrics.csv
  ```

- Python 验证脚本框架：
  ```python
  import numpy as np
  import cupy as cp
  
  M, N, K = 128, 256, 64
  A_host = np.random.randn(M, K).astype(np.float16)
  B_host = np.random.randn(K, N).astype(np.float16)
  
  # 期望结果：cuBLAS 或 NumPy
  expected = np.dot(A_host.astype(np.float32), B_host.astype(np.float32))
  
  # 调用自己的 kernel（通过 cupy kernel 或 ctypes + shared lib）
  # actual = kernel_result(...)
  
  relative_error = np.max(np.abs(actual - expected) / (np.abs(expected) + 1e-6))
  print(f"Max relative error: {relative_error}")
  assert relative_error < 1e-3
  ```

### 复盘问题

1. CUTLASS 的 5 层 hierarchy 中，各层分别对应"代码"和"硬件"的哪些概念？
2. 为什么 warp specialization 比"所有 warp 既加载又计算"的设计更高效？
3. TMA 的硬件流水能带来什么优势？如何在代码中体现这种流水？
4. Mbarrier + `wgmma.mma_async` 与传统 `__syncthreads` + `mma.sync` 的本质区别是什么？
5. 如果把 block tile 大小从 (64, 128) 改为 (128, 256)，性能会怎样变化？涉及哪些权衡？
6. 你的 mini 版本达到 baseline 的百分之几？差距来自哪些方面（算法 / 实现 / 编译器）？
7. 如何验证"shared memory bank conflict 被消除"？用什么 Nsight 指标？

### 对应官方参考

- CUTLASS 3.x Programming Guide：https://github.com/NVIDIA/cutlass/blob/main/media/docs/
- CUTLASS example `48_hopper_warp_specialized_gemm.cu`：https://github.com/NVIDIA/cutlass/tree/main/examples
- NVIDIA Hopper Tuning Guide：https://docs.nvidia.com/cuda/hopper-tuning-guide/
- CUTLASS Paper (VLDB 2017)：https://arxiv.org/abs/1706.04319

---

## 分支 B：Flash-Attention-2 前向源码精读 + 简化复现

### 目标

阅读 Flash-Attention-2 repo 的 `csrc/flash_attn/` 下的前向 kernel（FP16 + `mma.sync`），理解 tile 划分与在线 softmax + rescale 的流程，绘出数据流图和 shared memory 访问模式，再用 ≤800 行的 CUDA kernel 复现一个功能等价的简化版（FP16、causal/non-causal、无 head group 复杂度），与 PyTorch `scaled_dot_product_attention` 对齐精度 (ε < 1e-2)。

### 前置理解

- 完成模块 I（AI 算子手写实战），特别是 online softmax 和 attention 的基础知识。
- 理解"tile-based attention"：不一次性加载整个 KV，而是分块加载、逐块更新 softmax（running max / running sum）。
- 知道"in-register softmax"与"on-chip softmax"的区别和权衡。
- 了解 FP16 矩阵乘的精度特性（量化误差、数值稳定性）。

### 必做任务

1. **克隆 Flash-Attention 源码**：`git clone https://github.com/Dao-AILab/flash-attention.git`，定位 `csrc/flash_attn/flash_fwd_hdim*.cu` 中一个具体的前向 kernel（例如 `flash_fwd_hdim64_fp16.cu` 或 `flash_fwd_hdim128_fp16.cu`）。
   ```
   // TODO [必做] 列出选定 kernel 文件名和 head dimension / batch size / sequence length 的默认配置
   ```

2. **分析 tile 划分方案**：FA-2 的前向 kernel 采用 (Br, Bc) block tile 划分（Q 的行块、K/V 的列块）。理解：
   - Br：每次加载多少行 Q tile（例如 64）。
   - Bc：每次加载多少列 K/V tile（例如 32）。
   - 总共需要多少个 block tile 对（Br × Bc 的网格）来覆盖全序列。
   - 为什么这种划分支持 causal attention（自回归）。
   
   ```
   // TODO [必做] 绘制 Q/K/V tile 划分图，标注 Br、Bc、outer loop 和 inner loop 的迭代数
   ```

3. **分析在线 softmax 流程**：FA-2 的核心是"在处理每一块 K/V 时，实时更新 softmax"，而不是先算出全局 attention scores 再 softmax。理解：
   - 初始化 `m = -inf`（running max）、`l = 0`（running sum of exp）。
   - 对每个 K/V tile，计算 `S_ij = Q @ K_j^T`（形状 Br × Bc）。
   - 更新 `m_new = max(m_old, S_ij 的行最大值)`。
   - Rescale：`l_old = l_old * exp(m_old - m_new)` 和 `exp(S_ij - m_new)` 的结果也要rescale。
   - 累积 `l_new = l_old + sum(exp(S_ij - m_new), axis=1)`。
   - 累积 `accum = accum * (l_old / l_new) + exp(S_ij - m_new) @ V_j`。
   
   ```
   // TODO [必做] 推导在线 softmax 的完整公式，包含 rescale 步骤
   // TODO [必做] 绘制伪代码：outer loop (j in K blocks), inner loop (scatter accum)
   ```

4. **分析 shared memory 布局与访问**：FA-2 为了高吞吐，精心设计了 shared memory 的 tile layout（通常采用 column-major 或 row-major，加上 swizzle 消除 bank conflict）。理解：
   - Q tile、K tile、V tile 分别占用多少 smem bytes（例如 64×64×2 = 8KB for FP16）。
   - Producer warp 如何加载 K/V tile 到 smem（可能多个 warp 并行加载）。
   - Consumer warp 如何读 Q/K/V from smem 到 register，执行 mma。
   - Bank conflict 是否被消除（swizzle 参数）。
   
   ```
   // TODO [必做] 绘制 shared memory layout 图，标注各 tile 的 offset、swizzle 规则、bank 占用
   ```

5. **分析 causal masking**：如果是 causal attention，要在 tile level 确保 `j <= i`（K 的列块索引 ≤ Q 的行块索引），否则 mask attention scores。理解 FA-2 如何高效地实现 causal mask（通常是条件判断 + mask 一整行的 attention scores）。
   ```
   // TODO [必做] 写伪代码：如何在 tile-based kernel 中实现 causal mask
   ```

6. **精读笔记撰写**：1500+ 字的 Markdown，包含：
   - 概述：Attention 的复杂度（O(N^2) memory），FA-2 如何通过 tile 和在线 softmax 改善。
   - Tile 划分方案：(Br, Bc) 的选择、与 occupancy 和 smem 的权衡。
   - 在线 softmax：公式推导、rescale 的数值稳定性。
   - Shared memory 和 bank conflict：swizzle 技巧。
   - Causal attention：tile-level 的 mask 策略。
   - 性能模型：QK 矩阵乘的 FLOPS、softmax 的内存访问、V 累积的 FLOPS。
   - 与朴素 attention 的对比：内存访问减少多少、compute intensity 提升多少。
   
   ```
   // TODO [必做] 输出精读笔记到 flash_attention_analysis.md，≥1500 字
   ```

7. **简化复现 kernel**：用 ≤800 行 CUDA 实现一个功能等价的 FA-2 前向（FP16）：
   - 输入：Q (N, d), K (N, d), V (N, d)，其中 N = sequence length, d = head_dim（例如 64/128）。
   - 参数：Br = 64, Bc = 32（可配）。
   - Causal/non-causal flag。
   - 输出：attention result (N, d)。
   
   算法步骤：
   - Outer loop：遍历 Q 的每个 Br×d tile（行）。
   - Inner loop：遍历 K/V 的每个 Bc×d tile（列）。
   - 计算 `S = Q_tile @ K_tile^T`（Br × Bc）。
   - 更新 running max m 和 running sum l。
   - 使用 rescale 公式更新累加器。
   - 内层循环结束后，计算最终 output = accum / l。
   
   ```cuda
   // TODO [必做] 实现 kernel_flash_attention_fwd_simplified(...)，≤800 行
   // 输入参数：Q, K, V, N, d, Br, Bc, causal_flag
   // 输出：output (N, d)
   ```

8. **编译与精度验证**：
   - 编译 simple FA kernel 和官方 FA-2 kernel（或用 PyTorch 的 `torch.nn.functional.scaled_dot_product_attention` 作为参考）。
   - 生成随机 Q/K/V 输入（FP16），运行两个版本。
   - 计算 max relative error：`max(|simple - reference| / |reference|)`。
   
   ```
   // TODO [必做] 验证 max relative error < 1e-2（FP16 精度容限）
   ```

9. **性能对标**：
   - 用 Nsight Compute 采集 simple FA kernel 和官方 FA-2 kernel 的性能指标。
   - 对比 TFLOPS、内存带宽、Tensor Core 利用率。
   
   ```
   // TODO [必做] 生成性能对比表：official TFLOPS / simple TFLOPS / 比率
   ```

### 进阶任务

- 实现 FA-3（warp-specialized + TMA）版本的前向。
- 支持 multi-head 分组（group-query attention）。
- 添加 dropout 和 scale 参数。
- 实现后向传播（gradient computation）。
- 比较 FA-2 与 Triton 实现的性能差异。

### 验收点

- 精读笔记 ≥1500 字，公式推导清晰、图表完整。
- Simple FA kernel 编译通过，无 compute-sanitizer 错误。
- 精度对标：max relative error < 1e-2（FP16 量化误差）。
- 性能达到官方 FA-2 的 70% 以上（例如官方 120 TFLOPS，simple 达 84+ TFLOPS）。
- Nsight Compute 报告显示：
  - Tensor Core 利用率 ≥ 50%（softmax 开销导致不能达到 100%）。
  - L2/L1 缓冲命中率 > 70%。
  - 无 bank conflict 相关的性能降低（per-bank conflict miss count 很低）。

### 观察点

- Tile-based attention 的关键是"在线 softmax"：每处理一个 K/V tile 就更新 softmax 状态，不需要存储全局 attention matrix（从 O(N^2) memory 降到 O(N)）。
- Rescale 步骤的数值稳定性很关键：如果不小心处理可能导致 underflow / overflow，而 FA-2 通过 exp-normalize trick 规避这些。
- Tile 大小的选择是 smem 占用、bank conflict、occupancy 的平衡。

### 常见坑

1. **在线 softmax 的 rescale 公式写错**：常见错误是忘记对旧的 `l` 和 `accum` rescale，导致数值爆炸或消失。
2. **Causal mask 的条件判断错误**：如果条件是 `j > i` 应该 mask，容易写反成 `j < i`。
3. **Shared memory 不足**：如果选择的 (Br, Bc) 太大，会导致 smem overflow；编译时应该显式检查。
4. **Bank conflict 削弱吞吐**：即使功能正确，如果不消除 bank conflict，吞吐可能掉 30–50%。
5. **精度问题**：FP16 矩阵乘本身就有量化误差；如果 softmax 处理不当会放大误差。
6. **没有正确处理序列末尾（padding）**：如果序列不是 Br 的倍数，需要额外的尾部处理逻辑。
7. **Atomicity 问题**：如果多个 warp 并行写 output，需要确保原子性或正确同步。
8. **Loop unroll 过度**：如果内层循环 unroll 太激进，可能导致寄存器溢出。
9. **误解"FA-2 的内存复杂度"**：虽然 pass over K/V 次数多（O(N) passes），但总体 I/O 仍然是 O(N^2) 相似规模，关键优势在于"实时 softmax"而非"内存少"。
10. **Epsilon 值选错**：softmax 分母加的 epsilon（例如 1e-6）如果太小可能导致 NaN，太大会影响精度。

### 提示

- 在线 softmax 伪代码：
  ```
  m = -inf, l = 0, accum = 0
  for j in range(num_k_blocks):
    K_j = load K[:, j*Bc:(j+1)*Bc]
    V_j = load V[:, j*Bc:(j+1)*Bc]
    
    S = Q @ K_j^T  // shape (Br, Bc)
    
    if causal:
      mask S where col_j > row_i  // set to -inf
    
    m_new = max(m, S.row_max())
    P_tilde = exp(S - m_new)
    
    l_old = l
    l_new = l_old * exp(m - m_new) + P_tilde.sum(axis=1)
    
    accum = accum * (l_old / l_new).view(-1, 1) + P_tilde @ V_j
    m = m_new
    l = l_new
  
  output = accum / l.view(-1, 1)
  ```

- Nsight Compute 采集 attention kernel：
  ```bash
  ncu --set full -o result.ncu-rep ./your_attention_program
  ncu --import result.ncu-rep | grep -E "Tensor|Memory|FLOP"
  ```

- Python 精度验证框架：
  ```python
  import torch
  
  N, d = 1024, 64
  Q = torch.randn(N, d, dtype=torch.float16, device='cuda')
  K = torch.randn(N, d, dtype=torch.float16, device='cuda')
  V = torch.randn(N, d, dtype=torch.float16, device='cuda')
  
  # 官方参考
  with torch.no_grad():
    ref = torch.nn.functional.scaled_dot_product_attention(Q, K, V, is_causal=True)
  
  # 简化版本（通过 CUDA 调用）
  # simple = kernel_result(Q, K, V)
  
  rel_error = (simple - ref).abs().max() / (ref.abs().max() + 1e-6)
  print(f"Relative error: {rel_error}")
  assert rel_error < 1e-2
  ```

### 复盘问题

1. FA-2 的"在线 softmax"相比于"先算 attention matrix 再 softmax"有什么优势？
2. Rescale 公式 `l_new = l_old * exp(m_old - m_new)` 从何而来？能推导吗？
3. Causal attention 在 tile-based 实现中如何避免 K/V 的多次重复加载？
4. Tile 大小 (Br, Bc) 的选择对性能有什么影响？为什么 Br 通常比 Bc 大？
5. 你的 simple 版本与官方 FA-2 的性能差距来自哪些方面？
6. 如何验证"bank conflict 被消除"？
7. FP16 的量化误差如何影响最终 attention 的精度？

### 对应官方参考

- Flash-Attention-2 论文：https://arxiv.org/abs/2307.08691
- Flash-Attention GitHub：https://github.com/Dao-AILab/flash-attention
- Flash-Attention-3 论文（进阶）：https://arxiv.org/abs/2407.08608
- PyTorch scaled_dot_product_attention 文档：https://pytorch.org/docs/stable/generated/torch.nn.functional.scaled_dot_product_attention.html

---

## 分支 C：OptiX Mini 光追渲染器（Cornell Box + Denoiser）

### 目标

集成模块 K（K1–K4）的全部成果，实现一个完整的 Cornell Box 光追渲染器：支持三种材质（Lambertian / metallic / dielectric），1–2 点光源，蒙特卡罗路径追踪（≤512 样本/像素），使用 OptiX Denoiser 去噪，最后输出 denoised PNG。验收点：1024×1024 分辨率 512 spp 在 Hopper 上 ≤2 秒完成，denoised 输出与参考偏差 < 0.05（PSNR）。

### 前置理解

- 完成模块 K 全部 4 题（K1–K4），熟悉 OptiX pipeline、SBT、acceleration structure、raygen/hit/miss program、denoiser。
- 理解蒙特卡罗路径追踪的基本算法：每像素采样多条光线，递归追踪反射/折射，积累贡献取平均。
- 知道"Russian roulette"（概率性终止）如何避免无限递归。
- 理解三种基本材质的 BRDF（bidirectional reflectance distribution function）。

### 必做任务

1. **场景定义**：定义 Cornell Box（立方体房间，6 个面）加点光源（1–2 个）。
   - **Geometry**：六面体，可用三角形网格表达。
   - **Material**：
     - **Lambertian**（漫反射）：白墙、天花板。
     - **Metallic**（镜面反射）：红色或蓝色墙，有粗糙度。
     - **Dielectric**（玻璃/折射）：可选，一个透明球体。
   - **Light**：1–2 个点光源，位置和强度可配。
   
   ```cpp
   // TODO [必做] 定义 struct Material { type, color, roughness, ior }
   // TODO [必做] 定义 struct Light { pos, intensity }
   // TODO [必做] 初始化 Cornell Box 的 vertices/indices 和 material IDs
   ```

2. **BVH 构建**：用 OptiX acceleration structure 构建 Cornell Box 的 BVH（使用 GAS 和可选的 IAS）。
   ```cpp
   // TODO [必做] 调用 optixAccelBuild 构建 GAS
   // TODO [必做] 可选：如果用多个 instance，构建 IAS
   ```

3. **Raygen Program**：实现蒙特卡罗路径追踪的起点。每像素采样 512 条光线（可分多帧累积，这题简化为单帧 512 spp）。
   ```cuda
   // TODO [必做] extern "C" __global__ void __raygen__main()
   // 循环 512 次：
   //   生成随机方向光线
   //   调用 optixTrace，获取颜色贡献
   //   累积到 output buffer
   ```

4. **Material Program 组合**：实现多个 closest-hit 和 miss program 组合，对应不同材质和光源交互。
   - **Lambertian closest-hit**：计算漫反射方向、递归光线。
   - **Metallic closest-hit**：镜面反射方向、加入粗糙度扰动。
   - **Dielectric closest-hit**：处理折射（Snell's law）、Fresnel term、递归。
   - **Miss program**：背景（纯黑或环境贴图）。
   - **Light source**：如果光线直接打到光源，返回光源的 emissive 值。
   
   ```cuda
   // TODO [必做] __closesthit__lambertian() - 漫反射
   // TODO [必做] __closesthit__metallic() - 镜面反射
   // TODO [进阶] __closesthit__dielectric() - 玻璃折射
   // TODO [必做] __miss__background()
   ```

5. **Russian Roulette 和递归控制**：为了避免无限递归，使用 Russian roulette 概率性终止。递归深度通过 payload 传递。
   ```cuda
   // TODO [必做] 在 payload 中追踪 recursion_depth
   // TODO [必做] 在 closest-hit 中检查深度，如果 > max_depth，以概率 p 终止
   // TODO [必做] 调整权重：如果路径继续，权重 *= 1/(1-p)
   ```

6. **SBT 和 Program Group 组织**：为不同材质的三角形创建不同的 hitgroup（每个材质 ID 对应一个 closest-hit）。
   ```cpp
   // TODO [必做] 创建 raygen program group
   // TODO [必做] 创建 miss program group
   // TODO [必做] 为每种材质创建 hitgroup program group
   // TODO [必做] 组织 SBT 三段式
   ```

7. **Accumulation Buffer**：512 spp 需要累积 512 帧的结果。为了加速，可以：
   - **方案 A**（简化）：单帧 512 spp（raygen 循环 512 次生成随机方向）。
   - **方案 B**（推荐）：多帧累积（launch kernel 512 次，每次 1 spp，最后平均）。
   
   方案 A 更简单但 register 压力大；方案 B 更现实。**选择方案 A**（为了代码简洁）。
   
   ```cuda
   // TODO [必做] raygen 循环 512 次，每次计算一个随机方向光线
   // TODO [必做] 累积所有样本，最后除以 512 得到最终颜色
   ```

8. **OptiX Denoiser 集成**：对 512 spp 的高噪声输出应用 denoiser。
   ```cpp
   // TODO [必做] 调用 optixDenoiserCreate / optixDenoiserSetup / optixDenoiserInvoke
   // TODO [必做] 提供 albedo 和 normal 作为 guided denoising 的信息
   ```

9. **Tonemap 和输出**：Denoised 结果的 HDR 颜色转换为 LDR（uint8），保存为 PNG。
   ```cpp
   // TODO [必做] 实现 tonemap kernel（ACES 或 gamma）
   // TODO [必做] 调用 tonemap，输出到 uint8 buffer
   // TODO [必做] 保存为 PNG（用 stb_image_write）
   ```

10. **性能优化**：
    - 确保 raygen 中的随机数生成效率高（可用 curand 或 Sobol 序列）。
    - 利用 Hopper 的 wgmma 和 TMA（虽然这题主要是 ray tracing logic，但可在 closest-hit 的 BRDF 计算中应用优化）。
    - 使用 `__launch_bounds__` 调优 occupancy。
    
    ```cuda
    // TODO [必做] 用 curand 或内置 RNG 高效生成随机方向
    // TODO [必做] 调整 block size 和 __launch_bounds__ 达到高 occupancy
    ```

11. **验证与测试**：
    - 生成 1024×1024 的 Cornell Box，512 spp。
    - 测量运行时间（应该 ≤ 2 秒 on Hopper）。
    - 保存 denoised 输出。
    - 与参考（例如 offline renderer PBRT / Mitsuba）对比（可选，精度偏差 < 0.05 PSNR）。

### 进阶任务

- 实现 Microfacet BRDF（GGX）替代简单镜面反射。
- 实现 Next-Event Estimation（直接光照采样）。
- 支持 animated scene（帧更新几何体变换）。
- 实现 OptiX curve primitive 绘制毛发或植被。
- 支持多 GPU 分片渲染（分割 tile 到不同 GPU）。

### 验收点

- 编译无错误。
- 程序运行无 OptiX / CUDA error。
- 输出两个 PNG：
  - `output_noisy.png`：512 spp 去噪前的原始结果（参考，应该很有噪声）。
  - `output_denoised.png`：denoised 后的最终结果（平滑、细节保留）。
- 性能：1024×1024, 512 spp on Hopper ≤ 2 秒。
- Denoised 与参考 offline 渲染的偏差 < 0.05 PSNR（可用 SSIM / LPIPS 替代，可选）。
- Nsight Compute 报告验证无 warp inefficiency 或严重的 memory stall。

### 观察点

- 完整光追管线的复杂度体现在"多种 material 的管理"和"递归控制的稳定性"上。
- Russian roulette 是一个优雅的概率论技巧，避免无限递归同时保持无偏估计。
- Denoiser 相比朴素滤波效果显著，但代价是增加了 computational overhead。

### 常见坑

1. **随机数生成器的周期过短**：如果用简单的线性同余，512 spp 可能穷举周期导致明显的噪声模式。用 curand 或 Sobol。
2. **Russian roulette 概率设置不当**：如果 p（继续概率）太低，大部分路径早早终止，噪声很高；太高则递归深度太大。通常 p = 0.9 附近。
3. **权重没有正确调整**：如果路径继续，权重应该乘以 `1/(1-p)`，如果忘记会导致 bias。
4. **Fresnel term 计算错误**：玻璃材质的反射/折射比例由 Fresnel 决定，误算会导致材质不真实。
5. **折射方向计算错误**：Snell's law 在 BRDF 中的应用容易出错（特别是法线朝向、IOR 顺序）。
6. **Albedo buffer 初始化不对**：Denoiser 需要 albedo（表面固有颜色，不含阴影）；如果传的是最终颜色，guided denoising 效果会差。
7. **Normal buffer 从 closest-hit 读取时的法线方向错误**：法线应该指向 ray 射来的方向（outward from surface）。
8. **Accumulation 没有正确归一化**：512 spp 需要除以 512，如果忘记输出会全白。
9. **Tonemap 的 gamma 值选错**：通常 2.2，如果用其他值，图像会显示不对。
10. **多次 optixLaunch 调用时流同步问题**：如果 512 spp 分成 512 个 launch（每个 1 spp），需要确保流同步或使用 CUDA Graph。
11. **没有检查 denoiser 的输入/输出 buffer size**：如果实际分辨率与 setup 不匹配，invoke 会出错。
12. **光源强度设置不合理**：如果太强导致输出饱和，太弱则不可见。需要调试。

### 提示

- Cornell Box 数据框架：
  ```cpp
  // 6 个面的三角形顶点
  float3 cornell_box_vertices[] = {
    // 地板（白）
    {-1, -1, -1}, {1, -1, -1}, {1, -1, 1}, {-1, -1, 1},
    // 天花板（白）
    {-1, 1, -1}, {1, 1, -1}, {1, 1, 1}, {-1, 1, 1},
    // 后墙（白）
    {-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1},
    // 左墙（红）
    {-1, -1, -1}, {-1, -1, 1}, {-1, 1, 1}, {-1, 1, -1},
    // 右墙（蓝）
    {1, -1, -1}, {1, 1, -1}, {1, 1, 1}, {1, -1, 1},
    // 前墙（黑，打开）
    // (省略)
  };
  
  uint3 cornell_box_indices[] = {
    {0, 1, 2}, {0, 2, 3},  // 地板
    // ...
  };
  
  struct Material {
    enum Type { LAMBERTIAN, METALLIC, DIELECTRIC } type;
    float3 color;
    float roughness;  // for metallic
    float ior;         // for dielectric
  };
  
  Material cornell_materials[] = {
    {Material::LAMBERTIAN, {1, 1, 1}, 0, 0},  // 白
    {Material::LAMBERTIAN, {1, 0, 0}, 0, 0},  // 红
    {Material::LAMBERTIAN, {0, 0, 1}, 0, 0},  // 蓝
  };
  ```

- Raygen 蒙特卡罗采样框架：
  ```cuda
  extern "C" __global__ void __raygen__main() {
    unsigned int idx = optixGetLaunchIndex().x;
    unsigned int idy = optixGetLaunchIndex().y;
    
    float3 color_accum = {0, 0, 0};
    
    for (int s = 0; s < NUM_SAMPLES_PER_PIXEL; s++) {
      // 生成随机方向（可以用 cosine-weighted hemisphere sampling）
      float2 uv = {(idx + drand48()) / width, (idy + drand48()) / height};
      float3 ray_dir = camera_ray_at(uv);
      
      RayPayload payload = {0};
      payload.radiance = {0, 0, 0};
      payload.depth = 0;
      
      optixTrace(params.handle, camera_pos, ray_dir, ...);
      
      color_accum += payload.radiance;
    }
    
    float3 final_color = color_accum / NUM_SAMPLES_PER_PIXEL;
    output[idy * width + idx] = {final_color.x, final_color.y, final_color.z, 1};
  }
  ```

- Russian roulette 在 closest-hit 中的实现：
  ```cuda
  extern "C" __global__ void __closesthit__lambertian() {
    RayPayload* payload = (RayPayload*)optixGetAttribute_0();
    
    if (payload->depth >= MAX_DEPTH) {
      payload->radiance = {0, 0, 0};
      return;
    }
    
    float rr_prob = 0.9f;
    if (drand48() > rr_prob) {
      payload->radiance = {0, 0, 0};
      return;
    }
    
    // 继续追踪
    float3 normal = /* compute normal */;
    float3 scattered_dir = cosine_hemisphere_sample(normal);
    
    RayPayload next_payload = {0};
    next_payload.depth = payload->depth + 1;
    
    optixTrace(params.handle, hit_point + normal * 1e-4f, scattered_dir, ...);
    
    float3 material_color = {0.8, 0.8, 0.8};  // 灰色 Lambertian
    payload->radiance = material_color * next_payload.radiance / rr_prob;
  }
  ```

### 复盘问题

1. Cornell Box 有几个面？各自是什么材质和颜色？
2. Russian roulette 的概率 p 应该如何选择？太高或太低会怎样？
3. 为什么权重需要乘以 `1/(1-p)`？从数学上怎样理解？
4. Lambertian / metallic / dielectric 三种材质各自的 BRDF 是什么？
5. Denoiser 的 albedo 和 normal buffer 与最终渲染结果有什么区别？
6. 如果光源强度增加 10 倍，输出应该如何变化？（直观 + 数学）
7. 这个项目为什么要用 OptiX 而不是普通 CUDA kernel？OptiX 的优势是什么？

### 对应官方参考

- OptiX Programming Guide：https://raytracing-docs.nvidia.com/optix8/guide/
- Cornell Box 原始定义：https://www.graphics.cornell.edu/online/box/
- Ray Tracing: The Next Week（Path Tracing 教材）：https://raytracing.github.io/books/RayTracingTheNextWeek.html
- PBRT（参考 offline renderer）：https://www.pbrt.org/

---

## 完成标准与答辩（自我）

任选一条分支后完成，应该能回答下列问题（自检）：

1. **架构理解**：你能画出该 SOTA 实现的 5–10 层核心流程图吗？能讲清每层的职责和数据流吗？

2. **性能瓶颈识别**：用 Nsight Compute 采集的数据，能指出当前实现的主要瓶颈是什么（计算 / 内存 / 延迟）？能预测改动（例如改 block size、改 tile 大小）会如何影响性能吗？

3. **设计权衡**：作者为什么这样设计（例如 warp specialization / tile-based softmax / multi-material dispatch）？相比于朴素实现，优势和代价各是什么？还有没有其他权衡方案？

4. **数值稳定性**：这个算法涉及到的数值稳定性问题有哪些（例如 softmax overflow、BRDF NaN）？作者如何规避这些问题？

5. **一致性验证**：用什么方法验证你的实现与参考实现一致（精度对齐、性能对标）？误差来自哪些方面（量化 / 算法 / 实现 / 编译器）？

6. **扩展可能性**：基于你的理解，这个实现能扩展到什么新的场景或应用？有没有更激进的优化方向？

7. **跨项目借鉴**：这个实现中的关键技巧（例如 tile-based processing / producer-consumer synchronization / online reduction），是否能借鉴到其他算子（例如 conv、FFT、sort）？

---

## 做完结课 2 之后，你现在应该能说清楚什么

至少把下面几句话说顺（任选一条分支）：

**分支 A**：
- CUTLASS 的 5 层 hierarchy 如何组织代码，让 kernel 优化从"整体 kernel"变成"分层参数调优"。
- Warp specialization 如何通过专业分工（producer vs consumer）隐藏延迟、提升吞吐。
- TMA 的硬件流水如何驱动异步数据加载，与 wgmma 无缝衔接。

**分支 B**：
- Flash-Attention 如何用"tile-based 在线 softmax"将 attention 的内存复杂度从 O(N^2) 显式存储降至实时计算。
- Rescale 技巧如何保证数值稳定性，同时支持 pipeline 并发。
- Tile 大小选择的三角权衡（smem、bank conflict、occupancy）。

**分支 C**：
- 完整光追管线从"光线生成"到"最终去噪"的端到端架构。
- Russian roulette 如何在无偏估计与递归控制之间找平衡。
- OptiX 的"间接调用 + SBT"设计如何优雅地支持多材质管理。

