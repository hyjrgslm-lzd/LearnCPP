# I6 — FP8 E4M3 GEMM + Per-tensor / Per-block Scaling

## 目标

`[I6-T01]` (main.cu:2) 练习 I6：FP8 E4M3 GEMM + Per-tensor / Per-block Scaling。
`[I6-T02]` (main.cu:4) 学习目标：FP8 E4M3 数据范围、per-tensor/per-block scaling、dequant-fused matmul、FP8 vs FP16 精度漂移分析。
`[I6-T03]` (main.cu:11) 注意：FP8 需要 sm_89+（Ada Lovelace）或 sm_90a（Hopper）。在 sm_80/sm_86 设备上，运行时跳过 FP8 测试。

学会 FP8 E4M3 量化在 GEMM 中的应用：per-tensor scaling、per-block scaling、dequant-fused matmul。理解精度漂移与 scale 管理的关系。对标 TransformerEngine 的 FP8 策略。

## 硬件要求

**FP8 E4M3/E5M2 需要 sm_89+（Ada Lovelace）；sm_90a 推荐（Hopper wgmma FP8 路径）。**

在 sm_80/sm_86 设备上，程序会在运行时检测并跳过 FP8 测试（FP16 baseline 仍运行）。

## 问题规模

`[I6-T04]` (main.cu:25) FP8 数据类型头文件（CUDA 11.8+）。
`[I6-T05]` (main.cu:26) 在 sm_89+ 编译时可用；这里通过 preprocessor guard 保护。
`[I6-T06]` (main.cu:31) 主机端：尝试 include（CUDA 11.8+ toolkit 提供）。
`[I6-T07]` (main.cu:51) 问题规模常量。
`[I6-T08]` (main.cu:55) per-block scaling tile 大小。
`[I6-T09]` (main.cu:56) FP8 E4M3 最大绝对值。

| 参数 | 值 |
|------|----|
| M = N = K | 4096 |
| FP8 类型 | E4M3（`__nv_fp8_e4m3`） |
| Per-block tile | 64 × 64 |
| 累加器 | FP32 |
| FP8_MAX | 240（E4M3 最大绝对值） |

## 前置知识

- FP8 E4M3：4 exponent bits + 3 mantissa bits，范围 `±240`，精度 `1/8`
- per-tensor scaling：整个矩阵共用一个 scale = `amax / FP8_MAX`
- per-block scaling：每个 tile 一个独立 scale（更精确，额外存储 metadata）
- dequant-fused：在 GEMM mainloop 中直接反量化，避免写出中间矩阵

## FP8 类型别名

`[I6-T10]` (main.cu:60) FP8 E4M3 类型别名（guard 保护）。
`[I6-T11]` (main.cu:64) 非 FP8 编译：用 `uint8_t` 占位，运行时 skip。

## FP16 GEMM 基线

`[I6-T12]` (main.cu:68) Kernel 0：FP16 GEMM baseline（对照组，完全实现）。
`[I6-T13]` (main.cu:69) `C = A * B`，FP16 输入，FP32 累加，FP16 输出。

## Per-tensor 量化 kernel

`[I6-T14]` (main.cu:87) Kernel 1：per-tensor FP8 量化（amax scan + 量化写出）。
`[I6-T15]` (main.cu:89) TODO [必做] 步骤 2：per-tensor 量化。`scale = amax / FP8_MAX`，`A_fp8[i] = clamp(A[i] / scale, -FP8_MAX, FP8_MAX)`。
`[I6-T16]` (main.cu:103) TODO [必做] 步骤 2：FP32 → FP8 量化模板。

## Per-block 量化 kernel

`[I6-T17]` (main.cu:111) Kernel 2：per-block FP8 量化（每个 `TILE_SZ × TILE_SZ` block 独立 scale）。
`[I6-T18]` (main.cu:113) TODO [必做] 步骤 3：per-block 量化。每个 `[row_tile, col_tile]` block：`block_amax = max(abs(A[r:r+TILE, c:c+TILE]))`，`scale[r_tile, c_tile] = block_amax / FP8_MAX`。
`[I6-T19]` (main.cu:125) 每个 block 处理一个 tile。
`[I6-T20]` (main.cu:132) TODO [必做] 步骤 3：计算 block amax + 量化模板（4 步流程）。
`[I6-T21]` (main.cu:138) stub：跳过。

## Dequant-fused GEMM kernel

`[I6-T22]` (main.cu:144) Kernel 3：dequant-fused GEMM（per-tensor scaling）。
`[I6-T23]` (main.cu:146) `C = A_fp8 * B_fp8 * scale_A * scale_B`（在 mainloop 内直接反量化累加）。
`[I6-T24]` (main.cu:147) 避免写出中间的反量化矩阵。
`[I6-T25]` (main.cu:149) TODO [必做] 步骤 4：dequant-fused GEMM。在 mainloop 中：读 FP8 → 转 FP32 → 乘 scale → FP32 累加；最终写出 FP32 C（或转 FP16）。
`[I6-T26]` (main.cu:165) TODO [必做] 步骤 4：dequant-fused mainloop 模板。
`[I6-T27]` (main.cu:175) non-FP8 build：输出 0。

## 必做任务（对应 `main.cu` 中的 `// TODO [必做]` 标注）

1. **步骤 1**：构造 FP32 矩阵 A、B（高斯分布），小矩阵（64×64）用于精度验证。
2. **步骤 2**：实现 per-tensor FP8 量化：
   - `scale = max(abs(A)) / 240.0f`
   - `A_fp8 = clamp(A / scale, -240, 240)`（转 FP8）
   - 反量化验证：`A_recovered = A_fp8 * scale`
3. **步骤 3**：实现 per-block 量化（block = 64×64）：
   - 每 block 独立计算 `block_amax`
   - 存储 scale metadata（shape: `[ceil(M/64), ceil(K/64)]`）
4. **步骤 4**：实现 dequant-fused GEMM：
   - mainloop 中读 FP8、反量化到 FP32、累加
   - 不写出中间的反量化矩阵
5. **步骤 5**：对比精度：per-tensor FP8 vs FP32 baseline；per-block 应更精确。
6. **步骤 6**：Nsight Compute 观察 dequant-fused 的寄存器占用与吞吐。

## 进阶任务

`[I6-T28]` (main.cu:184) TODO [进阶] delayed scaling（TransformerEngine 风格）。
`[I6-T29]` (main.cu:188) TODO [进阶] per-block dequant-fused GEMM（逐 tile 读取 scale metadata）。
`[I6-T30]` (main.cu:192) TODO [进阶] 非对称量化（考虑 zero point）。

- 实现 delayed scaling（TransformerEngine 风格）：积累多步 amax，周期更新 scale
- 支持非对称量化（考虑 zero point）
- FP8 input + FP8 weight 的完整推理路径

## CPU 参考

`[I6-T31]` (main.cu:196) CPU 参考：FP32 GEMM（完全实现；小规模验证用）。

## 主程序

`[I6-T32]` (main.cu:230) 主程序入口。
`[I6-T33]` (main.cu:238) 运行时 FP8 能力检测（sm_89+ 必需）。
`[I6-T34]` (main.cu:243) 打印设备 Compute Capability。
`[I6-T35]` (main.cu:247) 打印 FP8 不支持的提示。
`[I6-T36]` (main.cu:249) 打印当前设备 CC 不满足要求。
`[I6-T37]` (main.cu:251) 打印 FP16 baseline 仍运行。
`[I6-T38]` (main.cu:252) 继续执行 FP16 baseline 部分，FP8 部分 skip。
`[I6-T39]` (main.cu:255) 打印 FP8 支持已确认。
`[I6-T40]` (main.cu:262) 打印问题规模。
`[I6-T41]` (main.cu:264) 打印 per-block tile 大小。
`[I6-T42]` (main.cu:267) TODO [必做] 步骤 1：构造 FP32 矩阵 A、B（高斯分布）。
`[I6-T43]` (main.cu:268) `M*N*K = 4096^3` 会非常大，CPU ref 只用小矩阵验证。
`[I6-T44]` (main.cu:269) 这里用小矩阵（64×64）做 CPU 正确性验证，大矩阵做性能测试。
`[I6-T45]` (main.cu:270) 验证用小矩阵大小。
`[I6-T46]` (main.cu:280) 打印小矩阵验证完成。
`[I6-T47]` (main.cu:282) per-tensor scale 计算（FP32 → FP8）。
`[I6-T48]` (main.cu:288) 打印 per-tensor scale。
`[I6-T49]` (main.cu:290) 打印 amax 与 FP8_MAX。
`[I6-T50]` (main.cu:294) GPU 分配（大矩阵 `M=N=K=4096` 性能测试）。
`[I6-T51]` (main.cu:295) `4096^2 * sizeof(fp16) = 32 MB per matrix`，FP8 减半。
`[I6-T52]` (main.cu:301) per-block scales for A。
`[I6-T53]` (main.cu:316) 初始化 FP16 数据（随机）。
`[I6-T54]` (main.cu:323) 启动配置（16×16 thread tile）。
`[I6-T55]` (main.cu:325) 打印启动配置。
`[I6-T56]` (main.cu:330) FP16 GEMM baseline。
`[I6-T57]` (main.cu:343) fp16_baseline 输出。
`[I6-T58]` (main.cu:345) FP8 dequant-fused GEMM（per-tensor scaling）。
`[I6-T59]` (main.cu:359) fp8_dequant 输出。
`[I6-T60]` (main.cu:362) 小矩阵精度验证（CPU ref vs GPU FP8 stub）。
`[I6-T61]` (main.cu:364) 打印精度分析标题。
`[I6-T62]` (main.cu:366) 打印 per-tensor 精度期望。
`[I6-T63]` (main.cu:368) 打印 per-block 精度期望。
`[I6-T64]` (main.cu:372) TODO [必做] 步骤 5：对比 per-tensor 和 per-block FP8 的相对误差。
`[I6-T65]` (main.cu:373) TODO [必做] 步骤 6：Nsight Compute 观察 dequant-fused 的寄存器占用与吞吐。
`[I6-T66]` (main.cu:375) 打印验收目标标题。
`[I6-T67]` (main.cu:377) 打印 per-tensor 验收。
`[I6-T68]` (main.cu:379) 打印 per-block 验收。
`[I6-T69]` (main.cu:381) 打印 dequant-fused 吞吐验收。
`[I6-T70]` (main.cu:392) 收尾提示：用 ncu 分析寄存器。

## 验收标准

| 指标 | 目标 |
|------|------|
| per-tensor FP8 vs FP32 | rel_err `>= 1e-2`（FP8 精度损失大，属预期） |
| per-block FP8 vs FP32 | rel_err `< 1e-3`（明显优于 per-tensor） |
| dequant-fused 吞吐 | 相对分离版本无明显下降 |
| 支持任意 M/K/N | 是（不仅 64 倍数） |

## 关键知识点

### FP8 E4M3 数值特性

| 属性 | 值 |
|------|----|
| 最大绝对值 | 240（不是 255，易混淆） |
| 精度 | 约 FP16 的 1/16 |
| 典型量化误差 | `±0.5 * (amax / 240) * 2^(-3)`|

### Per-tensor vs Per-block Scaling

```
per-tensor：一个 scale 表示整个矩阵的动态范围
  优点：简单，metadata 只有 1 个 float
  缺点：若矩阵存在极值（outlier），其他元素精度丢失严重

per-block（tile 64×64）：每个 tile 独立动态范围
  优点：精度更高，特别是存在 outlier 时
  缺点：需要存储 [M/64, K/64] 个 float scale（少量开销）
```

### 量化公式

```c
// 量化（FP32 -> FP8）：
scale   = amax / FP8_MAX;          // amax = max(abs(A))
A_fp8[i] = clamp(A[i] / scale, -FP8_MAX, FP8_MAX);

// 反量化（FP8 -> FP32）：
A_f32[i] = (float)A_fp8[i] * scale;

// dequant-fused GEMM mainloop：
float a = (float)A_fp8[row * K + k] * scale_A;
float b = (float)B_fp8[k  * N + col] * scale_B;
acc += a * b;  // 等价于：acc += A[i]*B[j] 但避免写出反量化矩阵
```

## FP8 编译要求

```cmake
# CMakeLists.txt 中已配置：
set_target_properties(I6_fp8_gemm_with_scaling PROPERTIES
    CUDA_ARCHITECTURES "89;90a")
# 说明：sm_89 = Ada Lovelace，sm_90a = Hopper
# 低于 sm_89 的设备无法编译使用 __nv_fp8_e4m3 的 device 函数
```

## 常见坑

- FP8 E4M3 最大值是 **240**，不是 255（易与 uint8 混淆）
- scale = `amax / 240`，不是 `amax`（方向搞反）
- 反量化忘记乘以 scale（只转 FP32 但没有乘）
- per-block metadata 索引计算：`tile_row = row / TILE_SZ`，不是 `row`
- dequant-fused 中 FP8 `(float)A_fp8[i]` 已做数值转换，但**不包含 scale 乘法**
- delayed scaling 的 amax 需要跨步骤积累（不是每步重算）

## 复盘问题

1. per-tensor vs per-block scaling 的精度-性能权衡是什么？
2. FP8 E4M3 的最大值为什么是 240？
3. dequant-fused GEMM 如何避免写出反量化矩阵？
4. per-block scale metadata 的存储开销与精度改进的权衡（多大的 block 最优）？
5. delayed scaling 如何工作，相对 just-in-time 的好处是什么？
6. 如何通过 Nsight Compute 测量 dequant-fused 的寄存器占用和吞吐？

## 编译与运行

```bash
# 需要 sm_89+ 设备（Ada Lovelace / Hopper）
cmake --build build --target I6_fp8_gemm_with_scaling
./exercises/I6_fp8_gemm_with_scaling/I6_fp8_gemm_with_scaling

# 在 sm_80/sm_86 设备上，FP8 部分会被跳过（graceful skip）

# 寄存器与吞吐分析：
ncu --set full ./I6_fp8_gemm_with_scaling
ncu --metrics sm__registers_per_thread_avg,l1tex__t_bytes ./I6_fp8_gemm_with_scaling
```

## 参考文献

- CUDA FP8 Datatypes: https://docs.nvidia.com/cuda/cuda-c-programming-guide/
- FP8 in Transformer Engine: https://github.com/NVIDIA/TransformerEngine
- NVIDIA Blog: "FP8 in Transformer Engine: Delivering Faster and Smaller AI Models"
- TransformerEngine delayed scaling: https://docs.nvidia.com/deeplearning/transformer-engine/user-guide/
- cuBLASLt FP8 GEMM: https://docs.nvidia.com/cuda/cublas/

## 输出对照（printf / std::puts 原文）

- `[I6-T34]` (main.cu:243) 原文：`设备 Compute Capability：%d.%d (cc=%d)` → 现：`Device Compute Capability: %d.%d (cc=%d)`
- `[I6-T35]` (main.cu:247) 原文：`[跳过] FP8 E4M3 需要 sm_89+（Ada Lovelace 或 Hopper）。` → 现：`[skip] FP8 E4M3 requires sm_89+ (Ada Lovelace or Hopper).`
- `[I6-T36]` (main.cu:249) 原文：`当前设备 CC=%d 不满足要求，程序正常退出。` → 现：`Current device CC=%d does not satisfy this; program exits gracefully.`
- `[I6-T37]` (main.cu:251) 原文：`FP16 baseline 仍会运行以验证框架正确性。` → 现：`FP16 baseline still runs to validate the framework.`
- `[I6-T39]` (main.cu:255) 原文：`FP8 支持：已确认（cc=%d >= 89）` → 现：`FP8 support: confirmed (cc=%d >= 89)`
- `[I6-T40]` (main.cu:262) 原文：`问题规模：M=%d  N=%d  K=%d` → 现：`Problem size: M=%d  N=%d  K=%d`
- `[I6-T41]` (main.cu:264) 原文：`Per-block tile 大小：%d×%d` → 现：`Per-block tile size: %dx%d`
- `[I6-T46]` (main.cu:280) 原文：`小矩阵验证（%dx%d）：CPU FP32 GEMM 参考已计算。` → 现：`Small matrix validation (%dx%d): CPU FP32 GEMM reference computed.`
- `[I6-T48]` (main.cu:288) 原文：`Per-tensor scale：scale_A=%.4f  scale_B=%.4f` → 现：`Per-tensor scale: scale_A=%.4f  scale_B=%.4f`
- `[I6-T49]` (main.cu:290) 原文：`  amax_A=%.4f  amax_B=%.4f  FP8_MAX=%.1f` → 现：保持英文
- `[I6-T55]` (main.cu:325) 原文：`启动配置（FP16/FP8 GEMM）：grid=(%d,%d,1)  block=(%d,%d,1)` → 现：`Launch config (FP16/FP8 GEMM): grid=(%d,%d,1)  block=(%d,%d,1)`
- `[I6-T57]` (main.cu:343) 原文：`[fp16_baseline]  %.3f ms  %.2f TFLOPS` → 现：保持英文
- `[I6-T59]` (main.cu:359) 原文：`[fp8_dequant]    %.3f ms  %.2f TFLOPS  (stub: 输出全零)` → 现：`[fp8_dequant]    %.3f ms  %.2f TFLOPS  (stub: outputs all zeros)`
- `[I6-T61]` (main.cu:364) 原文：`小矩阵精度分析（%dx%d，stub 实现后有意义）：` → 现：`Small-matrix accuracy analysis (%dx%d, meaningful once stub is implemented):`
- `[I6-T62]` (main.cu:366) 原文：`  per-tensor FP8 vs FP32 baseline：期望 rel_err >= 1e-2（可接受范围）` → 现：`  per-tensor FP8 vs FP32 baseline: expect rel_err >= 1e-2 (acceptable range)`
- `[I6-T63]` (main.cu:368) 原文：`  per-block  FP8 vs FP32 baseline：期望 rel_err < 1e-3（更精确）` → 现：`  per-block  FP8 vs FP32 baseline: expect rel_err < 1e-3 (more accurate)`
- `[I6-T66]` (main.cu:375) 原文：`验收目标（实现后）：` → 现：`Acceptance (after stub is implemented):`
- `[I6-T67]` (main.cu:377) 原文：`  per-tensor FP8 rel_err >= 1e-2（FP8 精度损失大，属预期）` → 现：`  per-tensor FP8 rel_err >= 1e-2 (FP8 precision loss is large, expected)`
- `[I6-T68]` (main.cu:379) 原文：`  per-block  FP8 rel_err <  1e-3（明显优于 per-tensor）` → 现：`  per-block  FP8 rel_err <  1e-3 (clearly better than per-tensor)`
- `[I6-T69]` (main.cu:381) 原文：`  dequant-fused 吞吐相对分离版本无明显下降` → 现：`  dequant-fused throughput: no significant drop vs separated version`
- `[I6-T70]` (main.cu:392) 原文：`[I6] 完成。用 ncu --set full ./I6_fp8_gemm_with_scaling 分析寄存器。` → 现：`[I6] done. Use ncu --set full ./I6_fp8_gemm_with_scaling to inspect registers.`
