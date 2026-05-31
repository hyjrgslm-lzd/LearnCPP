# I5 — Flash-Attention-2 风格前向

## 目标

`[I5-T01]` (main.cu:2) 练习 I5：Flash-Attention-2 风格前向（2D tile + online softmax）。
`[I5-T02]` (main.cu:4) 学习目标：2D tile（Br × Bc）、outer/inner loop、on-device online softmax、causal early exit、IO 字节数对比、理解为何不需要 N×N softmax buffer。

完整实现 Flash-Attention-2 前向：2D tile 划分（Br × Bc）、outer loop 遍历 Q blocks、inner loop 遍历 K/V blocks、on-device online softmax、局部归一化。对标 cuDNN fused attention，理解现代 attention 的 IO 最优性。

## 问题规模

`[I5-T03]` (main.cu:36) 问题规模（与 I4 保持一致）。
`[I5-T04]` (main.cu:37) 序列长度 N。
`[I5-T05]` (main.cu:38) head dimension（`d_k = d_v`）。
`[I5-T06]` (main.cu:39) `scale = 1/sqrt(64)`。
`[I5-T07]` (main.cu:42) FA-2 tile 大小说明。
`[I5-T08]` (main.cu:43) 选择依据：`2*Br*d*sizeof(fp16) + 2*Bc*d*sizeof(fp16) < smem 上限`。
`[I5-T09]` (main.cu:44) 实例：`Br=64, Bc=64, d=64 → 2*64*64*2 + 2*64*64*2 = 32 KB`（典型 smem 限制内）。
`[I5-T10]` (main.cu:45) Q tile 行数。
`[I5-T11]` (main.cu:46) K/V tile 列数（沿 seq 维度）。

| 参数 | 值 |
|------|----|
| 序列长度 N | 1024 |
| Head dimension d | 64 |
| Q tile 大小 Br | 64 |
| K/V tile 大小 Bc | 64 |
| scale | `1/sqrt(64) = 0.125` |
| 数据类型 | FP16 输入，FP32 累加 |

## FA-2 核心思想

**为什么不需要 N×N softmax buffer？**

Flash-Attention-2 的关键洞察：
- 每个 block 只保留 `running max m_i` 和 `running lse l_i`（各 Br 个标量）
- 不需要写出完整的 Attention Score 矩阵 `S[N, N]`
- 输出 O 在 inner loop 中增量累积，每轮用修正因子 `exp(m_old - m_new)` 更新
- 最终 `O[i] = O_acc[i] / l_i`（归一化在 outer loop 结束后一次完成）

## 前置知识

- FA-2 outer loop：按 Br 划分 Q，每个 block 处理一个 Q tile
- FA-2 inner loop：按 Bc 划分 K/V，逐步更新 partial attention
- online softmax：每读入新 K block，更新 max 和 sum，同时修正之前输出
- Causal：inner loop block 级判断 `kv_block_idx * Bc > q_start` 即可跳过

## 设备工具

`[I5-T12]` (main.cu:50) 设备工具：warp reduce max / sum。

## Flash-Attention-2 kernel

`[I5-T13]` (main.cu:67) Kernel：Flash-Attention-2 风格前向。
`[I5-T14]` (main.cu:69) 为什么不需要 N×N softmax buffer：每个 block 只保留 `running max m_i` 和 `running lse l_i`（各 Br 个标量），不需要写出完整 `S[N, N]`，输出 O 在 inner loop 中增量累积，每轮用 `exp(m_old - m_new)` 修正。
`[I5-T15]` (main.cu:75) 每个 CUDA block 处理一个 Q tile（Br 行查询）。
`[I5-T16]` (main.cu:77) TODO [必做] 步骤 2：实现 block 划分逻辑（外层 outer loop）。
`[I5-T17]` (main.cu:78) TODO [必做] 步骤 3：内层 inner loop，融合 online softmax。
`[I5-T18]` (main.cu:79) TODO [必做] 步骤 4：`P_block · V_block` 累积到 O。
`[I5-T19]` (main.cu:80) TODO [必做] 步骤 5：causal 版本（block 级 + 元素级 mask）。
`[I5-T20]` (main.cu:86) TODO [必做] 步骤 2：block 划分模板。
`[I5-T21]` (main.cu:93) 共享内存声明（待学生填写尺寸后取消注释）。
`[I5-T22]` (main.cu:98) 寄存器：每行的 running state（每个 thread 负责 `Br/blockDim.y` 行）。
`[I5-T23]` (main.cu:103) TODO [必做] 步骤 3：inner loop 模板（遍历 K/V tiles）。
`[I5-T24]` (main.cu:140) TODO [必做] 步骤 4：归一化并写出 O。
`[I5-T25]` (main.cu:145) stub：输出 0。

## 必做任务（对应 `main.cu` 中的 `// TODO [必做]` 标注）

1. **步骤 1**：构造 Q、K、V（shape: `[N, d]`），FP16 初始化。
2. **步骤 2**：实现 block 划分逻辑：定义 Br 和 Bc，outer loop 和 inner loop 结构。
3. **步骤 3**：inner loop 中融合 online softmax：
   - 读 K block，计算 `S_block = Q_block · K_block^T / sqrt(d)`
   - 更新 `m_i`（running max）和 `l_i`（running sum of exp）
   - 用 `exp(m_old - m_new)` 修正之前的 `O_acc`
4. **步骤 4**：`P_block · V_block` 累积到 `O_acc`。
5. **步骤 5**：causal 版本：inner loop 检查 `kv_block_idx <= q_block_idx`。
6. **步骤 6**：FA-2 vs unfused（I4）输出差异 `< 1e-2`（FP16，ε 阈值宽松）。

## 进阶任务

`[I5-T26]` (main.cu:155) TODO [进阶] FP16 输入 FP32 累加（已在 stub 结构中留位置）。
`[I5-T27]` (main.cu:159) TODO [进阶] Hopper TMA 异步加载 Q/K/V tile（对接模块 G4）。
`[I5-T28]` (main.cu:163) TODO [进阶] MQA/GQA 支持（multi-query / grouped query attention）。

- FP16 输入 FP32 累加（已在 stub 结构中预留）
- Hopper TMA 异步加载 Q/K/V tile（对接模块 G4 producer-consumer warp 流水线）
- MQA/GQA 支持（multi-query / grouped-query attention）

## CPU 参考

`[I5-T29]` (main.cu:167) CPU 参考：标准 attention（`O(N^2*d)` 实现，完全正确）。

## 主程序

`[I5-T30]` (main.cu:210) 主程序入口。
`[I5-T31]` (main.cu:220) 打印 `M N d_k Br Bc causal scale`。
`[I5-T32]` (main.cu:224) 检查 smem 需求。
`[I5-T33]` (main.cu:226) 打印 smem 估算。
`[I5-T34]` (main.cu:230) TODO [必做] 步骤 1：构造 Q、K、V（FP16）。
`[I5-T35]` (main.cu:242) CPU 参考。
`[I5-T36]` (main.cu:244) 打印 CPU 参考开始。
`[I5-T37]` (main.cu:246) 打印 CPU 参考完成。
`[I5-T38]` (main.cu:249) GPU 分配。
`[I5-T39]` (main.cu:259) 启动配置：每个 block 处理一个 Q tile（Br 行），`blockDim.x = Bc`。
`[I5-T40]` (main.cu:261) `blockDim` 可根据 d 调整。
`[I5-T41]` (main.cu:262) 打印启动配置。
`[I5-T42]` (main.cu:263) 打印 tile 行列说明。
`[I5-T43]` (main.cu:269) Flash-Attention-2 kernel。
`[I5-T44]` (main.cu:285) 理论 IO：FA-2 每元素 Q/K/V 读一次，O 写一次（忽略 tile 重加载）。
`[I5-T45]` (main.cu:289) flash_attn_v2 输出。
`[I5-T46]` (main.cu:295) TODO [必做] 步骤 6：对比 FA-2 与 unfused（I4）的 IO 字节数。
`[I5-T47]` (main.cu:299) IO 估算（理论值）。
`[I5-T48]` (main.cu:306) 打印理论 IO 对比标题。
`[I5-T49]` (main.cu:308) 打印 unfused IO。
`[I5-T50]` (main.cu:309) 打印 FA-2 IO。
`[I5-T51]` (main.cu:310) 打印 IO 减少百分比。
`[I5-T52]` (main.cu:314) 打印验收目标。
`[I5-T53]` (main.cu:315) 打印 stub 行为说明。
`[I5-T54]` (main.cu:322) 收尾提示：用 ncu 分析 smem 利用率。

## 验收标准

| 指标 | 目标 |
|------|------|
| FA-2 vs unfused 输出差异 | `< 1e-2`（FP16，宽松阈值）|
| IO 字节数减少 | 相对 unfused 减少 `>= 50%`（理论） |
| causal mask 正确性 | 下三角无误 |
| 寄存器占用 | `< 120 per thread` |

## IO 字节数对比（理论）

```
unfused（I4）：
  读 Q/K/V：3 × N × d × sizeof(fp16)
  写/读 S：  2 × N × N × sizeof(fp32)   ← 瓶颈
  写 O：     N × d × sizeof(fp16)
  总计（N=1024, d=64）：约 8.5 MB

FA-2：
  读 Q/K/V：3 × N × d × sizeof(fp16)
  写 O：     N × d × sizeof(fp16)
  总计：     约 0.5 MB   ← IO 减少 ~94%
```

## 关键公式

```
// online softmax 修正因子（每轮 inner loop 后）：
m_new = max(m_old, max(S_block))
O_acc *= exp(m_old - m_new)            // 修正之前的累积输出
l_new = l_old * exp(m_old - m_new) + sum(exp(S_block - m_new))

// causal block 级判断：
if (causal && kv_block_idx * Bc > q_start + Br - 1) break;

// 最终归一化：
O[i] = O_acc[i] / l_final
```

## Shared Memory 需求估算

```
smem_Q  = Br × d × sizeof(fp16) = 64 × 64 × 2 = 8 KB
smem_K  = Bc × d × sizeof(fp16) = 64 × 64 × 2 = 8 KB
smem_V  = Bc × d × sizeof(fp16) = 64 × 64 × 2 = 8 KB
总计：24 KB（在 48 KB 默认 smem 限制内）
```

## 常见坑

- online softmax 修正公式写错（应是 `old_P *= exp(old_max - new_max)`）
- inner loop 中忘记累积 O 结果，导致只有最后一个 block 的贡献
- causal mask 检查应在 block 级和元素级都做（block 级可提前 exit）
- FP16 输入时，累加器精度丢失（必须用 FP32）
- 跨 block 的 softmax 修正需要 `__syncthreads()`

## 复盘问题

1. FA-2 的 outer loop 和 inner loop 分别遍历 Q 和 K/V 的哪些维度？
2. online softmax 在每轮 inner loop 后如何修正之前的 O 输出？
3. causal attention 在 FA-2 框架内如何实现（block 级还是元素级判断）？
4. IO 复杂度相对 unfused 的改进来自哪里（smem 重用）？
5. Hopper TMA 如何与 FA-2 的 producer-consumer warp 流水线结合？
6. 如何用 Nsight Compute 验证 shared memory 的有效利用率？

## 编译与运行

```bash
cmake --build build --target I5_flash_attention_v2_style
./exercises/I5_flash_attention_v2_style/I5_flash_attention_v2_style

# smem 利用率与 IO 分析：
ncu --metrics l1tex__t_bytes,smsp__sass_inst_executed ./I5_flash_attention_v2_style
ncu --metrics sm__shared_memory_load_transactions,sm__shared_memory_store_transactions \
    ./I5_flash_attention_v2_style
```

## 参考文献

- FlashAttention-2 Paper: arXiv 2307.08691
- Flash-Attention GitHub: https://github.com/Dao-AILab/flash-attention
- CUTLASS 3.x FlashAttention example: https://github.com/NVIDIA/cutlass/tree/main/examples
- cuDNN Fused Attention: https://docs.nvidia.com/deeplearning/cudnn/latest/
- NVIDIA Blog: "Scaling Transformers with Efficient Attention"

## 输出对照（printf / std::puts 原文）

- `[I5-T31]` (main.cu:221) 原文：`M=%d N=%d d_k=%d Br=%d Bc=%d  causal=%s  scale=%.4f` → 现：保持英文不变
- `[I5-T33]` (main.cu:227) 原文：`共享内存需求估算：%zu KB  (Br=%d Bc=%d d=%d)` → 现：`Shared memory estimate: %zu KB  (Br=%d Bc=%d d=%d)`
- `[I5-T36]` (main.cu:244) 原文：`正在计算 CPU 参考（N=%d d=%d，可能需要数秒）...` → 现：`Computing CPU reference (N=%d d=%d, may take a few seconds)...`
- `[I5-T37]` (main.cu:246) 原文：`CPU 参考完成。` → 现：`CPU reference done.`
- `[I5-T41]` (main.cu:262) 原文：`启动配置：grid=(%d,1,1)  block=(%d,1,1)` → 现：`Launch config: grid=(%d,1,1)  block=(%d,1,1)`
- `[I5-T42]` (main.cu:263) 原文：`  每个 block 处理 Q tile [%d 行]，内层遍历 K/V tile [%d 列]` → 现：`  each block handles Q tile [%d rows], inner loop walks K/V tiles [%d cols]`
- `[I5-T45]` (main.cu:290) 原文：`[flash_attn_v2]  %.3f ms  max_err=%.2e  有效带宽=%.1f GB/s  (stub: 期望 err < 1e-2 实现后)` → 现：`[flash_attn_v2]  %.3f ms  max_err=%.2e  effective_bw=%.1f GB/s  (stub: expect err < 1e-2 once implemented)`
- `[I5-T48]` (main.cu:306) 原文：`理论 IO 对比：` → 现：`Theoretical IO comparison:`
- `[I5-T49]` (main.cu:308) 原文：`  unfused：%.1f MB（含 S[%dx%d] 中间矩阵）` → 现：`  unfused: %.1f MB (includes S[%dx%d] intermediate)`
- `[I5-T50]` (main.cu:309) 原文：`  FA-2：   %.1f MB（无 N×N buffer）` → 现：`  FA-2:    %.1f MB (no NxN buffer)`
- `[I5-T51]` (main.cu:310) 原文：`  IO 减少：%.0f%%` → 现：`  IO reduction: %.0f%%`
- `[I5-T52]` (main.cu:314) 原文：`验收目标：FA-2 vs CPU ref max_err < 1e-02（FP16 累加），ε 阈值宽松于 I4` → 现：`Acceptance: FA-2 vs CPU ref max_err < 1e-02 (FP16 accumulator), epsilon looser than I4`
- `[I5-T53]` (main.cu:315) 原文：`当前 stub 输出全零 → CPU check 会报 mismatch，这是预期行为。` → 现：`Current stub outputs all zeros; CPU check will report mismatch (expected).`
- `[I5-T54]` (main.cu:322) 原文：`[I5] 完成。用 ncu --metrics l1tex__t_bytes,smsp__sass_inst_executed ./I5_flash_attention_v2_style 分析 smem 利用率。` → 现：`[I5] done. Use ncu --metrics l1tex__t_bytes,smsp__sass_inst_executed ./I5_flash_attention_v2_style to analyze smem utilization.`
