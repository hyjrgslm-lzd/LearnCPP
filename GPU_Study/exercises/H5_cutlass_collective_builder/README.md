# 练习 H5：cutlass_collective_builder

## 1. 目标

`[H5-T01]` (main.cu:3) 练习 H5：CUTLASS 3.x CollectiveBuilder + Device API。
`[H5-T02]` (main.cu:5) 目标：用 CUTLASS 3.x 的 Device API 与 CollectiveBuilder 自动生成 Hopper warp-specialized GEMM kernel。理解五层 hierarchy（Device / Kernel / Collective / Tiled MMA+Copy / Atom），并与 H4 手写版本对比 TFLOPS。
`[H5-T03]` (main.cu:11) 编译要求：sm_90a（Hopper CollectiveBuilder），CUDA 13.x，CUTLASS 3.x headers。
`[H5-T04]` (main.cu:12) 依赖：`cutlass::headers`（由 CutlassSetup.cmake 提供）。

学会用 CUTLASS 3.x 的 Device API 与 CollectiveBuilder 自动生成高性能 GEMM kernel。理解 CUTLASS 的五层 hierarchy（Device / Kernel / Collective / Tiled MMA+Copy / Atom），以及如何通过改变高层参数自动生成不同的低层实现（sm_80 用 mma.sync，sm_90a 用 wgmma + TMA）。

## 2. 前置知识

- 完成 H1-H4，理解各优化阶段
- 理解 C++ template 参数与 type traits
- 了解 CUTLASS 的基础概念（五层 hierarchy，即使没用过）

## 3. 硬件要求

- **sm_90a 推荐**（Hopper CollectiveBuilder 使用 wgmma + TMA）
- sm_80 可以使用不同的 KernelSchedule（mma.sync）
- 非 Hopper GPU：程序运行时跳过 kernel，打印提示
- CUTLASS 3.x headers（由 `cutlass::headers` 别名提供）

## 4. 文件说明

| 文件 | 说明 |
|------|------|
| `main.cu` | CUTLASS CollectiveBuilder boilerplate + 性能测量 |
| `CMakeLists.txt` | CUDA_ARCHITECTURES = "90a"，链接 `cutlass::headers` |
| `README.md` | 本文件 |

## 5. 必做任务

### 步骤 1 — include CUTLASS 头文件

`[H5-T05]` (main.cu:16) CUTLASS 核心头文件。
`[H5-T06]` (main.cu:33) 练习公共头文件。
`[H5-T07]` (main.cu:50) Hopper 运行时检测。
`[H5-T08]` (main.cu:61) 超参。

```cpp
#include "cutlass/gemm/device/gemm_universal_adapter.h"
#include "cutlass/gemm/kernel/gemm_universal.hpp"
#include "cutlass/gemm/collective/collective_builder.hpp"
#include "cutlass/epilogue/collective/collective_builder.hpp"
```

### 步骤 2 — 定义数据类型与 Layout

`[H5-T09]` (main.cu:67) CUTLASS 类型别名。
`[H5-T10]` (main.cu:69) TODO [必做] 步骤 3：用 CollectiveBuilder 生成 CollectiveMainloop。
`[H5-T11]` (main.cu:76) 数据类型。
`[H5-T12]` (main.cu:77) `ElementA`：FP16。
`[H5-T13]` (main.cu:78) `ElementB`：FP16。
`[H5-T14]` (main.cu:79) `ElementC`：FP32 输出。
`[H5-T15]` (main.cu:80) `ElementAccum`：FP32 累加器。
`[H5-T16]` (main.cu:82) Layout（A 行主，B 列主是 CUTLASS 约定的 NT GEMM 格式）。
`[H5-T17]` (main.cu:87) 对齐（FP16：128-bit = 8 elements）。

```cpp
using ElementA     = cutlass::half_t;
using ElementB     = cutlass::half_t;
using ElementC     = float;
using ElementAccum = float;
using LayoutA = cutlass::layout::RowMajor;
using LayoutB = cutlass::layout::ColumnMajor;  // NT GEMM 标准格式
using LayoutC = cutlass::layout::RowMajor;
```

### 步骤 3 — CollectiveBuilder 生成 CollectiveMainloop

`[H5-T18]` (main.cu:92) TODO [必做] 步骤 3：CollectiveMainloop。
`[H5-T19]` (main.cu:111) stub：仅占位，编译可通过；student 替换为真实 CollectiveBuilder。
`[H5-T20]` (main.cu:117) TODO [必做] 步骤 3（学生填写区域）。

```cpp
using CollectiveMainloop =
  typename cutlass::gemm::collective::CollectiveBuilder<
    cutlass::arch::Sm90,
    cutlass::arch::OpClassTensorOp,
    ElementA, LayoutA, /*AlignA=*/8,
    ElementB, LayoutB, /*AlignB=*/8,
    ElementAccum,
    cutlass::gemm::Shape<_128, _128, _64>,   // CTA tile shape
    cutlass::gemm::Shape<_1, _2, _1>,         // cluster shape
    cutlass::gemm::collective::StageCountAuto,
    cutlass::gemm::collective::KernelScheduleAuto
  >::CollectiveOp;
```

### 步骤 4 — KernelSchedule 对比

`[H5-T21]` (main.cu:133) TODO [必做] 步骤 4：KernelSchedule 与 CollectiveEpilogue。

将 `KernelScheduleAuto` 替换为以下两种之一，对比性能：

| Schedule | 说明 |
|----------|------|
| `KernelTmaWarpSpecializedCooperative` | 合作式：producer/consumer warp 共同推进，适合大 tile |
| `KernelTmaWarpSpecializedPingpong` | Pingpong 式：producer/consumer 交替，适合中等 tile |

### 步骤 5 — GemmUniversalAdapter 封装与运行

`[H5-T22]` (main.cu:149) TODO [必做] 步骤 5：GemmKernel + GemmUniversalAdapter。
`[H5-T31]` (main.cu:243) TODO [必做] 步骤 5：构建 CUTLASS Gemm 参数并运行。

```cpp
using GemmKernel = cutlass::gemm::kernel::GemmUniversal<
    cutlass::gemm::Shape<int, int, int, int>,
    CollectiveMainloop,
    CollectiveEpilogue
>;
using Gemm = cutlass::gemm::device::GemmUniversalAdapter<GemmKernel>;

typename Gemm::Arguments args{
    cutlass::gemm::GemmUniversalMode::kGemm,
    {M, N, K},
    {dA, K, dB, K, dC, N, dC, N},
    {1.0f, 0.0f}
};

Gemm gemm_op;
gemm_op.initialize(args, workspace);
gemm_op.run();
```

### 步骤 6 — 测量 TFLOPS，与 H4 对比

`[H5-T26]` (main.cu:209) TODO [必做] 步骤 2：问题规模。
`[H5-T40]` (main.cu:305) TODO [必做] 步骤 6：测量吞吐，与 H4 手写版本对比。

## 6. 进阶任务

`[H5-T41]` (main.cu:309) TODO [进阶] 更换不同 CollectiveBuilder 配置（tile size / schedule）。
`[H5-T42]` (main.cu:313) TODO [进阶] 在 sm_80 与 sm_90a 上各运行，观察 PTX 差异。
`[H5-T43]` (main.cu:317) TODO [进阶] 尝试 split-K 模式（`GemmUniversalMode::kGemmSplitKParallel`）。

- 尝试不同 CollectiveBuilder 配置（不同 tile size、不同 schedule），对比性能
- 在 sm_80 与 sm_90a 上各运行一遍，观察自动生成的 PTX 差异
- 尝试 split-K 模式：`GemmUniversalMode::kGemmSplitKParallel`

## 7. 验收标准

- kernel 编译通过（CUTLASS 3.x headers only，无外部动态库依赖）
- GEMM 结果与 H4 逐元素一致（容差 1e-2）
- 吞吐与手写 H4 相近（±10%）
- 能在 CUTLASS 生成的 PTX 中识别 wgmma / TMA / mbarrier 指令

## 8. 常见坑

| 坑 | 说明 |
|----|------|
| CollectiveBuilder 参数组合非法 | 某些 tile size / precision 组合不支持，编译报长错误 |
| 模板实例化失败 | 错误信息极其冗长，需要逐行找根本 error |
| M/N/K 非 tile 倍数 | 需要配置 epilogue 的 padding，否则结果错误 |
| workspace 忘记分配 | split-K 等模式需要 workspace；`get_workspace_size()` 返回非零时必须分配 |
| FetchContent 未拉取 | `cutlass::headers` 别名不可用时是 FetchContent 网络问题，非本练习问题 |

## 9. CUTLASS 五层 Hierarchy

```
Device Layer     — GemmUniversalAdapter：封装整个 kernel 参数与 launch
Kernel Layer     — GemmUniversal：定义 grid mapping 与主循环
Collective Layer — CollectiveMainloop/Epilogue：定义数据流动与同步
Tiled MMA/Copy   — TiledMma / TiledCopy：定义小块计算与搬运模式
Atom Layer       — MMA_Atom / Copy_Atom：硬件最小操作单元
```

## 10. 复盘问题

1. CUTLASS 五层 hierarchy 各层分别负责什么，为什么这样分层有利？
2. CollectiveBuilder 如何从高层参数自动生成低层实现？
3. `KernelTmaWarpSpecializedCooperative` vs `KernelTmaWarpSpecializedPingpong` 的区别是什么？

## 参考资料

- CUTLASS GitHub: `media/docs/cpp/gemm_api_3x.md`
- CUTLASS examples: `examples/48_hopper_warp_specialized_gemm/`
- CUTLASS `include/cutlass/gemm/collective/` 源码

## 输出对照（printf / std::puts 原文）

- `[H5-T23]` (main.cu:160) CPU 参考实现。
- `[H5-T24]` (main.cu:197) `main` 入口。
- `[H5-T25]` (main.cu:206) Hopper 运行时检测。原文：`[H5] 当前 GPU 不支持 Hopper (sm_90a)，跳过。` → 现：`[H5] Current GPU lacks Hopper (sm_90a); skipping.`
- `[H5-T27]` (main.cu:212) 原文：`问题规模：M=%d  N=%d  K=%d` → 现：`Problem size: M=%d  N=%d  K=%d`
- `[H5-T28]` (main.cu:215) 主机内存。
- `[H5-T29]` (main.cu:230) 原文：`正在计算 CPU 参考...` → 现：`Computing CPU reference...`
- `[H5-T30]` (main.cu:233) 设备内存。
- `[H5-T32]` (main.cu:282) stub：直接用 cudaMemset 产生零输出。
- `[H5-T33]` (main.cu:283) 占位。
- `[H5-T34]` (main.cu:284) 原文：`[stub] CUTLASS kernel 未实现，输出为零（CPU check 预期 FAIL）。` → 现：`[stub] CUTLASS kernel not implemented; output is zero (CPU check FAIL is expected).`
- `[H5-T35]` (main.cu:288) 正确性检查。
- `[H5-T36]` (main.cu:290) 原文：`── 正确性检查（stub 阶段预期 FAIL）──` → 现：`-- Correctness check (FAIL expected at stub stage) --`
- `[H5-T37]` (main.cu:295) 性能汇总。
- `[H5-T38]` (main.cu:299) 原文：`── 性能汇总 ──...` → 现：`-- Performance summary --`
- `[H5-T39]` (main.cu:303) 原文：`(上表数据为 stub 占位，不代表真实性能)` → 现：`(numbers above are stub placeholders, not real performance)`
- `[H5-T44]` (main.cu:324) 原文：`[H5] 完成。` → 现：`[H5] done.`
