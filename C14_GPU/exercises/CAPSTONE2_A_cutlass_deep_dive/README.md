# 结课项目 2 — 分支 A：CUTLASS Kernel 精读 + 手写 warp-specialized GEMM

## 题目概述

选择 CUTLASS `examples/` 下一个 SM90 warp-specialized GEMM（例如
`48_hopper_warp_specialized_gemm` 或 `55_hopper_mixed_dtype_gemm`），
深入阅读其 5 层 hierarchy（Device / Kernel / Collective / Tiled MMA+Copy / Atom），
理解 `CollectiveMainloop` 如何用 TMA + wgmma 流水消除延迟，
画出完整的 data flow 图和同步点，
再用裸 CUDA PTX + cuTe 写一个功能等价的简化版（目标 ≥80% CUTLASS 性能）。

**问题规模**：FP16 GEMM，A(4096×4096) × B(4096×4096) → D(4096×4096, FP32)

---

## 硬件与依赖要求

| 项目 | 要求 |
|------|------|
| GPU | Hopper sm_90a（H100/H800）；非 Hopper 机器运行时自动跳过 |
| CUDA Toolkit | 13.x 或更新 |
| CUTLASS | 3.x（通过 `CutlassSetup.cmake` FetchContent 自动拉取） |
| CMake | 3.28+ |
| Nsight Compute | `ncu` 命令可用（性能采集必需） |

---

## 必做任务清单

- [ ] **步骤 1**：选定 CUTLASS example（≤600 行 device code，含 TMA + wgmma + mbarrier）
- [ ] **步骤 2**：阅读 5 层 hierarchy，绘制架构图（ASCII art 或纸质），标注各层职责
- [ ] **步骤 3**：定位 TMA + wgmma 核心循环，标注 producer/consumer 代码段行号
- [ ] **步骤 4**：绘制 shared memory layout 图，标注 swizzle 规则、bank 占用
- [ ] **步骤 5**：提炼 3–5 项关键设计决策与性能影响
- [ ] **步骤 6**：撰写精读笔记（≥1500 字，输出至 `NOTES.md`）
- [ ] **步骤 7**：实现手写 warp-specialized GEMM（`main.cu` 中 TODO [必做] 框架）
- [ ] **步骤 8**：用 Nsight Compute 采集两条路径的性能数据，输出对比表
- [ ] **步骤 9**：Python 验证脚本（max relative error < 1e-3）

---

## 编译与运行

### 前提条件

1. 确保 CUDA Toolkit 13.x 已安装（`nvcc --version` 确认）
2. CUTLASS 会通过 FetchContent 自动下载；首次配置需要网络连接
3. 仅在 Hopper (sm_90a) 机器上运行，非 Hopper 自动 skip

### 编译步骤

```bash
# 从 exercises/ 目录配置（如果还没有配置）
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# 仅编译分支 A
cmake --build build --target CAPSTONE2_A_cutlass_deep_dive --config Release -j8

# Windows Visual Studio 生成器
cmake --build build-vs2026 --target CAPSTONE2_A_cutlass_deep_dive --config Release
```

### 运行

```bash
# Linux / WSL
./build/CAPSTONE2_A_cutlass_deep_dive/CAPSTONE2_A_cutlass_deep_dive

# Windows
build-vs2026\Release\CAPSTONE2_A_cutlass_deep_dive.exe
```

预期输出（stub 阶段）：

```
[CAPSTONE2_A] CUTLASS 精读 + 手写 warp-specialized GEMM
=== Device 0: NVIDIA H100 SXM5 80GB ===
  Compute Capability : 9.0  (sm_90a)
  ...
路径 1：CUTLASS CollectiveBuilder（参考实现）
  CUTLASS 平均耗时：X.XXX ms
路径 2：手写 warp-specialized GEMM（TODO 实现后替换 stub）
  手写 kernel（stub）平均耗时：X.XXX ms
  注意：stub 输出全零，验证将 FAIL，实现 TODO 后替换。

── 正确性检查
  [cutlass_path] PASS  (NaN check)
  [manual_vs_cutlass] FAIL  (stub 阶段预期 FAIL)

── 性能汇总
变体                                 |   ms/iter |     TFLOPS | % peak | % of CUTLASS
cutlass_collective_cooperative       |     X.XXX |    XXX.XXX | XX.XX% |      100.00%
manual_warp_specialized_sm90 (stub)  |     X.XXX |    XXX.XXX | XX.XX% |      XXXX.XX%
```

### Nsight Compute 性能采集

```bash
# 完整采集（含 source-level SASS 映射）
ncu --set full --target-processes all \
    -o capstone_a.ncu-rep \
    ./CAPSTONE2_A_cutlass_deep_dive

# 查看关键指标
ncu --import capstone_a.ncu-rep \
    --csv \
    --metrics \
    sm__pipe_tensor_cycles_active.avg.pct_of_peak_sustained_active,\
l1tex__t_bytes_pipe_lsu_mem_global_op_ld.sum.pct_of_peak_sustained_elapsed,\
l2_global_load_bytes,\
smsp__sass_inst_executed.sum \
    > metrics.csv

# compute-sanitizer 验证无 race condition（实现完成后运行）
compute-sanitizer --tool racecheck ./CAPSTONE2_A_cutlass_deep_dive
```

---

## 验收标准

| 项目 | 验收点 |
|------|--------|
| 精读笔记 | `NOTES.md` ≥1500 字，逻辑清晰，能讲清"为什么这样设计" |
| 手写 GEMM 编译 | 无编译错误，无 compute-sanitizer race condition |
| 性能 | 手写版 ≥ CUTLASS baseline 的 80% TFLOPS |
| 正确性 | max relative error < 1e-3（FP16 量化误差） |
| Nsight 报告 | Tensor Core 利用率 ≥ 70%，Warp 占用率 ≈ 100%，L2 命中率 > 80% |

---

## 复盘问题（自检）

1. CUTLASS 的 5 层 hierarchy 中，各层分别对应"代码"和"硬件"的哪些概念？
2. 为什么 warp specialization 比"所有 warp 既加载又计算"的设计更高效？
3. TMA 的硬件流水能带来什么优势？如何在代码中体现这种流水？
4. `mbarrier + wgmma.mma_async` 与传统 `__syncthreads + mma.sync` 的本质区别是什么？
5. 如果把 block tile 大小从 (64, 128) 改为 (128, 256)，性能会怎样变化？涉及哪些权衡？
6. 你的手写版本达到 baseline 的百分之几？差距来自哪些方面？
7. 如何验证"shared memory bank conflict 被消除"？用什么 Nsight 指标？

---

## 参考资料

- [CUTLASS 3.x Programming Guide](https://github.com/NVIDIA/cutlass/blob/main/media/docs/)
- [CUTLASS example 48_hopper_warp_specialized_gemm](https://github.com/NVIDIA/cutlass/tree/main/examples)
- [NVIDIA Hopper Tuning Guide](https://docs.nvidia.com/cuda/hopper-tuning-guide/)
- [CUTLASS Paper (VLDB 2017)](https://arxiv.org/abs/1706.04319)
- [cuTe Programming Guide](https://github.com/NVIDIA/cutlass/blob/main/media/docs/cute/00_quickstart.md)

---

## 交付物清单

| 交付物 | 文件 | 说明 |
|--------|------|------|
| 源码精读笔记 | `NOTES.md` | ≥1500 字 Markdown |
| 实现代码 | `main.cu` | TODO 部分已实现 |
| Nsight 报告 | `capstone_a.ncu-rep` | `ncu --set full` 采集 |
| 性能对比表 | 程序运行输出 | 截图或重定向至 txt |
| 验证脚本 | `verify.py`（可选） | Python max error 验证 |
