# Flash-Attention-2 前向源码精读笔记

> 本文件是分支 B 的**源码精读分析模板**，供学生逐节填写。
> 所有 `> TODO 填入...` 块均需替换为实际分析内容，最终字数应 ≥ 1500 字。
> 完成后作为交付物之一提交。

---

## 0. 概述

> TODO 填入：
> - 选定的 FA-2 kernel 文件名（例如 `flash_fwd_hdim64_fp16.cu`）
> - head dimension / batch size / sequence length 的默认配置
> - 为什么 FA-2 比 naive attention 在 memory 上更优（O(N^2) → O(N·d)）
> - 与本分支的简化复现的主要区别（头数、tile 大小、mma 实现方式）

---

## 1. Tile 划分方案

### 1.1 (Br, Bc) 选择依据

FA-2 将 Q 矩阵按行分块（每块 Br 行），将 K/V 矩阵按列分块（每块 Bc 列）。
外层循环遍历 Q 行块，内层循环遍历 K/V 列块。

```
Q[N, d]：
  ┌──────────────────────────────┐
  │  Q tile 0 [Br, d]  ← block 0 │  outer loop: q_block = 0
  │  Q tile 1 [Br, d]  ← block 1 │  outer loop: q_block = 1
  │  ...                          │
  └──────────────────────────────┘

K[N, d]（每次迭代加载一个 Bc 块）：
  ┌────────┬────────┬──────────┐
  │ Bc 块 0 │ Bc 块 1 │   ...    │  inner loop
  └────────┴────────┴──────────┘
```

> TODO 填入：
> - Br 和 Bc 的实际选择值（本题 Br=64, Bc=64，理由是 smem 约束）
> - smem 计算：smem_Q = Br×d×2 bytes，smem_K = Bc×d×2 bytes，smem_V = Bc×d×2 bytes
> - 总 smem 与 GPU 默认上限（48 KB）和 opt-in 上限（共享内存最大值）的关系
> - 为什么 Br 通常 ≥ Bc（更大的 Q 块有助于提高寄存器利用率）

| 参数 | 数值 | 说明 |
|------|------|------|
| Br | 64 | Q tile 行数（外层循环步长） |
| Bc | 64 | K/V tile 列数（内层循环步长） |
| smem_Q | > TODO KB | Br × d × 2 bytes |
| smem_K | > TODO KB | Bc × d × 2 bytes |
| smem_V | > TODO KB | Bc × d × 2 bytes |
| smem_S | > TODO KB | Br × Bc × 4 bytes（FP32） |
| 总 smem | > TODO KB | 需要 < opt-in 上限 |

### 1.2 Outer / Inner Loop 结构图

> TODO 填入（含伪代码或 ASCII 图）：

```
outer loop: q_block in 0..ceil(N/Br):
  加载 Q tile[q_block] 到 smem_Q           // 一次
  初始化 m_i = -inf, l_i = 0, o_acc = 0    // per-row

  inner loop: kv_block in 0..ceil(N/Bc):
    if causal and kv_block * Bc > q_end: break  // 提前退出

    加载 K tile[kv_block] 到 smem_K         // 每轮
    加载 V tile[kv_block] 到 smem_V         // 每轮

    S = Q_tile @ K_tile^T * scale            // (Br, Bc)
    apply causal mask                        // element-wise

    online softmax 更新（见第 2 节）

  归一化：O[q_block] = o_acc / l_i          // 写出一次
```

---

## 2. Online Softmax 推导

### 2.1 问题背景

标准 attention softmax 需要先算出全行 N 个 score，再求 max、sum，最后归一化。
这要求 O(N^2) 的中间矩阵（score matrix S[N, N]），内存占用巨大。

FA-2 的核心创新：**用"在线"（online）方式逐块维护 softmax 的统计量**，
无需存储完整的 S 矩阵。

### 2.2 推导过程

设已处理了 j 块 K/V，当前 running 状态为 (m_old, l_old)，
现在要合并第 j+1 块的 score 行向量 s_j（长度 Bc）：

```
1. m_new = max(m_old, max(s_j))

2. P_j = exp(s_j - m_new)             // Bc 维向量，数值稳定

3. l_new = l_old * exp(m_old - m_new)  // 修正旧的 sum
         + sum(P_j)                    // 加入新块的贡献

4. o_acc = o_acc * exp(m_old - m_new)  // 修正旧的 output
         + P_j @ V_j                   // 加入新块的贡献
```

> TODO 填入完整的数学推导（≥ 200 字），包括：
> - 为什么乘以 `exp(m_old - m_new)` 是数值稳定的（而不是先 exp 再除）
> - 当 `m_old = m_new`（新块没有更大的 max）时，公式退化为何形式
> - 最终归一化：`O = o_acc / l_final` 为什么等价于标准 softmax 的结果
> - FP16 精度下的数值稳定性：为什么 exp 的参数要减去 max（避免 overflow）

### 2.3 公式总结

```
初始化：
  m = -∞,  l = 0,  o = 0

每块 K_j / V_j：
  s_j     = Q @ K_j^T / sqrt(d)         ∈ R^{Br × Bc}
  m_new   = max(m, row_max(s_j))
  α       = exp(m - m_new)              # 衰减因子（≤ 1）
  P_j     = exp(s_j - m_new)            ∈ R^{Br × Bc}
  l_new   = α * l + row_sum(P_j)
  o       = α * o + P_j @ V_j
  m, l    ← m_new, l_new

最终：
  O = o / l                             ∈ R^{Br × d}
```

### 2.4 Causal Mask 的 Online Softmax 处理

> TODO 填入：
> - Block 级提前退出：当 `kv_start > q_end` 时整个 KV 块都被 mask，直接 break
> - Element 级 mask：`if kv_col > q_row: s = -1e9`（大负数，exp 后趋向 0）
> - 为什么用 -1e9 而非 -inf（避免 FP16 NaN 的常见坑）

---

## 3. Shared Memory 布局与 Bank Conflict

### 3.1 smem layout（以 Br=64, Bc=64, d=64 为例）

```
smem 布局（FP16）：
offset 0     : smem_Q  [64][64] = 8192 bytes = 8 KB
offset 8192  : smem_K  [64][64] = 8192 bytes = 8 KB
offset 16384 : smem_V  [64][64] = 8192 bytes = 8 KB
offset 24576 : smem_S  [64][64] (FP32) = 16384 bytes = 16 KB
总计          : 40 KB（< 48 KB 默认 smem，无需 opt-in）
```

> TODO 填入：
> - 实际 FA-2 源码中 smem 的分配方式（静态 vs 动态 extern __shared__）
> - smem_Q / smem_K / smem_V 是行主序（row-major）还是列主序？为什么？
> - 不同 layout 对 `Q @ K^T` 的 bank conflict 影响（load pattern 分析）

### 3.2 Bank Conflict 分析

> TODO 填入（每种访问模式独立分析）：

**smem_Q 读取**（计算 S = Q @ K^T 时）：
```
每个 thread 读 smem_Q[row][0..d-1]（一整行），各行 thread 访问不同 bank
访问模式：thread i → smem_Q[i][k]，k 从 0 递增
bank id = (addr / 4) % 32
是否有 conflict：TODO 填入
```

**smem_K 读取**（计算 dot product 时）：
```
每个 thread 读 smem_K[j][0..d-1]（同一行，广播）
多个 thread 访问同一 bank → 需要 swizzle 或 padding 消除
TODO 分析是否有 conflict
```

> TODO 填入：
> - FA-2 官方实现是否使用 smem swizzle（参考 flash_attn 的 smem 声明）
> - 本简化版（main.cu）的 smem 声明方式，以及是否存在 bank conflict
> - Nsight Compute 中如何量化 bank conflict（指标名称）

---

## 4. Causal Attention 优化

> TODO 填入：
> - FA-2 的 causal mask 实现（tile 级 + element 级的两层判断）
> - Causal 时 inner loop 的迭代数如何减少（平均约 N/2 次 vs non-causal 的 N 次）
> - 为什么 causal 版本比 non-causal 快约 2×（计算量减半）
> - Causal mask 的正确性验证方法（从 CPU ref 对比）

```
Causal tile-level 判断：
if kv_block * Bc > q_start + Br - 1:
    break  // 整个 KV 块完全在对角线右侧，全部 mask

Causal element-level 判断（在 smem_S 赋值后）：
if (kv_start + j) > (q_start + row):
    smem_S[row][j] = -1e9f
```

---

## 5. 与 PyTorch `scaled_dot_product_attention` 的对比

> TODO 填入（实测数据）：

| 指标 | PyTorch SDPA | FA-2 简化版（本题） | 差距说明 |
|------|-------------|-------------------|---------|
| max rel error | — | TODO | — |
| 耗时 (ms) | TODO | TODO | — |
| TFLOPS | TODO | TODO | — |
| Tensor Core 利用率 | TODO | TODO | TODO |
| smem bank conflict | TODO | TODO | TODO |

验证命令（Python）：
```python
import torch

N, d = 2048, 64
Q = torch.randn(N, d, dtype=torch.float16, device='cuda')
K = torch.randn(N, d, dtype=torch.float16, device='cuda')
V = torch.randn(N, d, dtype=torch.float16, device='cuda')

with torch.no_grad():
    ref = torch.nn.functional.scaled_dot_product_attention(
        Q.unsqueeze(0), K.unsqueeze(0), V.unsqueeze(0), is_causal=True
    ).squeeze(0)

# simple = 从 main.cu 读取 GPU 输出（通过 ctypes 或临时文件）
# rel_err = (simple - ref).abs().max() / (ref.abs().max() + 1e-6)
# print(f"Relative error: {rel_err}")
# assert rel_err < 1e-2
```

---

## 6. FA-3（Hopper）差异预览

FA-3（arXiv 2407.08608）在 FA-2 基础上引入了 Hopper 特性：

| 特性 | FA-2（Ampere mma.sync） | FA-3（Hopper TMA + wgmma） |
|------|------------------------|---------------------------|
| 数据加载 | `__ldg` / shared mem load | TMA `cp.async.bulk.tensor` |
| 矩阵乘 | `mma.sync.aligned` | `wgmma.mma_async` |
| 同步 | `__syncthreads` | `mbarrier.arrive/wait` |
| Warp 分工 | 所有 warp 均做 | producer/consumer warp specialization |
| Pipeline 深度 | 2 | 2–4 |
| 理论加速比 | 1× | ~1.5–2× |

> TODO 填入：
> - FA-3 中 Q 是否也通过 TMA 加载（与 FA-2 的差异）
> - Warp specialization 如何与 online softmax 结合
> - FA-3 的 double-buffer 策略（pipeline depth = 2 的 ping-pong）

---

## 7. 性能对比表（完整）

> TODO 填入（Nsight Compute 实测）：

| 指标 | CPU 参考 | FA-2 简化版 | 官方 FA-2 |
|------|---------|------------|---------|
| 耗时 (ms) | TODO | TODO | TODO |
| TFLOPS | — | TODO | TODO |
| % 理论峰值 | — | TODO | TODO |
| Tensor Core 利用率 | — | TODO | TODO |
| L2 命中率 | — | TODO | TODO |
| smem bank conflict | — | TODO | TODO |
| max rel error（vs CPU） | — | TODO | TODO |

---

## 8. 收获与疑问

### 收获

> TODO 填入至少 5 条具体收获，例如：
> 1. online softmax 的 rescale 公式 `exp(m_old - m_new)` 是一个优雅的数值稳定技巧……
> 2. ……

### 疑问

> TODO 填入至少 3 条疑问，以及尝试解答：
> 1. Q：为什么 FA-2 不把 Q tile 也放进 inner loop（只加载一次 Q 是否总是最优）？
>    A（尝试）：……
> 2. ……

---

## 9. 关键观察点

| 观察点 | 解释 |
|--------|------|
| 在线 softmax 是否无偏 | 是的，数学上等价于标准 softmax（推导见第 2 节） |
| IO 减少的本质 | 避免了 O(N^2) 的中间矩阵，而非减少 Q/K/V 的读次数 |
| Tile 大小的三角权衡 | 大 Br → 寄存器多、占用低；大 Bc → smem 多、带宽利用高 |
| Causal 的计算量 | 约为 non-causal 的 50%（仅计算下三角） |
| rescale 的精度影响 | 每次 rescale 引入约 1 ULP FP16 误差，累积后 max_err ≈ 1e-2 |

---

*笔记模板版本：2025-05。请在完成精读后删除所有 `> TODO` 提示行，保留实际内容。*
