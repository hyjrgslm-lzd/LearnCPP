# 练习 Capstone3 · 并行计算项目

> 详尽版见 `../../17-第三阶段结课-并行计算项目.md`

## 目标

把一个计算密集核心**按层叠加优化**，每层都重新计时 + 重新校验，综合阶段三 J/K/L：

- **主线 GEMM（矩阵乘）**：朴素三重循环 `v0` → 缓存分块 `v1`（cache blocking，模块 J）→ 并行分块 `v2`（`std::execution::par`，模块 L）。打印「版本 × 耗时 × 加速比 × GFLOPS × 校验」表。
- **对照 1 并行归约**：`std::transform_reduce(par)` 标准版 vs 每线程局部累加 + `alignas(std::hardware_destructive_interference_size)` 防伪共享手写版（模块 J/L）。
- **对照 2 并行排序**：`std::sort` vs `std::sort(par)`，不同规模看加速比（模块 L）。

**零外部依赖**：只用标准库（MSVC 并行算法内置，无需 TBB）。SIMD 靠**自动向量化 + SIMD-friendly 循环**；显式 `std::simd`/xsimd 是**进阶**，指回模块 K。

## 前置理解

- 朴素 GEMM 慢在对 `B` 跨行跳（stride-N）；分块快在让 `BS×BS` 工作集驻留缓存。
- 并行归约要**避免伪共享**（局部累加器各占一条缓存行）、要懂**浮点不满足结合律**（并行求和与串行可能有末位差异，是性质非 bug）。
- 加速比测量**方法学**：预热、多跑取最小、Release/优化、`volatile` sink 防优化。

## 必做任务（对应 `main.cpp` 的 `// TODO [必做 N]`）

1. `gemm_naive` 朴素 i-j-k（基线）；2. `gemm_tiled` 六重分块；3. `gemm_par` 行块 `std::for_each(par,...)`（写不相交行区间→无需锁）；4. `reduce_par_std`（`transform_reduce(par)`）；5. `reduce_par_manual`（多线程 + `alignas` 防伪共享）；6. `sort_par`（`std::sort(par)`）；7. 接入 `bench_min_ms` 基准框架；8. 写逐元素/容差校验。

> 骨架已给可运行实现：`v0`/`v1` 是真实的朴素 + 分块（`v1` 一上来就有可观加速）；而 `v2` 并行、两个归约的并行/多线程、并行排序的占位仍是**串行**，初始加速比 ≈ 1.00x（**符合预期**），换上 `par`/多线程后才拉开。结果均正确（校验全 `OK`）。

## 进阶任务

显式 SIMD（`std::simd`/xsimd，指回模块 K）、`BS` 大小扫描、transpose-B、归约结合律实验、`par` vs `par_unseq`、NUMA/亲和（概念）。

## 验收点

- GEMM 三版校验全 `OK`、能跑出加速比/GFLOPS 表；能解释朴素为何慢、分块为何快、`v2` 为何无需锁。
- 手写归约确实**缓存行对齐**避免伪共享；能解释浮点并行求和的末位差异来自结合律。
- 基准框架正确落实预热/取最小/Release/防优化四项方法学，并能逐条说理由。

## 对应官方参考

- 《C++ Concurrency in Action, 2e》(Williams) 第 10 章（并行算法/执行策略）、第 8 章（8.2.3 伪共享）
- `std::execution`：<https://en.cppreference.com/w/cpp/algorithm/execution_policy_tag_t>
- `std::transform_reduce`：<https://en.cppreference.com/w/cpp/algorithm/transform_reduce>
- `std::hardware_destructive_interference_size`：<https://en.cppreference.com/w/cpp/thread/hardware_destructive_interference_size>

## 编译运行（VS2026, C++20，务必 Release）

```
cmake --build build-vs2026 --target Capstone3_parallel_compute --config Release
./build-vs2026/Capstone3_parallel_compute/Release/Capstone3_parallel_compute.exe
```
