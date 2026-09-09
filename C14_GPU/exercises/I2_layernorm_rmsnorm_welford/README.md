# I2 — LayerNorm / RMSNorm with Welford One-pass

## 目标

`[I2-T01]` (main.cu:2) 练习 I2：LayerNorm / RMSNorm — Welford one-pass 算法。
`[I2-T02]` (main.cu:4) 学习目标：单趟同时算 mean + M2，分布式 Welford 跨 warp/block 合并，FP16 输入 + FP32 累加，LayerNorm vs RMSNorm 切换。

学会两种 normalization：LayerNorm（传统，计算 mean 和 variance）与 RMSNorm（LLaMA 风格，省掉 mean 只用 RMS）。用 Welford one-pass 算法代替二趟法，对比精度与吞吐。理解 FP16 输入 FP32 累加的必要性。

## 问题规模

`[I2-T03]` (main.cu:32) 问题规模常量。
`[I2-T04]` (main.cu:33) batch（行数）。
`[I2-T05]` (main.cu:34) hidden dim（每行长度）。
`[I2-T06]` (main.cu:36) `blockDim.x = 256 = 8 warps`。

| 参数 | 值 |
|------|----|
| Batch（行数）B | 32 |
| Hidden dim H | 4096 |
| 输入数据类型 | FP16（`__half`） |
| 累加器 | FP32 |
| gamma / beta | FP32 per-feature |

## 前置知识

- LayerNorm：`(x - mean) / sqrt(var + eps) * gamma + beta`
- RMSNorm：`x / sqrt(mean(x^2) + eps) * gamma`（无 mean 步）
- Welford 算法：单趟同时更新 mean 和 M2（二阶矩），最后 `var = M2 / N`
- FP16 input + FP32 accumulation 的必要性

## 设备工具

`[I2-T07]` (main.cu:41) 设备工具：warp reduce sum（FP32）。

## Naive LayerNorm 基线 kernel

`[I2-T08]` (main.cu:50) Kernel 1：naive LayerNorm（两趟：先算 mean，再算 var）。
`[I2-T09]` (main.cu:51) 对照组，用于验证 Welford 版本。
`[I2-T10]` (main.cu:58) 一个 block 处理一行（`blockDim.x = 256`）。
`[I2-T11]` (main.cu:65) 每个 warp 的局部和。
`[I2-T12]` (main.cu:71) 第一趟：求 mean（FP32 累加）。
`[I2-T13]` (main.cu:84) 广播到所有线程（通过 smem）。
`[I2-T14]` (main.cu:91) 第二趟：求 var。
`[I2-T15]` (main.cu:112) 写出：`(x - mean) * inv_std * gamma + beta`。

## Welford one-pass 待实现 kernel

`[I2-T16]` (main.cu:118) Kernel 2：online Welford LayerNorm / RMSNorm（单趟）。
`[I2-T17]` (main.cu:122) TODO [必做] 步骤 2：warp-level Welford。初始化 `mean=0, M2=0, count=0`（FP32 累加器）；逐元素：`delta = x - mean; mean += delta/(count+1); delta2 = x - mean; M2 += delta*delta2`。
`[I2-T18]` (main.cu:125) TODO [必做] 步骤 3：block-level Welford reduce（分布式 Pebay 公式跨 warp 合并）。
`[I2-T19]` (main.cu:126) TODO [必做] 步骤 4：gamma/beta 广播应用（per-feature scale + shift）。
`[I2-T20]` (main.cu:127) TODO [必做] 步骤 5：`use_rmsnorm` 分支（省掉 mean，只用 `RMS = sqrt(mean(x^2))`）。
`[I2-T21]` (main.cu:143) TODO [必做] 步骤 2 的 Welford 模板（注意 `delta2 = x - new_mean`）。
`[I2-T22]` (main.cu:155) TODO [必做] 步骤 3 跨 warp 合并模板（Pebay 2008 分布式方差公式）。
`[I2-T23]` (main.cu:161) TODO [必做] 步骤 4/5：归一化写出模板。
`[I2-T24]` (main.cu:169) stub：输出 0。

## 必做任务（对应 `main.cu` 中的 `// TODO [必做]` 标注）

1. **步骤 1**：构造 matrix（shape: `B × H`），FP16 初始化，范围 `[-1, 1]`。
2. **步骤 2**：实现 warp-level Welford：
   - 初始化 `mean=0, M2=0, count=0`（FP32 累加器）
   - 逐元素（FP16 转 FP32）用 Welford 公式更新
3. **步骤 3**：block-level Welford reduce（Pebay 2008 分布式方差合并）。
4. **步骤 4**：gamma/beta 广播应用（per-feature scale 和 shift）。
5. **步骤 5**：`use_rmsnorm` 分支：省掉 mean，只用 `mean(x^2)`。
6. **步骤 6**：对比输出与 CPU LayerNorm/RMSNorm 参考，误差 `< 1e-3`（FP16 精度）。

## 进阶任务

`[I2-T25]` (main.cu:175) TODO [进阶] `cudaMemsetAsync` 初始化 stats buffer，避免 CPU 同步。
`[I2-T26]` (main.cu:179) TODO [进阶] gamma/beta FP16 支持（反归一化时转 FP32 临时计算）。
`[I2-T27]` (main.cu:183) TODO [进阶] fused normalization + 线性层（预备 I3 fusion）。

- 实现 `cudaMemsetAsync` 初始化 stats buffer，避免 CPU 同步
- 支持 gamma/beta 为 FP16（反归一化时转 FP32 临时计算）
- 写 fused 版本：normalization + 线性层（预备 I3 fusion）

## CPU 参考与验证

`[I2-T28]` (main.cu:187) CPU 参考：LayerNorm（完全实现）。
`[I2-T29]` (main.cu:213) CPU 参考：RMSNorm（完全实现）。

## 主程序

`[I2-T30]` (main.cu:243) 主程序入口。
`[I2-T31]` (main.cu:251) TODO [必做] 步骤 1：构造 FP16 矩阵，范围 `[-1, 1]`。
`[I2-T32]` (main.cu:252) 这里已实现初始化。
`[I2-T33]` (main.cu:256) FP32 for CPU ref，FP16 for GPU。
`[I2-T34]` (main.cu:263) gamma 随机 `[0.5, 1.5]`，beta 随机 `[-0.1, 0.1]`。
`[I2-T35]` (main.cu:269) 打印问题规模。
`[I2-T36]` (main.cu:271) CPU 参考。
`[I2-T37]` (main.cu:276) GPU 分配。
`[I2-T38]` (main.cu:288) 一个 block 处理一行，`blockDim.x = NUM_WARPS * WARP_SIZE = 256`。
`[I2-T39]` (main.cu:290) 打印启动配置。
`[I2-T40]` (main.cu:295) Naive LayerNorm（对照组）。
`[I2-T41]` (main.cu:310) naive_ln 输出。
`[I2-T42]` (main.cu:313) Welford LayerNorm（待实现）。
`[I2-T43]` (main.cu:328) welford_ln 输出。
`[I2-T44]` (main.cu:332) Welford RMSNorm（待实现）。
`[I2-T45]` (main.cu:346) welford_rms 输出。
`[I2-T46]` (main.cu:351) TODO [必做] 步骤 6：对比 Welford vs naive 的精度与吞吐。
`[I2-T47]` (main.cu:352) TODO [必做] 步骤 6：Nsight Compute 测量 block 级 Welford reduce 吞吐。
`[I2-T48]` (main.cu:354) 打印验收目标。
`[I2-T49]` (main.cu:355) 打印当前 stub 行为说明。
`[I2-T50]` (main.cu:362) 收尾提示：用 ncu 分析。

## 验收标准

| 指标 | 目标 |
|------|------|
| LayerNorm 误差 vs CPU ref | `< 1e-3`（FP16） |
| RMSNorm 误差 vs CPU ref | `< 1e-3`（FP16） |
| 吞吐 | `>= GPU 峰值带宽的 30-40%` |
| gamma/beta bank conflict | 无 |

## 关键公式

```
// Welford 单元素更新：
delta  = x - mean;
mean  += delta / (++count);
delta2 = x - mean;          // 注意：用 new mean！
M2    += delta * delta2;
var    = M2 / count;

// 分布式 Welford（跨 warp/block 合并，Pebay 2008）：
n_total = n_a + n_b
delta   = b_mean - a_mean
mean_c  = a_mean + delta * n_b / n_total
M2_c    = M2_a + M2_b + delta^2 * n_a * n_b / n_total
```

## 常见坑

- Welford 公式里 `delta2 = x - mean_new`（不是 mean_old），写反会导致方差错误
- FP16 input + FP32 accum 场景下，忘记 `__half2float()` 转换
- block-level M2 合并时，没用正确的分布式方差合并公式（简单相加是错的）
- gamma/beta 可能是 FP16，需转 FP32 相乘
- eps 值选太小（下溢）或太大（破坏精度）

## 观察点

- Welford 的核心优势：单趟扫描，避免两趟 L2 traffic
- FP16 直接累加 mean 的精度问题（构造 `[1e-2, ..., 1e2]` 行演示）
- RMSNorm 比 LayerNorm 少一趟减法，差异在现代硬件上有限

## 复盘问题

1. Welford 相对"二趟法"的性能优势在哪（L2 traffic vs 计算）？
2. 为什么 FP16 input 需要 FP32 accumulation？构造一个反例。
3. LayerNorm vs RMSNorm 在性能上有区别吗？精度上呢？
4. 分布式 Welford（跨 warp/block）的 M2 合并公式是什么？
5. 如何用 `cudaMemsetAsync` 与主计算 kernel 并发初始化 stats buffer？
6. per-layer gamma/beta vs per-token 的内存访问模式如何优化？

## 编译与运行

```bash
cmake --build build --target I2_layernorm_rmsnorm_welford
./exercises/I2_layernorm_rmsnorm_welford/I2_layernorm_rmsnorm_welford

# Nsight Compute 分析：
ncu --set full ./I2_layernorm_rmsnorm_welford
```

## 参考文献

- Pebay 2008: "Formulas for Robust, One-Pass Parallel Computation of Covariance and Arbitrary-Order Central Moments"
- NVIDIA Blog: "Normalizing Inputs in Deep Learning"
- PyTorch LayerNorm: https://pytorch.org/docs/stable/generated/torch.nn.LayerNorm.html

## 输出对照（printf / std::puts 原文）

- `[I2-T35]` (main.cu:269) 原文：`问题规模：B=%d  H=%d  total_floats=%d` → 现：`Problem size: B=%d  H=%d  total_floats=%d`
- `[I2-T39]` (main.cu:290) 原文：`启动配置：grid=(%d,1,1)  block=(%d,1,1)` → 现：`Launch config: grid=(%d,1,1)  block=(%d,1,1)`
- `[I2-T41]` (main.cu:310) 原文：`[naive_ln]    %.3f ms  max_err=%.2e  有效带宽=%.1f GB/s` → 现：`[naive_ln]    %.3f ms  max_err=%.2e  effective_bw=%.1f GB/s`
- `[I2-T43]` (main.cu:328) 原文：`[welford_ln]  %.3f ms  max_err=%.2e  有效带宽=%.1f GB/s  (stub: 期望 err < 1e-3 实现后)` → 现：`[welford_ln]  %.3f ms  max_err=%.2e  effective_bw=%.1f GB/s  (stub: expect err < 1e-3 once implemented)`
- `[I2-T45]` (main.cu:346) 原文：`[welford_rms] %.3f ms  max_err=%.2e  (stub: 期望 err < 1e-3 实现后)` → 现：`[welford_rms] %.3f ms  max_err=%.2e  (stub: expect err < 1e-3 once implemented)`
- `[I2-T48]` (main.cu:354) 原文：`验收目标：Welford LayerNorm max_err < 1e-03（FP16 精度）` → 现：`Acceptance: Welford LayerNorm max_err < 1e-03 (FP16 precision)`
- `[I2-T49]` (main.cu:355) 原文：`当前 stub 输出全零 → CPU check 会报 mismatch，这是预期行为。` → 现：`Current stub outputs all zeros; CPU check will report mismatch (expected).`
- `[I2-T50]` (main.cu:362) 原文：`[I2] 完成。用 ncu --set full ./I2_layernorm_rmsnorm_welford 分析。` → 现：`[I2] done. Use ncu --set full ./I2_layernorm_rmsnorm_welford to analyze.`
