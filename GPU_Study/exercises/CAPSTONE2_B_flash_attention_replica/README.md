# 结课项目 2 — 分支 B：Flash-Attention-2 前向源码精读 + 简化复现

## 题目概述

阅读 Flash-Attention-2 repo 的 `csrc/flash_attn/` 下的前向 kernel（FP16 + `mma.sync`），
理解 tile 划分与在线 softmax + rescale 的流程，绘出数据流图和 shared memory 访问模式，
再用 ≤800 行的 CUDA kernel 复现一个功能等价的简化版
（FP16、causal/non-causal、无 head group 复杂度），
与 CPU 参考实现（或 PyTorch `scaled_dot_product_attention`）对齐精度（ε < 1e-2）。

**问题规模**：B=4, H=16, N=2048, d\_k=64，每次调用处理单头的 N×d\_k attention

---

## 硬件与依赖要求

| 项目 | 要求 |
|------|------|
| GPU | sm\_80+（Ampere mma.sync）；sm\_90a 可进阶体验 TMA 版本 |
| CUDA Toolkit | 13.x 或更新 |
| CMake | 3.28+ |
| Python（可选） | 3.10+，用 `torch` 验证精度 |
| Nsight Compute | `ncu` 命令可用（性能采集） |

无需 CUTLASS；无需 Hopper 实机（Ampere A100/RTX 3090/4090 均可运行）。

---

## 必做任务清单

- [ ] **步骤 1**：克隆 Flash-Attention repo，定位目标 kernel 文件，记录配置
- [ ] **步骤 2**：分析 tile 划分（Br×Bc），绘制 Q/K/V tile 划分图
- [ ] **步骤 3**：分析在线 softmax 流程，推导 rescale 公式（写入 NOTES.md）
- [ ] **步骤 4**：分析 smem 布局与 bank conflict（swizzle 参数）
- [ ] **步骤 5**：分析 causal masking（tile-level + element-level）
- [ ] **步骤 6**：撰写精读笔记（≥1500 字，输出至 `NOTES.md`）
- [ ] **步骤 7**：实现 `flash_attention_fwd` kernel（`main.cu` 中 TODO [必做] 框架）
- [ ] **步骤 8**：编译运行，验证 max\_rel\_error < 1e-2
- [ ] **步骤 9**：用 Nsight Compute 采集性能数据，输出 TFLOPS 对比表

---

## 在线 Softmax 算法

```
初始化：m = -∞,  l = 0,  o = 0（per-row）

for j in 0 .. ceil(N/Bc):
    if causal and kv_start > q_end: break     # 提前退出

    S_j = Q_tile @ K_j^T * scale              # (Br, Bc)
    apply causal mask（element-wise）

    m_new   = max(m, row_max(S_j))
    P_j     = exp(S_j - m_new)
    l_new   = l * exp(m - m_new) + row_sum(P_j)
    o       = o * exp(m - m_new) + P_j @ V_j
    m, l   ← m_new, l_new

output = o / l                                # 归一化
```

关键数值稳定性技巧：
- 所有 `exp` 的参数都减去当前 `max`（避免 FP16 overflow）
- `l` 的 rescale 系数 `exp(m_old - m_new) ≤ 1`，不会放大数值误差
- 输出最终除以 `l` 而不是边累加边除（减少精度损失）

---

## 编译与运行

### 编译步骤

```bash
# 配置（从 exercises/ 目录）
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# 仅编译分支 B
cmake --build build --target CAPSTONE2_B_flash_attention_replica --config Release -j8

# Windows
cmake --build build-vs2026 --target CAPSTONE2_B_flash_attention_replica --config Release
```

### 运行

```bash
# Linux / WSL
./build/CAPSTONE2_B_flash_attention_replica/CAPSTONE2_B_flash_attention_replica

# Windows
build-vs2026\Release\CAPSTONE2_B_flash_attention_replica.exe
```

预期输出（stub 阶段）：

```
[CAPSTONE2_B] Flash-Attention-2 mini 复现
=== Device 0: NVIDIA A100-SXM4-80GB ===
  Compute Capability : 8.0
  ...
问题规模：B=4  H=16  N=2048  d_k=64  causal=true  scale=0.1250
Tile 参数：Br=64  Bc=64

── 理论 IO 对比
  Unfused（含 S[2048×2048] 中间矩阵）：xxx.x MB
  FA-2（无 N×N buffer）：xxx.x MB
  IO 减少：xx%

正在计算 CPU 参考（N=2048 d=64，可能需要数秒）...
CPU 参考完成。

── FA-2 kernel 启动配置：grid=(32,1)  block=(64,1)
   每 block 处理 Q[64 行]，内层遍历 32 个 KV tile

── 路径：FA-2 简化版前向（TODO 实现后替换 stub）
── 正确性验证（vs CPU FP32 参考）
  [fa2_simplified] FAIL (stub 阶段预期 FAIL)  max_rel_err=x.xxxe+00  (阈值 1e-2)
  -> 实现 main.cu 中所有 TODO [必做] 步骤后重新运行。

── 性能汇总
变体                           |   ms/iter |     TFLOPS | 有效带宽 GB/s
fa2_simplified (stub)          |     x.xxx |     x.xxxx |          x.x
```

实现 TODO 后预期输出：
```
  [fa2_simplified] PASS  max_rel_err=x.xxxe-03  (阈值 1e-2)
fa2_simplified              |     x.xxx |    xx.xxxx |        xxx.x
```

### Nsight Compute 性能采集

```bash
# 完整采集
ncu --set full -o capstone_b.ncu-rep ./CAPSTONE2_B_flash_attention_replica

# 关键指标
ncu --import capstone_b.ncu-rep --csv \
    --metrics \
    sm__pipe_tensor_cycles_active.avg.pct_of_peak_sustained_active,\
l1tex__data_bank_conflicts_pipe_lsu_mem_shared_op_ld.sum,\
l1tex__data_pipe_lsu_wavefronts_mem_shared_op_ld.sum \
    > metrics_b.csv

# compute-sanitizer（验证无 race condition）
compute-sanitizer --tool racecheck ./CAPSTONE2_B_flash_attention_replica
```

---

## 实现指南

### main.cu 中的 TODO 步骤

1. **步骤 2**（Q 外层块）：每个 block 协作加载 `smem_Q[Br][HEAD_DIM]`，初始化 online softmax 状态
2. **步骤 3**（KV 内层块）：加载 `smem_K`、`smem_V`，计算 `S = Q_tile @ K_tile^T * scale`，执行 online softmax 更新
3. **步骤 4**（output 累加）：用 `exp(m_old - m_new)` rescale `o_acc`，累加 `P @ V`
4. **步骤 5**（causal mask）：block-level break + element-level `-1e9f` mask

### 常见坑

1. **rescale 公式写错**：忘记对 `l` 和 `o_acc` 都乘以 `exp(m_old - m_new)`，导致数值爆炸
2. **causal 条件写反**：`j > i` 应 mask，容易写成 `j < i`
3. **smem 不足**：Br=64, Bc=64, d=64 时总 smem 约 40 KB，注意不要超过 48 KB 默认上限
4. **bank conflict**：K/V tile 加载时多 thread 访问同列 → 考虑 padding 或 swizzle
5. **序列末尾边界**：当 `N` 不是 `Br` 或 `Bc` 整数倍时，需要边界检查
6. **精度问题**：FP16 累加，softmax 前后均使用 FP32 中间寄存器，最后转回 FP16 输出

### 进阶挑战

- 用 `mma.sync`（`__hmma_m16n8k16`）替代 scalar 循环计算 `Q @ K^T`，提升 Tensor Core 利用率
- 添加 Hopper TMA 版本（参考 G4 模式）
- 支持多头（B×H 个 head 并行）
- 实现后向传播

---

## 验收标准

| 项目 | 验收点 |
|------|--------|
| 精读笔记 | `NOTES.md` ≥1500 字，公式推导清晰、图表完整 |
| kernel 编译 | 无编译错误，无 compute-sanitizer 错误 |
| 精度 | max\_rel\_error < 1e-2（FP16 量化误差） |
| 性能 | ≥ 官方 FA-2 的 70% TFLOPS（取决于实现方式） |
| Nsight 报告 | Tensor Core 利用率 ≥ 50%，L2/L1 缓冲命中率 > 70% |

---

## 复盘问题（自检）

1. FA-2 的"在线 softmax"相比于"先算 attention matrix 再 softmax"有什么优势？
2. Rescale 公式 `l_new = l_old * exp(m_old - m_new)` 从何而来？能推导吗？
3. Causal attention 在 tile-based 实现中如何避免 K/V 的多次重复加载？
4. Tile 大小 (Br, Bc) 的选择对性能有什么影响？为什么 Br 通常比 Bc 大？
5. 你的简化版与官方 FA-2 的性能差距来自哪些方面？
6. 如何验证"bank conflict 被消除"？
7. FP16 的量化误差如何影响最终 attention 的精度？

---

## 参考资料

- [Flash-Attention-2 论文](https://arxiv.org/abs/2307.08691)
- [Flash-Attention GitHub](https://github.com/Dao-AILab/flash-attention)
- [Flash-Attention-3 论文（进阶）](https://arxiv.org/abs/2407.08608)
- [PyTorch scaled_dot_product_attention 文档](https://pytorch.org/docs/stable/generated/torch.nn.functional.scaled_dot_product_attention.html)

---

## 交付物清单

| 交付物 | 文件 | 说明 |
|--------|------|------|
| 源码精读笔记 | `NOTES.md` | ≥1500 字 Markdown |
| 实现代码 | `main.cu` | TODO 部分已实现 |
| Nsight 报告 | `capstone_b.ncu-rep` | `ncu --set full` 采集 |
| 性能对比表 | 程序运行输出 | 截图或重定向至 txt |
| Python 验证（可选） | `verify_b.py` | torch SDPA 对比精度 |
