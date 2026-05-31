# 练习 L-3：并行 vs 串行基准

> 详尽版见 `../../15-模块L-并行算法与执行策略.md` 的 练习 L-3。

## 目标

系统对比 `seq` vs `par` vs `par_unseq` 在不同数据规模下的耗时，打印加速比表，并用实测证据讲清**何时并行才真正加速**（数据量、每元素工作量、内存带宽瓶颈、并行开销），给出「小数据并行更慢」的实测证据。

## 前置理解

- 何时并行**才真加速**：
  - **数据量太小** → 线程启动/任务分发/归约合并的**固定开销**比计算本身还贵，并行**更慢**；
  - **每元素工作量太轻**（如纯加法）→ 受**内存带宽（memory bandwidth）**而非 CPU 限制，多核抢同一条总线，收益有限甚至为负；
  - **数据量大 + 每元素计算重（compute-bound）** → 并行才接近线性加速。
- **基准方法学**：每配置多跑取**最小值**（min，最接近无干扰真实耗时）；先 **warm-up 预热**（线程池/页缓存就绪）；用 `volatile` sink **防优化**（否则结果没人用会被整段优化掉）。
- 基准结果**高度依赖机器**（核数、缓存、内存带宽），且**必须在 Release/优化下**观察趋势——Debug 下并行往往更慢。

## 必做任务

1. `// TODO [必做 1]`：在 `run_scale` 中补上 `par` 与 `par_unseq` 两种策略的测量（`bench_min_ms` 取最小耗时），与 `seq` 比得加速比。`heavy()` 是纯函数，满足 `par_unseq` 的交错约束。
2. 主函数扫描多个规模（1e3 → 8e6）打印「规模 × 策略 × 加速比」表，在小规模行复现「并行更慢」（加速比 < 1）。

## 验收点

- 输出的表里，**小规模行**加速比通常 < 1（并行更慢），**大规模行**加速比 > 1（并行明显加速）。
- 能据表说清「小数据并行更慢」的根因（固定开销 > 收益）。
- 能说清并行真加速的条件：数据量足够大 + 每元素工作量足够重（compute-bound），并避开内存带宽瓶颈。

## 对应官方参考

- cppreference [`std::execution` 策略](https://en.cppreference.com/w/cpp/algorithm/execution_policy_tag_t) / [`transform`](https://en.cppreference.com/w/cpp/algorithm/transform)
- 《C++ Concurrency in Action, 2nd ed.》(Williams) 第 10 章

## 构建运行

```bash
cmake --build build-vs2026 --target L3_par_vs_seq_bench --config Release
./build-vs2026/L3_par_vs_seq_bench/Release/L3_par_vs_seq_bench.exe
```
