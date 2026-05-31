# I3 — GELU / SiLU 激活函数与 Epilogue 融合

## 目标

`[I3-T01]` (main.cu:2) 练习 I3：GELU / SiLU 激活函数与 Epilogue 融合。
`[I3-T02]` (main.cu:4) 学习目标：tanh-approx GELU、精确 GELU、SiLU、融合 epilogue 与寄存器/occupancy 权衡。

学会三种激活函数及其融合策略：
- tanh-approx GELU（匹配 PyTorch `approximate='tanh'`）
- 精确 GELU（`erff`）
- SiLU/Swish（`x * sigmoid(x)`）

理解 epilogue 融合的带宽收益与寄存器压力的权衡。

## 问题规模

`[I3-T03]` (main.cu:32) 问题规模常量。
`[I3-T04]` (main.cu:33) 元素数（约 1M，覆盖 `[1..100]` tiles）。

| 参数 | 值 |
|------|----|
| 元素数 N | 1M（`1 << 20`） |
| 数据类型 | FP32 |
| 输入分布 | 高斯 N(0, 1) |

## 前置知识

- GELU 精确形式：`0.5 * x * (1 + erf(x / sqrt(2)))`
- tanh 近似：`0.5 * x * (1 + tanh(sqrt(2/pi) * (x + 0.044715 * x^3)))`
- SiLU/Swish：`x * sigmoid(x) = x / (1 + exp(-x))`
- Epilogue 融合：在 GEMM 最后一步写出前直接应用激活，减少全局内存往返

## tanh-approx GELU 系数

`[I3-T05]` (main.cu:39) tanh-approx GELU 系数（PyTorch `approximate='tanh'`）：`0.5 * x * (1 + tanh(sqrt(2/pi) * (x + 0.044715 * x^3)))`。

## 设备激活函数

`[I3-T06]` (main.cu:46) 设备函数：tanh-approx GELU。
`[I3-T07]` (main.cu:48) TODO [必做] 步骤 2：实现 tanh-approx GELU。
`[I3-T08]` (main.cu:55) 设备函数：精确 GELU（`erff`）。
`[I3-T09]` (main.cu:57) TODO [必做] 步骤 3：实现精确 GELU。`erff` 返回 `erf(x)`；`erfcf` 返回 `1 - erf(x)`，注意区分。
`[I3-T10]` (main.cu:67) 设备函数：SiLU（数值稳定版本）。
`[I3-T11]` (main.cu:69) TODO [必做] 步骤 4：实现 `SiLU = x * sigmoid(x)`。数值稳定 sigmoid：正负分支分开，避免 `exp` 溢出。

## 独立 kernel

`[I3-T12]` (main.cu:80) Kernel 1：独立的 tanh GELU kernel。
`[I3-T13]` (main.cu:91) Kernel 2：独立的精确 GELU kernel。
`[I3-T14]` (main.cu:101) Kernel 3：独立的 SiLU kernel。

## 融合 epilogue kernel

`[I3-T15]` (main.cu:112) Kernel 4：融合 epilogue kernel。
`[I3-T16]` (main.cu:113) 模拟 GEMM + bias-add 后直接应用三种激活（三个输出数组）。
`[I3-T17]` (main.cu:115) TODO [必做] 步骤 5：在一个 kernel 中对同一输入应用三种激活（三个输出数组）。
`[I3-T18]` (main.cu:116) 观察：每个激活的寄存器占用（用 Nsight Compute 检查）。
`[I3-T19]` (main.cu:128) TODO [必做] 步骤 5：读一次 `in[i]`，写出三个激活结果。
`[I3-T20]` (main.cu:134) stub：输出 0。

## 必做任务（对应 `main.cu` 中的 `// TODO [必做]` 标注）

1. **步骤 1**：构造高斯分布 N(0, 1) 输入向量。
2. **步骤 2**：实现 tanh-approx GELU（`tanhf`，PyTorch 相同系数）。
3. **步骤 3**：实现精确 GELU（`erff`，注意 `erfcf` 是互补误差函数）。
4. **步骤 4**：实现 SiLU（数值稳定 sigmoid，正负分支分开）。
5. **步骤 5**：融合 epilogue kernel：对同一输入同时输出三种激活结果（三个输出数组）。
6. **步骤 6**：tanh-approx vs 精确 GELU 的元素差异应 `< 1e-2`（`[-5, 5]` 区间）。

## 进阶任务

`[I3-T21]` (main.cu:140) TODO [进阶] BF16 向量内建函数版本（sm_90a `__nv_bfloat162`）。
`[I3-T22]` (main.cu:144) TODO [进阶] fused GEMM + GELU epilogue（配合模块 H mma.sync）。
`[I3-T23]` (main.cu:148) TODO [进阶] 通过改变 blockDim 观察 register spill 与性能的权衡。

- 用 BF16 向量内建函数（sm_90a `__nv_bfloat162`）加速
- 实现 fused GEMM + GELU epilogue（配合模块 H mma.sync）
- 研究不同 blockDim 下 register spill 与性能的权衡

## CPU 参考

`[I3-T24]` (main.cu:152) CPU 参考（完全实现）。

## 主程序

`[I3-T25]` (main.cu:178) 主程序入口。
`[I3-T26]` (main.cu:185) TODO [必做] 步骤 1：构造高斯分布 N(0, 1) 输入向量。
`[I3-T27]` (main.cu:186) 这里已实现初始化。
`[I3-T28]` (main.cu:194) 打印问题规模。
`[I3-T29]` (main.cu:196) CPU 参考计算。
`[I3-T30]` (main.cu:204) GPU 分配。
`[I3-T31]` (main.cu:213) 打印启动配置。
`[I3-T32]` (main.cu:223) report lambda 输出格式。
`[I3-T33]` (main.cu:227) 独立 kernel 测试。
`[I3-T34]` (main.cu:259) 融合 epilogue kernel。
`[I3-T35]` (main.cu:271) 融合 epilogue 计时输出。
`[I3-T36]` (main.cu:275) TODO [必做] 步骤 6：tanh-approx vs 精确 GELU 元素差应 < 1e-2（`[-5, 5]` 区间）。
`[I3-T37]` (main.cu:278) 在 `[-5, 5]` 区间内比较两种 GELU。
`[I3-T38]` (main.cu:285) 打印 tanh-approx vs 精确 GELU 差异。
`[I3-T39]` (main.cu:289) 打印验收目标提示。
`[I3-T40]` (main.cu:296) 收尾提示：用 ncu 观察寄存器占用。

## 验收标准

| 指标 | 目标 |
|------|------|
| tanh-approx GELU vs 精确 GELU | 平均相对误差 `< 1e-2` |
| SiLU vs CPU ref | `< 1e-4`（FP32） |
| 每个激活的寄存器 | `< 64 per thread`（Nsight Compute）|
| 吞吐 | `>= GPU 峰值带宽的 50%` |

## 关键公式

```c
// tanh-approx GELU (PyTorch approximate='tanh'):
// sqrt(2/pi) ~ 0.7978845608f
// 0.5 * x * (1 + tanh(0.7978845608f * (x + 0.044715f * x^3)))

// 精确 GELU (erff):
// 0.5 * x * (1 + erff(x * 0.7071067811865476f))
// 注意：erff(x)=erf(x)；erfcf(x)=1-erf(x)（不同！）

// 数值稳定 sigmoid (SiLU 用):
// sig(x) = (x >= 0) ? 1/(1+expf(-x)) : expf(x)/(1+expf(x))
```

## 常见坑

- tanh-approx GELU 与 PyTorch 必须用相同系数（`0.044715`）
- `erfcf` 返回互补误差函数（`1 - erf`），公式需调整
- SiLU 的 sigmoid 用 `1/(1+exp(-x))` 在 x 很大时下溢（用分情况讨论）
- FP16 输入需先转 FP32 计算激活再转回
- epilogue 融合时，smem 若被 GEMM tile 占满则需调整 tile 大小

## 观察点

- tanh 多项式近似的项数与精度权衡（PyTorch 用 5 项多项式）
- epilogue 融合减少内存往返：分离 kernel 读 1 次写 1 次 × 3，融合读 1 次写 3 次
- GELU 计算量大 → 寄存器多 → occupancy 低（对比 SiLU）
- `erff` 在 x 极大/小时 erf 接近 ±1，浮点下溢

## 复盘问题

1. tanh-approx 和精确 GELU 的精度差异主要来自哪里？
2. 为什么 SiLU 的 sigmoid 需要分情况讨论？
3. epilogue 融合如何减少全局内存流量（理论字节数计算）？
4. 三个激活的寄存器占用分别是多少，为什么会不同？
5. 如何在不增加寄存器溢出的前提下融合激活到 GEMM？
6. sm_90a 上，向量内建函数相对标量的加速比是多少？

## 编译与运行

```bash
cmake --build build --target I3_gelu_silu_fused
./exercises/I3_gelu_silu_fused/I3_gelu_silu_fused

# 寄存器分析：
ncu --set full ./I3_gelu_silu_fused
```

## 参考文献

- CUDA C++ Programming Guide Section 4.4: "Mathematical Functions"
- PyTorch activation functions: https://pytorch.org/docs/stable/nn.functional.html
- NVIDIA Blog: "Fast and Accurate Approximations of GELU"

## 输出对照（printf / std::puts 原文）

- `[I3-T28]` (main.cu:194) 原文：`问题规模：N=%d  (%.1f M 元素)` → 现：`Problem size: N=%d  (%.1f M elements)`
- `[I3-T31]` (main.cu:213) 原文：`启动配置：grid=%d  block=%d` → 现：`Launch config: grid=%d  block=%d`
- `[I3-T32]` (main.cu:223) 原文：`[%s]  %.3f ms  max_err=%.2e  有效带宽=%.1f GB/s` → 现：`[%s]  %.3f ms  max_err=%.2e  effective_bw=%.1f GB/s`
- `[I3-T35]` (main.cu:271) 原文：`[fused_epilogue]  %.3f ms  (读 1x + 写 3x = 4x BW，stub 输出全零)` → 现：`[fused_epilogue]  %.3f ms  (read 1x + write 3x = 4x BW, stub outputs zeros)`
- `[I3-T38]` (main.cu:286) 原文：`tanh-approx vs 精确 GELU（CPU ref，[-5,5] 区间）：max_diff=%.2e  （验收：< 1e-2）` → 现：`tanh-approx vs exact GELU (CPU ref, [-5,5] range): max_diff=%.2e  (acceptance: < 1e-2)`
- `[I3-T39]` (main.cu:289) 原文：`验收目标：stub 实现后 gelu_tanh max_err < 1e-2，silu < 1e-4` → 现：`Acceptance: after stub is filled, gelu_tanh max_err < 1e-2, silu < 1e-4`
- `[I3-T40]` (main.cu:296) 原文：`[I3] 完成。用 ncu --set full ./I3_gelu_silu_fused 观察寄存器占用。` → 现：`[I3] done. Use ncu --set full ./I3_gelu_silu_fused to inspect register usage.`
