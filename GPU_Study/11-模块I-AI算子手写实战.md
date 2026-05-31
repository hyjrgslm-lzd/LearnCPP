# 模块 I · AI 算子手写实战

## 模块目标

这个模块只做一件事：通过手写六个核心 AI 算子，理解"高性能 = 数值稳定 + 内存高效 + 局部计算"的真实含义。你不再依赖 CUTLASS/cuDNN，而是直接对标 FlashAttention、LLaMA LayerNorm、Transformer 推理端到端的低级算子。每个算子都有"天真版"和"生产版"的对比，让你看见优化的每一步成本与收益。

## 前置知识

- 完成模块 H（GEMM 优化阶梯与 CUTLASS 基础，特别是 warp tile 和 mma.sync）
- 理解 Hopper wgmma 异步指令和 TMA 流水线（模块 G）
- 掌握 shared memory 冲突规避和 warp-level reduce（模块 D）
- 理解数值稳定性（减法的精度丢失、运行最大值的用途）

## 模块完成标准

- 能手写 online softmax（一趟）并证明数值精度不低于 two-pass safe softmax
- 能用 Welford 算法写 online LayerNorm/RMSNorm，对比二趟方差法的性能差异
- 理解 GELU 的三种近似（tanh、精确 erff、SiLU），能识别精度要求选择合适版本
- 能写 unfused 三核心 attention（QK^T → softmax → ·V），再融合 softmax 到 attention block
- 掌握 Flash-Attention-2 前向的完整思想：2D tile、on-device reduce、online softmax、局部 denorm
- 理解 FP8 E4M3 量化：per-tensor scaling、per-block scaling、dequant-fused matmul，能量化 GEMM 并验证精度漂移

## 硬件与工具链要求

- Compute Capability：sm_89+（FP8 E4M3/E5M2）；sm_90a 推荐（wgmma、TMA）
- CUDA Toolkit：13.x+
- Nsight Compute：用于观察寄存器占用、block-level reduce 吞吐
- 浮点精度检查：用 `-DFLOAT_PRECISION_CHECK=1` 编译，对比输出偏差

---

## 练习 I1：online_softmax_milakov

### 目标

学会 Milakov & Gimelshein 2018 的 online softmax：一趟计算，running max + running denominator。理解为什么这比"先找 max、再计算 exp、再求和"的两趟法快，且数值精度接近甚至更优。

### 前置理解

- softmax 的定义：`exp(x - max) / sum(exp(x - max))`
- running max 的更新：`max_new = max(max_old, x_i)`
- 第一趟扫过时 max 不是全局最大值，需要第二趟修正输出：`output_i *= exp(max_old - max_final)`
- 与两趟法的精度对比（一般数值上接近或更好）

### 必做任务

1. // TODO [必做] 构造一个 row-major matrix（shape: `N × d`），初始化为随机值，最大值跨度 1e-3 到 1e3。
2. // TODO [必做] 实现 warp-level online softmax（一条 warp 处理一行，假设 `d ≤ 32`）：
   - 初始化 `running_max = -INFINITY`、`running_sum = 0.0f`
   - 逐个 lane 读元素，用 `__shfl_sync` 广播 max，更新 running_max
   - 用 warp shuffle reduce 合并最终的 running_max 和 running_sum
3. // TODO [必做] 在 block 级扩展：多个 warp 处理多行；每行长度支持任意值（warp 内循环 + 跨 warp 合并）。
4. // TODO [必做] 实现 block-level reduce：用 shared memory 汇合多 warp 的 running_max，再做全局 max 修正。
5. // TODO [必做] 对比输出：与两趟安全 softmax 的元素差异应 < 1e-5（FP32）。
6. // TODO [必做] 用 Nsight Compute 测量：block-level reduce 的吞吐应接近 L2 读带宽。

### 进阶任务

- 实现 FP16 版本（FP32 累加器），观察精度与吞吐的权衡
- 支持 block-sparse attention mask（可选某些 block 跳过 softmax）
- 用 FP8 运行 max 和 sum（带 rescaling）

### 验收点

- online softmax 输出与 CPU 参考实现的误差 < 1e-5（FP32）
- 吞吐 ≥ GPU 峰值带宽的 40%（对于 pure memory 操作）
- 代码无寄存器溢出（Nsight Compute 检查）
- 支持任意 row length（不仅 32 的倍数）

### 观察点

- running max 初始化为 `-INFINITY` 是必要的（不能是 0 或第一个元素）
- warp shuffle reduce 的最后 step 需要 max value，在第一轮 reduce 后可用
- block-level reduce 需要在 shared memory 前先加屏障同步所有 warp
- 为什么一趟法的精度有时比两趟法更好（浮点舍入的累积误差减少）

### 常见坑

- running_max 初始化错误（-FLT_MAX vs -INFINITY，后者是特殊值）
- 忘记 step 1：从 max 中减去旧 max（修正因子 `exp(old_max - new_max)`）
- warp reduce 时，某个 warp 的 max 没被所有 lane 看到（broadcast 前忘记同步）
- block-level reduce 中 shared memory 冲突（没用 warp ID 作 offset）
- 跨 warp 的 max 合并时，没在 shared memory 上也做一次 warp reduce，导致精度丢失
- tile 循环里忘记累积结果，导致每个 tile 覆盖之前的计算
- 长行处理时，没有处理"最后不足一 warp"的 tile
- 数值检查未考虑 inf/nan（online softmax 在全 -inf 时行为特殊）

### 提示

- online softmax 的关键公式：`new_output[i] = old_output[i] * exp(old_max - new_max)`
- warp reduce max 模式：`for(int offset = 16; offset > 0; offset >>= 1) { int other_max = __shfl_down_sync(0xffffffff, running_max, offset); running_max = max(running_max, other_max); }`
- block-level 合并：用 blockIdx.x 或 threadIdx.x/blockDim.x 计算 shared memory 位置
- 验证：对比一小部分行的输出与 numpy 的 softmax(x, axis=-1)

### 复盘问题

- 为什么 running_max 初始化为 `-INFINITY` 而不是第一个元素？
- 一趟法的精度有时比两趟法更优，为什么？
- 如何在 warp reduce 时确保每个 lane 的 max 被正确合并？
- 为什么需要 block-level reduce，而不是每个 warp 独立处理？
- row length 不是 32 倍数时，如何避免 warp divergence？
- 如何通过 Nsight Compute 的 L2 hit/miss 比例验证内存访问模式？

### 对应官方参考

- arXiv 1805.02867: "Online Softmax" (Milakov & Gimelshein 2018)
- CUDA C++ Programming Guide Section 4.3: "Warp Shuffle Functions"
- CUB library: `warp_reduce` example at https://github.com/NVIDIA/cccl/tree/main/cub/cub/block
- cuDNN fused attention docs: https://docs.nvidia.com/deeplearning/cudnn/

---

## 练习 I2：layernorm_rmsnorm_welford

### 目标

学会两种 normalization：LayerNorm（传统，计算 mean 和 variance）与 RMSNorm（LLaMA 风格，省掉 mean 只用 RMS）。用 Welford one-pass 算法代替二趟法，对比精度与吞吐。理解 FP16 输入 FP32 累加的必要性。

### 前置理解

- LayerNorm：`(x - mean) / sqrt(var + eps) * gamma + beta`
- RMSNorm：`x / sqrt(mean(x^2) + eps) * gamma`（简化版没有 mean 步）
- Welford 算法：在一趟扫过中同时更新 mean 和 M2（二阶矩），最后 var = M2 / N
- FP16 input 但 FP32 accumulation 的重要性（减少累积误差）

### 必做任务

1. // TODO [必做] 构造一个 matrix（shape: `N × d`），FP16 初始化，范围 [-1, 1]。
2. // TODO [必做] 实现 warp-level Welford：
   - 初始化 `mean = 0.0f`、`M2 = 0.0f`、`count = 0`（FP32 累加器）
   - 逐个元素读取（FP16，转 FP32），用 Welford 更新公式：`delta = x - mean`、`mean += delta / (count + 1)`、`delta2 = x - mean`、`M2 += delta * delta2`
   - 用 warp shuffle reduce 合并所有 lane 的 mean 和 M2
3. // TODO [必做] 实现 block-level Welford reduce（多 warp 合并统计量）。
4. // TODO [必做] 实现 gamma/beta 的广播应用（per-feature scale 和 shift）。
5. // TODO [必做] 在同一 kernel 中实现 LayerNorm 和 RMSNorm 两个分支；用命令行参数选择。
6. // TODO [必做] 对比输出：与 PyTorch 的 nn.LayerNorm 和自定义 RMSNorm 的误差应 < 1e-3（FP16 精度下）。

### 进阶任务

- 实现 `cudaMemsetAsync` 初始化 stats buffer，避免 CPU 同步
- 支持 gamma/beta 为 FP16（在反归一化时转 FP32 临时计算）
- 写一个 fused 版本：normalization + 线性层（预备 I3 fusion）

### 验收点

- LayerNorm 输出与 PyTorch 参考的相对误差 < 1e-3（FP16）
- RMSNorm 与手写 PyTorch 版本一致
- 性能：block 级 Welford reduce 的吞吐达 GPU 峰值带宽的 30-40%
- gamma/beta 的广播不引入额外 shared memory bank conflict

### 观察点

- Welford 的关键优势：一趟扫过同时计算 mean 和 variance，避免两趟的额外 L2 traffic
- FP16 input 需要 FP32 累加器，否则 mean 会严重偏离（演示给学生看）
- RMSNorm 比 LayerNorm 少一趟减法操作（no mean），但差异在现代硬件上不明显
- block-level reduce 时，M2 需要特殊合并公式（不是简单相加）

### 常见坑

- Welford 公式里 `delta2 = x - mean_new`（不是 mean_old），写反会导致方差错误
- 在 FP16 input 但 FP32 accum 的场景下，忘记转换导致精度丢失
- block-level M2 合并时，没用正确的分布式方差合并公式
- gamma/beta 广播时，没考虑到它们可能也是 FP16（需要转 FP32 相乘）
- 多 block 情形下，每个 block 分别做 mean/var 而不是全局统计（推理 batch size 为 1 或常用？）
- 在 RMSNorm 时错误地计算了 "mean(x^2)" 而不是 "mean(x) = 0 的 case 下 E[x^2]"
- eps 值选择太小导致 sqrt 下溢，或太大破坏数值精度
- 没有处理 NaN/Inf 输入（虽然一般不会出现，但鲁棒性考虑）

### 提示

- Welford 一趟更新公式（分布式版本参考 Pebay 2008 论文）：
  ```
  n_total = n_a + n_b
  delta = b_mean - a_mean
  a_mean += delta * n_b / n_total
  M2_total = M2_a + M2_b + delta^2 * n_a * n_b / n_total
  ```
- block-level reduce 前在 shared memory 存 mean 和 M2，再用 warp reduce 合并一次
- 验证 FP16 问题：构造一个 `[1e-2, 1e-2, ..., 1, 1, ..., 1e2, 1e2, ...]` 的行，看 mean 是否偏离

### 复盘问题

- Welford 相对于"二趟法"的性能优势在哪（hint：L2 traffic vs 计算）？
- 为什么 FP16 input 需要 FP32 accumulation？构造一个反例。
- LayerNorm vs RMSNorm 在性能上有区别吗？在精度上呢？
- 分布式 Welford（跨 warp/block）的 M2 合并公式是什么？
- 如何用 `cudaMemsetAsync` 初始化 stats buffer，与主计算 kernel 并发？
- per-layer gamma/beta vs per-token 的内存访问模式如何优化？

### 对应官方参考

- NVIDIA Blog: "Normalizing Inputs in Deep Learning" (cuDNN 背景知识)
- Pebay 2008: "Formulas for Robust, One-Pass Parallel Computation of Covariance and Arbitrary-Order Central Moments"
- PyTorch LayerNorm docs: https://pytorch.org/docs/stable/generated/torch.nn.LayerNorm.html
- cuDNN GroupNorm/LayerNorm API: https://docs.nvidia.com/deeplearning/cudnn/latest/

---

## 练习 I3：gelu_silu_fused

### 目标

学会三种激活函数及其融合策略。GELU 有两个近似（tanh 和精确 erff），SiLU 只有一个公式。融合成 epilogue（配合后续的 H5 思想），理解寄存器占用与吞吐的权衡。

### 前置理解

- GELU 的精确形式：`0.5 * x * (1 + erf(x / sqrt(2)))`
- tanh-approximation：`0.5 * x * (1 + tanh(sqrt(2/pi) * (x + 0.044715 * x^3)))`（PyTorch approximate="tanh"）
- SiLU/Swish：`x * sigmoid(x) = x / (1 + exp(-x))`
- Epilogue 融合：在 GEMM 的最后一步（写出结果前），直接应用激活，避免回写到 global memory 再读

### 必做任务

1. // TODO [必做] 构造一个 vector（shape: `[1..100]`），初始化为高斯分布 N(0, 1)。
2. // TODO [必做] 实现 tanh-approx GELU：使用 `tanhf`（或自己实现 tanh 多项式近似）。
3. // TODO [必做] 实现精确 GELU：使用 `erfcf` 或 `erff`（CUDA 提供）。
4. // TODO [必做] 实现 SiLU：计算 `x / (1 + expf(-x))`。
5. // TODO [必做] 在一个 kernel 中，三个激活都作为 epilogue 应用到同一个输入（三个输出数组）。观察每个激活的吞吐和寄存器占用。
6. // TODO [必做] 对比输出：tanh-approx vs 精确 GELU 的元素差异应 < 1e-2（对 x 在 [-5, 5] 范围）。

### 进阶任务

- 用向量内建函数（如 `__expf` 的向量版本，sm_90a 上 `__nv_bfloat16 v2` 类型）加速
- 实现 fused GEMM + GELU 的 epilogue（配合模块 H 的 mma.sync 版本）
- 研究 register spill 与性能的权衡（通过改变 blockDim 观察）

### 验收点

- tanh-approx GELU vs 精确 GELU 的平均相对误差 < 1e-2
- SiLU 输出与 `torch.nn.SiLU()` 的误差 < 1e-4（FP32）
- 三个激活的 kernel 寄存器占用均 < 64 per thread（Nsight Compute 检查）
- 吞吐 ≥ GPU 峰值带宽的 50%

### 观察点

- tanh 多项式近似的项数与精度的权衡（PyTorch 用 5 项）
- `erfcf` vs `erff` 在极值处的数值稳定性
- epilogue 融合减少内存往返的吞吐收益（对比分离的 kernel）
- 寄存器数量与 occupancy 的反向关系（GELU 计算多 → 寄存器多 → occupancy 低）

### 常见坑

- GELU tanh-approx 与 PyTorch 的 approximate="tanh" 必须用相同系数（0.044715）
- 忘记 `erfcf` 返回的是互补误差函数（1 - erf），公式需调整
- 精确 GELU 在 x 很大或很小时 erf 接近 ±1，导致浮点下溢/溢出
- SiLU 的 sigmoid 实现用 `1 / (1 + expf(-x))` 容易在 x 很大时下溢（用 `1 - 1/(1 + expf(x))` 更稳定）
- epilogue 融合时，shared memory 可能不足（如果 GEMM tile 已用满），需要改 tile 大小
- 没有考虑到三个激活函数的指令级并行性（寄存器数量会增加）
- 在 FP16 输入上，忘记转 FP32 计算激活再转回
- 对于极值输入（e.g., x = -1000），需要特殊处理避免 exp 溢出

### 提示

- PyTorch GELU tanh-approx 系数：`sqrt(2/pi) ≈ 0.7978845608f`，`0.044715f`
- 稳定的 sigmoid：`sigmoid(x) = (x >= 0) ? 1 / (1 + expf(-x)) : expf(x) / (1 + expf(x))`
- Nsight Compute 检查寄存器：找 "Registers per thread" 指标
- 验证：对比一个小向量的输出与 PyTorch

### 复盘问题

- tanh-approx 和精确 GELU 的精度差异主要来自哪里？
- 为什么 SiLU 的 sigmoid 实现需要分情况讨论？
- epilogue 融合如何减少全局内存流量？
- 三个激活的寄存器占用分别是多少，为什么会不同？
- 如何在不增加寄存器溢出的前提下融合激活到 GEMM？
- 在 sm_90a 上，向量内建函数相对标量的加速比是多少？

### 对应官方参考

- CUDA C++ Programming Guide Section 4.4: "Mathematical Functions"
- PyTorch activation functions: https://pytorch.org/docs/stable/nn.functional.html
- NVIDIA Blog: "Fast and Accurate Approximations of GELU"
- cuDNN activation API: https://docs.nvidia.com/deeplearning/cudnn/latest/

---

## 练习 I4：attention_fwd_unfused_vs_fused

### 目标

对标 Flash-Attention-2，但从"三个独立 kernel"开始（unfused），逐步融合 softmax 到 attention block。理解为什么融合减少内存访问，以及融合的代价（寄存器压力、shared memory 竞争）。

### 前置理解

- 标准 attention：`out = softmax(Q · K^T / sqrt(d_k)) · V`
- unfused 版本：三个 kernel（QK^T → softmax → ·V）
- Flash-Attention-2 思想：tile 划分、online softmax、局部 reduce，减少 L2/global 往返
- block-sparse mask：可选，某些 (i, j) block 对被跳过

### 必做任务

1. // TODO [必做] 构造 Q、K、V（shape: `[seq_len, d_k]` 或 batch 版本），FP16 初始化。
2. // TODO [必做] 实现 unfused 版本（三个 kernel）：
   - K1：Q · K^T → S（shape: `[seq_len, seq_len]`）
   - K2：S → softmax(S) → P（online softmax，shape: `[seq_len, seq_len]`）
   - K3：P · V → O（shape: `[seq_len, d_v]`）
3. // TODO [必做] 实现 fused 版本：把 softmax 的 reduce 和 P · V 的 matmul 融合到同一 block。
4. // TODO [必做] 支持 causal mask（下三角，`mask[i, j] = -inf for j > i`）。
5. // TODO [必做] 对比输出：unfused vs fused 的元素差异应 < 1e-3（FP16 精度）。
6. // TODO [必做] 用 Nsight Compute 测量：unfused 的 global memory traffic vs fused 的比值（预期 unfused 更大）。

### 进阶任务

- 实现 block-sparse mask（mask 可以是一个稀疏矩阵，某些 block 被跳过）
- 引入 FP8 路径（Tensor Core 计算）
- 写一个 fused flash-attention-v1 风格的版本（多个 warp block-wise softmax）

### 验收点

- unfused 和 fused 的输出差异 < 1e-3（FP16）
- fused 版本的 global memory traffic 相对 unfused 减少 ≥ 20%
- causal mask 正确应用（下三角 -inf 处的 softmax 输出为 0）
- 寄存器占用在合理范围（< 96 per thread）

### 观察点

- unfused 版本的三个 kernel 之间需要 global synchronization（host 端或 CUDA Graph）
- fused 版本在 shared memory 中做 online softmax，避免写回 P 到 global
- causal mask 下，对于 j > i 的位置，softmax 输入为 -inf，输出为 0，其贡献应被忽略
- 为什么 block-level softmax 和 matmul 能融合（它们共用同一个 tile）

### 常见坑

- unfused 版本的 softmax kernel 前要 scale by `1/sqrt(d_k)`（attention 公式的一部分）
- causal mask 实现错误（e.g., 用 > 而不是 >=，导致对角线被 mask）
- fused 版本里，多 warp 的 softmax max 没被所有 lane 看到（divergence）
- shared memory 不足以存储 temp P（fused 版本），导致溢出到 global（失去融合好处）
- FP8 输入时，没有正确处理 scale factor（Q · K^T 后要乘 scale）
- 没有考虑 multi-head attention 的批处理（hint：可以让不同 head 用不同 block）

### 提示

- scale by `1/sqrt(d_k)` 可在 QK^T matmul 时一步完成，或在 softmax 前单独乘
- causal mask 判断：`if (col > row) then set to -INFINITY`
- fused softmax：使用 I1 中的 online softmax 代码，在 block-level 合并
- 验证：对比小序列长度（e.g., 64）的输出与 PyTorch 的 `torch.nn.MultiheadAttention` 或手写 softmax 版本

### 复盘问题

- unfused 版本的三个 kernel 之间的同步点在哪？
- fused 版本为什么能减少 global memory traffic？
- causal mask 在 softmax 步骤如何应用（数值上）？
- 如何通过 Nsight Compute 测量 memory traffic（字节数）？
- multi-head attention 如何分配到多个 block（假设一个 head 一个 block）？
- FP8 quantization 在 attention 中的 scale factor 如何管理？

### 对应官方参考

- Flash-Attention-2 GitHub: https://github.com/Dao-AILab/flash-attention
- FlashAttention-2: Faster Attention with Better Parallelism and Work Partitioning (arXiv 2307.08691)
- cuDNN Fused Attention: https://docs.nvidia.com/deeplearning/cudnn/latest/
- NVIDIA Blog: "Fast Transformer Inference with NVIDIA Triton and A100 GPUs"

---

## 练习 I5：flash_attention_v2_style

### 目标

完整实现 Flash-Attention-2 前向：2D tile 划分（Br × Bc）、outer loop 遍历 Q 的 blocks、inner loop 遍历 K/V 的 blocks、on-device online softmax、局部归一化。对标 cuDNN fused attention，理解现代 attention 的 IO 最优性。可选进阶：Hopper TMA 加载 tile。

### 前置理解

- FA-2 的核心：按 tile 把 Q、K、V 分解，避免装载整个 S 矩阵到 SRAM
- outer loop：按 Br（e.g., 128）划分 Q，每个 block 处理一个 Q tile
- inner loop：按 Bc（e.g., 64）划分 K/V，逐步更新 partial attention
- online softmax：每读入一个新的 K block，更新 max 和 sum，同时修正之前的输出
- 非 causal 与 causal 两版：causal 下需要提前 exit

### 必做任务

1. // TODO [必做] 构造 Q、K、V（shape: `[N, d]`），FP16 初始化。
2. // TODO [必做] 实现 block 划分逻辑：定义 Br 和 Bc，outer loop 和 inner loop。
3. // TODO [必做] 在 inner loop 中融合 online softmax：
   - 读 K block、计算 S_block = Q_block · K_block^T / sqrt(d_k)
   - 更新 running_max 和 running_sum（I1 的 online softmax 逻辑）
   - 用新的 max 修正之前的 P_block 输出
4. // TODO [必做] 实现 matmul 融合：P_block · V_block，累积到全局 O
5. // TODO [必做] 实现 causal 版本：在 inner loop 检查 j < i（block 级别的 causal mask）
6. // TODO [必做] 对比输出：FA-2 vs unfused 三 kernel 版本的差异应 < 1e-3（FP16）；与官方 cuDNN fused attention 对齐（若可用）。

### 进阶任务

- 引入 FP16 输入 FP32 累加
- 实现 Hopper TMA 加载 Q、K、V tile（对接模块 G4）
- 支持 multi-query attention（MQA）或 grouped query attention（GQA）

### 验收点

- FA-2 输出与 unfused 版本的差异 < 1e-3（FP16）
- IO 字节数：FA-2 应相对 unfused 三 kernel 减少（理想情况 50% 以上）
- causal mask 正确性：下三角矩阵计算无误
- 寄存器占用 < 120 per thread（TMA 版本可能更高）

### 观察点

- outer loop 和 inner loop 的粒度选择（Br = 128, Bc = 64 是常见值）
- online softmax 在每个 inner loop iteration 都要修正之前的 P（公式复杂）
- TMA 异步加载如何与 online softmax 流水线化（producer-consumer warp 分工）
- 为什么 FA-2 比 unfused 快：shared memory 中完成大部分计算，减少 L2/global 往返

### 常见坑

- online softmax 修正公式写错（应是 `old_P[i] *= exp(old_max - new_max)`）
- inner loop 中忘记累积 O 结果，导致只有最后一个 block 的贡献
- causal mask 检查应在 block 级别或元素级别（取决于 tile 大小），逻辑复杂易错
- FP16 输入时，累加器精度丢失（一定要用 FP32）
- 跨 block 的 softmax 修正需要 shared memory 同步，但代码没加屏障
- TMA 版本中，tile 大小与 TMA 硬件限制不匹配（e.g., 16 的倍数）
- 没有处理 Q/K/V 长度不是 Br/Bc 倍数的边界情况

### 提示

- Br 和 Bc 选择通常基于 shared memory 大小：`2 * Br * d * sizeof(float) + 2 * Bc * d * sizeof(float) < smem_size`
- online softmax 每轮 iteration 需要 `exp(old_max - new_max)` 修正因子
- causal mask block 级判断：`if (block_j > block_i) skip block` 或 element 级判断
- 验证：对比中等序列长度（e.g., 1024 或 2048）与 PyTorch 官方 attention 或 xformers FlashAttention
- Nsight Compute 查看 shared memory bank conflict 和 cache hit 率

### 复盘问题

- FA-2 的 outer loop 和 inner loop 分别遍历 Q 和 K 的哪些维度？
- online softmax 在每轮 inner loop 后如何修正之前的 P 输出？
- causal attention 在 FA-2 框架内如何实现（block 级还是元素级判断）？
- IO 复杂度相对 unfused 的改进来自哪里（hint：smem 重用）？
- Hopper TMA 如何与 FA-2 的 producer-consumer warp 流水线结合？
- 如何用 Nsight Compute 验证 shared memory 的有效利用率？

### 对应官方参考

- Flash-Attention-2 Paper: arXiv 2307.08691
- Flash-Attention GitHub: https://github.com/Dao-AILab/flash-attention
- CUTLASS 3.x FlashAttention example: https://github.com/NVIDIA/cutlass/tree/main/examples
- cuDNN Fused Attention docs: https://docs.nvidia.com/deeplearning/cudnn/latest/
- NVIDIA Blog: "Scaling Transformers with Efficient Attention"

---

## 练习 I6：fp8_gemm_with_scaling

### 目标

学会 FP8 E4M3 量化在 GEMM 中的应用：per-tensor scaling、per-block scaling、dequant-fused matmul。理解精度漂移与 scale 管理的关系。对标 TransformerEngine 的 FP8 策略，但从基础量化开始。

### 前置理解

- FP8 E4M3：4 exponent bits、3 mantissa bits，范围 ±240、精度 1/8（相对 1/128 的 FP16）
- per-tensor scaling：整个矩阵共用一个 scale（简单，但精度损失大）
- per-block scaling：每个 tile 一个 scale（更精确，但需要额外存储和管理）
- dequant-fused matmul：在 mainloop 中直接反量化和累加，避免额外内存访问

### 必做任务

1. // TODO [必做] 构造 FP32 矩阵 A、B（shape: `[M, K]` 和 `[K, N]`），初始化为高斯分布。
2. // TODO [必做] 实现 per-tensor FP8 量化：
   - 计算全局 scale：`scale = max(abs(A)) / 240.0f`（FP8 E4M3 的最大值）
   - 量化：`A_fp8 = A / scale`（截断到 FP8 范围）
   - 反量化验证：`A_recovered = A_fp8 * scale`
3. // TODO [必做] 实现 per-block 量化（假设 block 大小 64×64）：
   - 每个 block 独立计算 scale
   - 存储 scale metadata（shape: `[ceil(M/64), ceil(K/64)]`）
4. // TODO [必做] 实现 dequant-fused GEMM：
   - 在 mma.sync（或普通 gemm 循环）的 mainloop 中，读 FP8、反量化到 FP32、累加
   - 避免写出中间的反量化矩阵
5. // TODO [必做] 对比输出：per-tensor FP8 GEMM vs FP32 baseline 的相对误差；per-block FP8 的误差应更小。
6. // TODO [必做] 用 Nsight Compute 观察：dequant-fused 相对分离版本的寄存器占用和吞吐。

### 进阶任务

- 实现 delayed scaling（TransformerEngine 风格）：积累多步的 scale amax，周期更新 scale
- 支持非对称量化（考虑 zero point）
- 实现 FP8 input + FP8 weight 的完整推理路径

### 验收点

- per-tensor FP8 GEMM 相对 FP32 baseline 的相对误差 ≥ 1e-2（可接受范围）
- per-block FP8 的误差应 < 1e-3，明显优于 per-tensor
- dequant-fused 版本的吞吐相对分离版本无明显下降（可能因寄存器增加略降）
- 支持任意 M、K、N 大小（不仅 64 倍数）

### 观察点

- FP8 E4M3 相对 FP16 的精度损失（演示在神经网络权重分布上的影响）
- per-block scaling 对存储空间的额外开销（metadata size）
- dequant-fused 在 mainloop 中引入额外乘法操作，对寄存器压力的影响
- delayed scaling 与 just-in-time scaling 的性能差异（TE 的优化点）

### 常见坑

- FP8 E4M3 的最大值是 240，不是 255（易与 uint8 混淆）
- 量化时 scale 计算错误（应是 `max_abs / 240`，不是 `max_abs`）
- 反量化忘记乘以 scale（只转 FP32 但没有乘）
- per-block metadata 的索引计算错误（block row/col 与矩阵行列的映射）
- dequant-fused 中，FP8 转 FP32 的方式不对（直接 cast vs 显式乘以 scale）
- per-block scale 存储和读取的 global memory 同步问题（没加足够的屏障）
- 没有考虑量化后的数值分布（e.g., 某些权重接近 ±240，截断会丢失）
- 累加器精度丢失：FP32 累加过程中，如果多次加小数，会丢失精度

### 提示

- FP8 E4M3 范围：[-240, 240]，可用 `clamp(value / scale, -240, 240)` 量化
- per-tensor scale 存储为单个 FP32；per-block scale 存储为 2D 数组
- dequant-fused：在 mainloop 的 ldmatrix 后立即乘以 scale，再做 mma.sync
- 验证：小矩阵（e.g., 64×64）对比手写 FP32 GEMM 和 FP8 GEMM 的输出

### 复盘问题

- per-tensor vs per-block scaling 的精度-性能权衡是什么？
- FP8 E4M3 的最大值为什么是 240？
- dequant-fused GEMM 如何避免写出反量化矩阵？
- per-block scale metadata 的存储开销与精度改进的权衡（多大的 block 最优）？
- delayed scaling 如何工作，相对 just-in-time 的好处是什么？
- 如何通过 Nsight Compute 测量 dequant-fused 的寄存器占用和吞吐？

### 对应官方参考

- CUDA FP8 Datatypes: https://docs.nvidia.com/cuda/cuda-c-programming-guide/
- FP8 in Transformer Engine: https://github.com/NVIDIA/TransformerEngine
- NVIDIA Blog: "FP8 in Transformer Engine: Delivering Faster and Smaller AI Models"
- TransformerEngine delayed scaling: https://docs.nvidia.com/deeplearning/transformer-engine/user-guide/
- cuBLASLt FP8 GEMM: https://docs.nvidia.com/cuda/cublas/

---

## 做完本模块后应达到的水平

- 能独立手写 online softmax，理解数值稳定性与内存访问的权衡
- 掌握 Welford one-pass 算法，能在 LayerNorm/RMSNorm 中应用
- 了解激活函数的三种形式，能判断何时融合到 epilogue
- 理解 unfused vs fused attention 的内存成本差异，能用 Nsight Compute 量化
- 能复现 Flash-Attention-2 的核心思想，理解 2D tile 和 online softmax 的作用
- 掌握 FP8 量化的两种策略（per-tensor 和 per-block），能在 GEMM 中应用
- 会用 Nsight Compute 检查寄存器占用、shared memory 冲突、memory traffic 字节数
- 能用 Python 脚本对比输出与 PyTorch 参考实现的精度
