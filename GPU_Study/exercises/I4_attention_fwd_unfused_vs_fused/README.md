# I4 — Attention 前向：Unfused（3 kernel）vs Fused（1 kernel）

## 目标

`[I4-T01]` (main.cu:2) 练习 I4：Attention 前向 — Unfused（3 kernel）vs Fused（1 kernel）。
`[I4-T02]` (main.cu:4) 学习目标：unfused 三 kernel 路径、fused 单 kernel 路径、FP16/FP32 数据流、causal mask、ncu 对比 global memory traffic。

对标 Flash-Attention-2，但从"三个独立 kernel"开始（unfused），逐步融合 softmax 到 attention block。理解为什么融合减少内存访问，以及融合的代价（寄存器压力、shared memory 竞争）。

## 问题规模

`[I4-T03]` (main.cu:33) 问题规模（单 head 版本，batched 通过 blockIdx 扩展）。
`[I4-T04]` (main.cu:36) 序列长度 N。
`[I4-T05]` (main.cu:37) head dimension。
`[I4-T06]` (main.cu:38) value dimension（= d_k）。
`[I4-T07]` (main.cu:39) `scale = 1 / sqrt(64) = 0.125`。

| 参数 | 值 |
|------|----|
| Batch B | 4 |
| Heads H | 16 |
| 序列长度 N | 1024 |
| Head dimension d_k | 64 |
| Value dimension d_v | 64 |
| scale | `1/sqrt(64) = 0.125` |
| 数据类型 | FP16 输入，FP32 累加 |

## 前置知识

- 标准 attention：`out = softmax(Q · K^T / sqrt(d_k)) · V`
- Unfused：三个 kernel（QK^T → softmax → ·V），之间需要 global memory 传递中间结果
- Fused：tile 划分，softmax 在 shared memory 中完成，输出直接累积
- Causal mask：下三角，`j > i` 位置设为 `-inf`

## 设备工具

`[I4-T08]` (main.cu:43) 设备工具：warp reduce max / sum。

## Unfused 三 kernel 路径

`[I4-T09]` (main.cu:60) Kernel 1（unfused）：QK^T matmul → S[N, N]。
`[I4-T10]` (main.cu:61) `Q: [N, d_k]  K: [N, d_k]  S: [N, N]`（scale 在此步骤乘入）。
`[I4-T11]` (main.cu:63) TODO [必做] 步骤 2a：实现 Q · K^T，FP16 输入 FP32 累加。
`[I4-T12]` (main.cu:69) 简单实现：每个 thread 计算 `S[row, col]`。
`[I4-T13]` (main.cu:75) TODO [必做] 步骤 2a：FP16 点积模板。
`[I4-T14]` (main.cu:84) Kernel 2（unfused）：softmax `S → P`（支持 causal mask）。
`[I4-T15]` (main.cu:86) TODO [必做] 步骤 2b：online softmax（warp per row）+ causal mask。
`[I4-T16]` (main.cu:87) TODO [必做] 步骤 4：causal mask：`j > row` 则加 `-INFINITY`。
`[I4-T17]` (main.cu:99) TODO [必做] 步骤 2b：online softmax 模板（参考 I1）。
`[I4-T18]` (main.cu:118) Kernel 3（unfused）：P · V → O。
`[I4-T19]` (main.cu:119) `P: [N, N]  V: [N, d_v]  O: [N, d_v]`。
`[I4-T20]` (main.cu:121) TODO [必做] 步骤 2c：FP16 V，FP32 P，累加到 FP32 O。
`[I4-T21]` (main.cu:133) TODO [必做] 步骤 2c：FP32 累加后转 FP16 写出。

## Fused 单 kernel 路径

`[I4-T22]` (main.cu:142) Kernel 4：fused attention（softmax + PV 融合到单 kernel）。
`[I4-T23]` (main.cu:144) 每个 block 处理一行查询 `Q[row, :]`。
`[I4-T24]` (main.cu:145) 内层循环遍历 K/V tile，smem 中做 online softmax + 部分 PV 累加。
`[I4-T25]` (main.cu:147) TODO [必做] 步骤 3：实现 fused kernel（外层每 block 一行 Q；内层 tile K/V；in-block online softmax；修正之前部分输出）。
`[I4-T26]` (main.cu:161) 每个 block 处理一行 q。
`[I4-T27]` (main.cu:164) TODO [必做] 步骤 3：fused kernel 实现模板。
`[I4-T28]` (main.cu:187) stub：输出 0。

## 必做任务（对应 `main.cu` 中的 `// TODO [必做]` 标注）

1. **步骤 1**：构造 Q、K、V（shape: `[N, d_k]`），FP16 初始化。
2. **步骤 2**：实现 unfused 版本（三个 kernel）：
   - K1：`Q · K^T → S`（scale 乘入，FP16→FP32 累加）
   - K2：`S → softmax(S) → P`（online softmax + causal mask）
   - K3：`P · V → O`（FP32 P，FP16 V，FP32 累加，FP16 输出）
3. **步骤 3**：实现 fused kernel：softmax reduce 和 PV matmul 融合到同一 block。
4. **步骤 4**：支持 causal mask（`j > row` 则置 `-INFINITY`，softmax 输出为 0）。
5. **步骤 5**：验证 unfused vs fused 元素差异 `< 1e-3`（FP16）。
6. **步骤 6**：Nsight Compute 测量 unfused vs fused 的 global memory traffic 比值。

## 进阶任务

`[I4-T29]` (main.cu:193) TODO [进阶] block-sparse mask attention。
`[I4-T30]` (main.cu:197) TODO [进阶] FP8 attention 路径（Q/K/V FP8 量化）。
`[I4-T31]` (main.cu:201) TODO [进阶] Flash-Attention-v1 风格（多 warp block-wise softmax）。

- 实现 block-sparse mask（sparse 矩阵，某些 block 被跳过）
- 引入 FP8 路径（Tensor Core 计算）
- 写 flash-attention-v1 风格版本（多 warp block-wise softmax）

## CPU 参考

`[I4-T32]` (main.cu:205) CPU 参考：标准 attention（完全实现，FP32）。
`[I4-T33]` (main.cu:215) QK^T 步骤。
`[I4-T34]` (main.cu:226) softmax 步骤。
`[I4-T35]` (main.cu:234) `P · V` 步骤。

## 主程序

`[I4-T36]` (main.cu:253) 主程序入口（单 head，batch=1 演示）。
`[I4-T37]` (main.cu:262) 演示用 B=1, H=1 的单 head；扩展到多 head 在进阶任务。
`[I4-T38]` (main.cu:266) 打印问题规模。
`[I4-T39]` (main.cu:273) TODO [必做] 步骤 1：构造 Q、K、V（FP16）。
`[I4-T40]` (main.cu:285) CPU 参考（小规模，SEQ=1024 可能稍慢，属正常）。
`[I4-T41]` (main.cu:286) 注意：CPU ref 在 SEQ=1024 时 `O(N^2*d)` 较慢，仅做正确性验证。
`[I4-T42]` (main.cu:291) 打印 CPU 参考完成。
`[I4-T43]` (main.cu:293) GPU 分配。
`[I4-T44]` (main.cu:296) unfused 中间结果。
`[I4-T45]` (main.cu:312) Unfused 路径：K1(QK^T) + K2(softmax) + K3(PV)。
`[I4-T46]` (main.cu:321) K1：QK^T（16×16 thread tile）。
`[I4-T47]` (main.cu:326) K2：softmax（warp per row）。
`[I4-T48]` (main.cu:330) K3：PV（16×16 thread tile）。
`[I4-T49]` (main.cu:340) unfused 输出。
`[I4-T50]` (main.cu:344) Fused 路径。
`[I4-T51]` (main.cu:360) fused 输出。
`[I4-T52]` (main.cu:365) TODO [必做] 步骤 5：验证 unfused vs fused 差异 < 1e-3（FP16）。
`[I4-T53]` (main.cu:366) TODO [必做] 步骤 6：Nsight Compute 对比 global memory traffic（unfused 更大）。
`[I4-T54]` (main.cu:368) 打印验收目标。
`[I4-T55]` (main.cu:378) 收尾提示：用 ncu 对比 memory traffic。

## 验收标准

| 指标 | 目标 |
|------|------|
| unfused vs fused 输出差异 | `< 1e-3`（FP16） |
| fused global memory traffic | 相对 unfused 减少 `>= 20%` |
| causal mask 正确性 | 下三角位置 softmax 输出为 0 |
| 寄存器占用 | `< 96 per thread` |

## 关键公式

```
// attention：
out = softmax(Q · K^T / sqrt(d_k)) · V

// causal mask（unfused K2 中应用）：
if (col > row) S[row][col] = -INFINITY;

// fused kernel 关键：online softmax in smem
// Q tile 固定，K/V tile 循环加载，O 增量累积
```

## Unfused 路径的 Global Memory Traffic

unfused 在三个 kernel 之间必须经过 global memory 传递 `S[N, N]`：

- K1 写出 `S`：`N × N × 4 bytes`（FP32，SEQ=1024 → 4 MB）
- K2 读 `S` + 写 `P`：再 `2 × N × N × 4 bytes`
- K3 读 `P`：再 `N × N × 4 bytes`

fused 版本的 `S` 和 `P` 只在 smem/寄存器中存在，不写出 global memory。

## 常见坑

- unfused K2 中 softmax 前需乘以 `1/sqrt(d_k)`（K1 已完成此步骤）
- causal mask 判断：`j > row`（不是 `j >= row`，对角线不应被 mask）
- fused kernel 中多 warp 的 softmax max 需全 warp 可见（shfl 广播）
- shared memory 不足以存储 temp P 时，融合失去意义
- FP16 V 与 FP32 P 相乘前需先转 FP32

## 复盘问题

1. unfused 三 kernel 之间的同步点在哪（host 端 or CUDA Graph）？
2. fused 版本为什么能减少 global memory traffic？
3. causal mask 在 softmax 步骤如何应用（数值上）？
4. 如何通过 Nsight Compute 测量 memory traffic 字节数？
5. multi-head attention 如何分配到多个 block（一 head 一 block）？
6. FP8 quantization 在 attention 中的 scale factor 如何管理？

## 编译与运行

```bash
cmake --build build --target I4_attention_fwd_unfused_vs_fused
./exercises/I4_attention_fwd_unfused_vs_fused/I4_attention_fwd_unfused_vs_fused

# Memory traffic 对比：
ncu --metrics l1tex__t_bytes,lts__t_bytes ./I4_attention_fwd_unfused_vs_fused
```

## 参考文献

- FlashAttention-2: arXiv 2307.08691
- Flash-Attention GitHub: https://github.com/Dao-AILab/flash-attention
- cuDNN Fused Attention: https://docs.nvidia.com/deeplearning/cudnn/latest/

## 输出对照（printf / std::puts 原文）

- `[I4-T38]` (main.cu:266) 原文：`问题规模：B=%d  H=%d  N=%d  d_k=%d  d_v=%d  causal=%s` → 现：`Problem size: B=%d  H=%d  N=%d  d_k=%d  d_v=%d  causal=%s`
- `[I4-T41]` (main.cu:288) 原文：`正在计算 CPU 参考（SEQ=%d，可能需要数秒）...` → 现：`Computing CPU reference (SEQ=%d, may take a few seconds)...`
- `[I4-T42]` (main.cu:291) 原文：`CPU 参考完成。` → 现：`CPU reference done.`
- `[I4-T49]` (main.cu:340) 原文：`[unfused]  %.3f ms  max_err=%.2e  (stub: 期望 < 1e-3 实现后)` → 现：`[unfused]  %.3f ms  max_err=%.2e  (stub: expect < 1e-3 once implemented)`
- `[I4-T51]` (main.cu:360) 原文：`[fused]    %.3f ms  max_err=%.2e  (stub: 期望 < 1e-3 实现后)` → 现：`[fused]    %.3f ms  max_err=%.2e  (stub: expect < 1e-3 once implemented)`
- `[I4-T54]` (main.cu:368) 原文：`验收目标：fused 与 unfused 输出差异 < 1e-03，fused 的 global memory traffic 减少 >= 20%%` → 现：`Acceptance: fused vs unfused diff < 1e-03, fused global memory traffic reduced by >= 20%%`
- `[I4-T55]` (main.cu:378) 原文：`[I4] 完成。用 ncu --metrics l1tex__t_bytes,lts__t_bytes ./I4_attention_fwd_unfused_vs_fused 对比 memory traffic。` → 现：`[I4] done. Use ncu --metrics l1tex__t_bytes,lts__t_bytes ./I4_attention_fwd_unfused_vs_fused to compare memory traffic.`
