# I1 — Online Softmax（Milakov & Gimelshein 2018）

## 目标

`[I1-T01]` (main.cu:2) 练习 I1：Online Softmax（Milakov & Gimelshein 2018）。
`[I1-T02]` (main.cu:4) 学习目标：一趟 online softmax，warp shuffle reduce，block 跨 warp 合并，与 two-pass safe softmax 的精度/性能对比。

学会 Milakov & Gimelshein 2018 的 online softmax：一趟计算，running max + running denominator。理解为什么这比"先找 max、再计算 exp、再求和"的两趟法快，且数值精度接近甚至更优。

## 问题规模

`[I1-T03]` (main.cu:33) 问题规模常量。
`[I1-T04]` (main.cu:34) batch（行数）。
`[I1-T05]` (main.cu:35) 每行长度。
`[I1-T06]` (main.cu:36) 每个 block 处理的行数。

| 参数 | 值 |
|------|----|
| Batch（行数）B | 32 |
| 行长度 N | 4096 |
| 数据类型 | float32 |
| 布局 | row-major |

## 前置知识

- softmax 定义：`exp(x - max) / sum(exp(x - max))`
- running max 更新：`max_new = max(max_old, x_i)`
- 修正因子：`output_i *= exp(max_old - max_final)`
- warp shuffle 原语：`__shfl_down_sync`、`__shfl_sync`

## 设备工具

`[I1-T07]` (main.cu:41) 设备工具：warp reduce max。
`[I1-T08]` (main.cu:43) 使用 `__shfl_down_sync` 在 warp 内做树形 max reduce。
`[I1-T09]` (main.cu:53) 设备工具：warp reduce sum。

## Two-pass 基线 kernel

`[I1-T10]` (main.cu:65) Kernel 1：two-pass safe softmax（对照组，CPU 验证基准）。
`[I1-T11]` (main.cu:66) 第一趟找每行最大值；第二趟计算 `exp(x - max) / sum`。
`[I1-T12]` (main.cu:69) 每个 warp 处理一行；`blockDim.x = WARP_SIZE * BLOCK_ROWS`。
`[I1-T13]` (main.cu:78) 第一趟：找最大值（tile 循环，每次 WARP_SIZE 个元素）。
`[I1-T14]` (main.cu:84) lane 0 广播 row_max（已通过 shfl 全 warp 可见）。
`[I1-T15]` (main.cu:87) 第二趟：计算 `exp(x - max)` 并累加分母。
`[I1-T16]` (main.cu:95) 写出结果。

## Online softmax 待实现 kernel

`[I1-T17]` (main.cu:101) Kernel 2：online softmax（一趟，Milakov 2018）。
`[I1-T18]` (main.cu:102) 每个 warp 处理一行，单趟扫描维护 `running_max` 和 `running_sum`。
`[I1-T19]` (main.cu:105) TODO [必做] 步骤 2：warp-level online softmax（`d <= 32` 的简化版）。
`[I1-T20]` (main.cu:106) TODO [必做] 步骤 3：block 级扩展，支持任意行长（warp 内循环 + 跨 warp 合并）。
`[I1-T21]` (main.cu:107) TODO [必做] 步骤 4：block-level reduce：smem 汇合多 warp 的 running_max。
`[I1-T22]` (main.cu:113) 每个 warp 处理一行。
`[I1-T23]` (main.cu:121) TODO [必做] 步骤 2/3：单趟 online softmax 模板。
`[I1-T24]` (main.cu:122) 初始化 running state。
`[I1-T25]` (main.cu:135) TODO [必做] 步骤 4：warp reduce 合并 running_max 和 running_sum，注意修正因子。
`[I1-T26]` (main.cu:142) 写出（stub：输出 0，学生完成 TODO 后才会输出正确值）。
`[I1-T27]` (main.cu:146) 替换为：`row_out[i] = expf(row_in[i] - running_max_final) / running_sum_final;`。

## 必做任务（对应 `main.cu` 中的 `// TODO [必做]` 标注）

1. **步骤 1**：构造 row-major matrix（shape: `B × N`），初始化随机值，最大值跨度 1e-3 到 1e3。
2. **步骤 2**：实现 warp-level online softmax（一条 warp 处理一行，假设 `d <= 32`）：
   - 初始化 `running_max = -INFINITY`、`running_sum = 0.0f`
   - 逐个 lane 读元素，更新 running_max，用修正因子更新 running_sum
3. **步骤 3**：block 级扩展：多个 warp 处理多行；每行长度支持任意值（warp 内循环 + 跨 warp 合并）。
4. **步骤 4**：block-level reduce：用 shared memory 汇合多 warp 的 running_max，再做全局 max 修正。
5. **步骤 5**：对比输出：online softmax 与两趟安全 softmax 的元素差异应 `< 1e-5`（FP32）。
6. **步骤 6**：用 Nsight Compute 测量 block-level reduce 吞吐（应接近 L2 读带宽）。

## 进阶任务

`[I1-T28]` (main.cu:150) TODO [进阶] FP16 输入版本（FP32 累加器）。
`[I1-T29]` (main.cu:151) 函数声明示例。
`[I1-T30]` (main.cu:155) TODO [进阶] block-sparse mask softmax（某些 block 跳过 exp 计算）。
`[I1-T31]` (main.cu:159) TODO [进阶] FP8 running max + sum（带 rescaling）。

- 实现 FP16 版本（FP32 累加器），观察精度与吞吐的权衡
- 支持 block-sparse attention mask（可选某些 block 跳过 softmax）
- 用 FP8 运行 max 和 sum（带 rescaling）

## CPU 参考与验证

`[I1-T32]` (main.cu:163) CPU 参考：two-pass safe softmax（完全实现，用于验证）。
`[I1-T33]` (main.cu:171) 第一趟：找最大值。
`[I1-T34]` (main.cu:175) 第二趟：`exp(x - max) / sum`。
`[I1-T35]` (main.cu:184) 验证：计算两个数组的最大绝对误差。

## 主程序

`[I1-T36]` (main.cu:194) 主程序入口。
`[I1-T37]` (main.cu:203) TODO [必做] 步骤 1：构造 row-major matrix，随机初始化。
`[I1-T38]` (main.cu:204) 这里已实现初始化以便学生观察正确的输入分布。
`[I1-T39]` (main.cu:209) 值域跨度从 1e-3 到 1e3（使用均匀分布在 [-6, 6] 模拟）。
`[I1-T40]` (main.cu:213) 打印问题规模。
`[I1-T41]` (main.cu:215) CPU 参考。
`[I1-T42]` (main.cu:218) GPU 分配。
`[I1-T43]` (main.cu:225) 启动配置：每 block 有 BLOCK_ROWS 个 warp，每 warp 处理一行。
`[I1-T44]` (main.cu:227) 打印启动配置。
`[I1-T45]` (main.cu:233) Two-pass softmax（对照组）。
`[I1-T46]` (main.cu:249) 有效带宽：读 B*N float + 写 B*N float（两趟共 2x2xB*N bytes）。
`[I1-T47]` (main.cu:251) two-pass 计时与 max_err 输出。
`[I1-T48]` (main.cu:255) Online softmax（待实现）。
`[I1-T49]` (main.cu:271) 有效带宽：理论上一趟只读写一次（1x2xB*N bytes）。
`[I1-T50]` (main.cu:273) online 计时与 max_err 输出。
`[I1-T51]` (main.cu:279) TODO [必做] 步骤 5：对比 online vs two-pass 差异应 < 1e-5（实现后检查）。
`[I1-T52]` (main.cu:280) TODO [必做] 步骤 6：用 Nsight Compute 测量 block-level reduce 吞吐。
`[I1-T53]` (main.cu:282) 打印验收目标。
`[I1-T54]` (main.cu:283) 打印当前 stub 行为说明。
`[I1-T55]` (main.cu:289) 收尾提示：用 ncu 分析寄存器与带宽。

## 验收标准

| 指标 | 目标 |
|------|------|
| online softmax 误差 vs CPU ref | `< 1e-5`（FP32） |
| 吞吐 | `>= GPU 峰值带宽的 40%` |
| 寄存器溢出 | 无（Nsight Compute 验证） |
| 支持任意行长 | 是（不仅 32 的倍数） |

## 关键公式

```
// online softmax 核心（单趟）：
new_output[i] = old_output[i] * exp(old_max - new_max)

// warp reduce max（D3 中的基础原语）：
for (int offset = 16; offset > 0; offset >>= 1) {
    float other = __shfl_down_sync(0xffffffff, running_max, offset);
    running_max = fmaxf(running_max, other);
}
```

## 常见坑

- `running_max` 必须初始化为 `-INFINITY`，而不是 0 或 `-FLT_MAX`
- 修正因子 `exp(old_max - new_max)` 容易写反（方向错误）
- warp reduce 时某个 warp 的 max 没被所有 lane 看到（broadcast 前忘记同步）
- block-level reduce 中 shared memory bank conflict（没用 warp ID 作 offset）
- 长行处理时，没有处理"最后不足一 warp"的 tile

## 观察点

- running_max 初始化为 `-INFINITY` 是必要的（与 `-FLT_MAX` 的差别）
- 一趟法的精度有时比两趟法更好（浮点舍入累积误差减少）
- block-level reduce 需要在 shared memory 前先加 `__syncthreads()`

## 复盘问题

1. 为什么 `running_max` 初始化为 `-INFINITY` 而不是第一个元素？
2. 一趟法的精度有时比两趟法更优，为什么？
3. 如何在 warp reduce 时确保每个 lane 的 max 被正确合并？
4. 为什么需要 block-level reduce，而不是每个 warp 独立处理？
5. row length 不是 32 倍数时，如何避免 warp divergence？
6. 如何通过 Nsight Compute 的 L2 hit/miss 比例验证内存访问模式？

## 编译与运行

```bash
cmake --build build --target I1_softmax_online
./exercises/I1_softmax_online/I1_softmax_online

# Nsight Compute 分析：
ncu --set full ./I1_softmax_online
ncu --metrics l1tex__t_bytes,lts__t_bytes ./I1_softmax_online
```

## 参考文献

- arXiv 1805.02867: "Online Softmax" (Milakov & Gimelshein 2018)
- CUDA C++ Programming Guide Section 4.3: "Warp Shuffle Functions"
- CUB library: `warp_reduce` example

## 输出对照（printf / std::puts 原文）

- `[I1-T40]` (main.cu:213) 原文：`问题规模：B=%d  N=%d  total_floats=%d` → 现：`Problem size: B=%d  N=%d  total_floats=%d`
- `[I1-T44]` (main.cu:228) 原文：`启动配置：grid=(%d,1,1)  block=(%d,%d,1)  每 block 处理 %d 行` → 现：`Launch config: grid=(%d,1,1)  block=(%d,%d,1)  rows/block=%d`
- `[I1-T47]` (main.cu:251) 原文：`[two_pass]  %.3f ms  max_err=%.2e  有效带宽=%.1f GB/s` → 现：`[two_pass]  %.3f ms  max_err=%.2e  effective_bw=%.1f GB/s`
- `[I1-T50]` (main.cu:273) 原文：`[online]    %.3f ms  max_err=%.2e  有效带宽=%.1f GB/s  (stub: 期望 err < 1e-5 实现后)` → 现：`[online]    %.3f ms  max_err=%.2e  effective_bw=%.1f GB/s  (stub: expect err < 1e-5 once implemented)`
- `[I1-T53]` (main.cu:282) 原文：`验收目标：online softmax max_err vs CPU ref < 1.0e-05（FP32）` → 现：`Acceptance: online softmax max_err vs CPU ref < 1.0e-05 (FP32)`
- `[I1-T54]` (main.cu:283) 原文：`当前 stub 输出全零 → CPU check 会报 mismatch，这是预期行为。` → 现：`Current stub outputs all zeros; CPU check will report mismatch (expected).`
- `[I1-T55]` (main.cu:289) 原文：`[I1] 完成。使用 ncu --set full ./I1_softmax_online 分析寄存器与带宽。` → 现：`[I1] done. Use ncu --set full ./I1_softmax_online to inspect registers and bandwidth.`
