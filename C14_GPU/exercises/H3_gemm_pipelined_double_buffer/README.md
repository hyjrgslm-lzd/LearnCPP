# 练习 H3：gemm_pipelined_double_buffer

## 1. 目标

`[H3-T01]` (main.cu:3) 练习 H3：双缓冲/多级 Pipeline GEMM（cp.async + cuda::pipeline）。
`[H3-T02]` (main.cu:5) 目标：在 H2 warp tile 基础上，引入 2-stage 与 3-stage 双缓冲 shared memory pipeline。producer 用 `cp.async`（sm_80+）异步加载下一个 K tile，consumer warp 同时对当前 tile 执行 mma.sync，消除 global memory 延迟。
`[H3-T03]` (main.cu:11) 编译要求：sm_80+，CUDA 13.x，C++20 device / C++26 host。

在 H2 warp tile 基础上，引入 producer-consumer pipeline：两个（或三个）stage 的 shared memory buffer。producer 线程用 `cp.async`（`cuda::memcpy_async`）提前加载下一个 tile 到 buffer，consumer warp 对当前 buffer 执行 mma.sync，实现计算与数据加载的重叠，消除 global memory 加载延迟。

## 2. 前置知识

- 完成 H2，理解 warp tile + Tensor Core 的结构
- 理解 double-buffering 的概念与同步机制
- 理解 `cp.async`（sm_80+）与 `cuda::pipeline` / `cuda::barrier` 的 API
- 包含 `<cuda/pipeline>`, `<cuda/barrier>`, `<cooperative_groups/memcpy_async.h>`

## 3. 硬件要求

- sm_80+（cp.async 需要 Ampere+）
- CUDA Toolkit 13.x+

## 4. 文件说明

| 文件 | 说明 |
|------|------|
| `main.cu` | 无 pipeline 基线 + 2-stage + 3-stage 三个版本 |
| `CMakeLists.txt` | CUDA_ARCHITECTURES = 80;86;89;90a |
| `README.md` | 本文件 |

## 5. 必做任务

### 步骤 1 — 两个 smem buffer

`[H3-T04]` (main.cu:32) 常量与超参。
`[H3-T05]` (main.cu:36) `NUM_STAGES`：修改为 3 可测试三级 pipeline。
`[H3-T06]` (main.cu:42) 版本 1（基线）：无 pipeline 的 tiled GEMM（FP16 输入 + FP32 累加）。
`[H3-T07]` (main.cu:56) TODO [必做] 步骤 1：复制 H2 的 tiled 逻辑作为无 pipeline 基线。
`[H3-T08]` (main.cu:61) stub。
`[H3-T09]` (main.cu:64) 版本 2：2-stage double-buffer pipeline（cuda::pipeline API）。
`[H3-T10]` (main.cu:77) TODO [必做] 步骤 1：声明多 stage smem buffer。
`[H3-T11]` (main.cu:88) TODO [必做] 步骤 2：初始化阶段 — 加载第 0 个 tile 到 stage 0。
`[H3-T12]` (main.cu:92) TODO [必做] 步骤 3：pipeline 主循环。
`[H3-T13]` (main.cu:123) stub。
`[H3-T14]` (main.cu:126) 版本 3：3-stage pipeline（修改 NUM_STAGES=3 后的变体）。
`[H3-T15]` (main.cu:144) TODO [必做] 步骤 4：实现三级 pipeline。
`[H3-T16]` (main.cu:151) stub。

声明：

```cpp
__shared__ __half sA[NUM_STAGES][BM][BK];
__shared__ __half sB[NUM_STAGES][BK][BN];
```

### 步骤 2 — 初始化阶段

加载第 0 个 tile 到 stage 0，提交到 pipeline：

```cpp
__pipeline_memcpy_async(&sA[0][ty][tx], &A[...], sizeof(__half));
__pipeline_commit();
```

### 步骤 3 — pipeline 主循环（2-stage）

```
for tile = 1 .. num_tiles:
  cur_stage  = tile % 2
  prev_stage = (tile - 1) % 2

  // producer：异步加载 tile 到 cur_stage
  __pipeline_memcpy_async(...)
  __pipeline_commit()

  // 等待 prev_stage 数据就位
  __pipeline_wait_prior(1)
  __syncthreads()

  // consumer：对 prev_stage 执行 mma.sync
  for k in 0..BK: acc += sA[prev_stage][...] * sB[prev_stage][...]

// 末尾：等待并处理最后一个 tile
__pipeline_wait_prior(0)
__syncthreads()
```

### 步骤 4 — 3-stage pipeline

`[H3-T23]` (main.cu:254) TODO [必做] 步骤 4：测量 3-stage pipeline。

与步骤 3 逻辑相同，将 stage 数改为 3。注意需要预填充 2 个 tile（stage 0 和 stage 1），`__pipeline_wait_prior(2)` 允许最多 2 个 pending commit。

### 步骤 5 — 测量 2-stage 吞吐

`[H3-T22]` (main.cu:241) TODO [必做] 步骤 5：测量 2-stage pipeline。

应相比 H2 提升 15-40%（取决于 global memory latency 与计算强度平衡）。

### 步骤 6 — Nsight Compute 观察 stall 减少

`[H3-T29]` (main.cu:292) TODO [必做] 步骤 6：Nsight Compute 观察 “Smem/Dmem Pipeline stall”。

```bash
ncu --metrics l1tex__t_sectors_pipe_lsu_mem_global_op_ld.sum \
    --metrics smsp__warp_issue_stalled_long_scoreboard_per_warp_active.pct \
    ./H3_gemm_pipelined_double_buffer
```

## 6. 进阶任务

`[H3-T30]` (main.cu:298) TODO [进阶] 实现三层 pipeline 并进一步重叠。
`[H3-T31]` (main.cu:302) TODO [进阶] 对比 cp.async 与 cuda::memcpy_async 性能差异。

- 实现三层 pipeline，进一步重叠计算与搬运
- 对比 `__pipeline_*` 底层 API 与 `cuda::pipeline` 高级 API 的性能差异
- 对比 `cp.async` 与 `cudaMemcpy2DAsync` 的性能差异

## 7. 验收标准

- kernel 编译通过，结果与 H2 逐元素一致
- TFLOPS 相比 H2 提升 15-40%
- Nsight Compute 显示 memory pipeline stall 相比 H2 显著下降
- stage 数 = 3 的版本比 stage 数 = 2 略优或性能相近

## 8. 常见坑

| 坑 | 说明 |
|----|------|
| buffer swap 错误 | 多个 stage 写同一 buffer，结果错误 |
| `__pipeline_wait_prior()` 位置 | 必须在使用 smem 数据前等待，位置错误导致数据竞争 |
| 末尾 flush 遗漏 | 最后一个 tile 加载完但无下一 tile，需要额外 flush |
| cp.async 对齐 | `__pipeline_memcpy_async` 源/目地址需要 4/8/16 字节对齐 |
| smem 占用过高 | NUM_STAGES × 2 buffer × BM × BK × 2B 超限导致 occupancy 骤降 |

## 9. 观察点

- double-buffering 让 global memory 延迟被隐藏（latency hiding），计算与加载并行
- stage 数过多会导致 smem 占用增加，occupancy 下降，收益递减
- 在 memory-bound kernel 中，pipeline 收益最大；在 compute-bound 中效果有限

## 10. 复盘问题

1. double-buffering 如何隐藏 global memory 延迟？
2. stage 数 = 2 vs 3，为什么后者不一定总是更快？
3. 如果计算时间（H2 warp tile 执行时间）短于加载时间，pipeline 还有效吗？

## 参考资料

- CUDA C++ Programming Guide Section 3.2.5: “Asynchronous Warp-Level Primitives”
- CUDA C++ libcxx documentation: `<cuda/pipeline>` and `<cuda/barrier>`
- CUTLASS 3.x examples `examples/` 中包含 pipelining 的 Hopper GEMM

## 输出对照（printf / std::puts 原文）

- `[H3-T17]` (main.cu:154) CPU 参考实现。
- `[H3-T18]` (main.cu:191) `main` 入口。
- `[H3-T19]` (main.cu:201) 原文：`问题规模：M=%d  N=%d  K=%d` → 现：`Problem size: M=%d  N=%d  K=%d`
- `[H3-T20]` (main.cu:211) 原文：`正在计算 CPU 参考（可能需要数十秒）...` → 现：`Computing CPU reference (may take tens of seconds)...`
- `[H3-T21]` (main.cu:228) 无 pipeline 基线。
- `[H3-T24]` (main.cu:267) 正确性检查。
- `[H3-T25]` (main.cu:269) 原文：`── 正确性检查（stub 阶段预期 FAIL）──` → 现：`-- Correctness check (FAIL expected at stub stage) --`
- `[H3-T26]` (main.cu:274) 性能汇总。
- `[H3-T27]` (main.cu:278) 原文：`── 性能汇总 ──...` → 现：`-- Performance summary --`
- `[H3-T28]` (main.cu:288) 原文：`2-stage 加速比: %.2fx  3-stage 加速比: %.2fx` → 现：`2-stage speedup: %.2fx  3-stage speedup: %.2fx`
- `[H3-T32]` (main.cu:309) 原文：`[H3] 完成。` → 现：`[H3] done.`
