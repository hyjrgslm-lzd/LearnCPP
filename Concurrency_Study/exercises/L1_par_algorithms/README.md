# 练习 L-1：并行算法与执行策略

> 详尽版见 `../../15-模块L-并行算法与执行策略.md` 的 练习 L-1。

## 目标

把串行的 `std::for_each` / `std::transform` / `std::sort` 加上执行策略（execution policy）`std::execution::par` 升级为并行，验证并行结果与串行**完全一致**并做计时对比。讲清四种策略的语义与 `par_unseq`/`unseq` 的约束。

## 前置理解

- 执行策略在 `<execution>`，作为算法的**第一个实参**传入：
  - `std::execution::seq`（C++17）串行、不向量化；
  - `std::execution::par`（C++17）多线程并行；
  - `std::execution::par_unseq`（C++17）多线程并行 + 允许向量化（SIMD）；
  - `std::execution::unseq`（**C++20**）单线程但允许向量化。
- **约束**：`par_unseq` / `unseq` 下元素函数体内**禁止加锁、禁止分配内存、禁止相邻元素调用相互依赖**（允许交错执行，interleaving），否则未定义行为；`par` 不向量化，单个元素调用是完整的，约束较松（但仍不能有数据竞争）。
- **工具链差异**：MSVC 并行算法**内置完整支持、无需链接任何库**；GCC/libstdc++ 历史上把并行后端委托 Intel TBB，需 `-ltbb` 才真正并行（否则退化串行）。本仓库 CMake 用 `TbbSetup` 守卫非 MSVC 平台。

## 必做任务

1. `// TODO [必做 1]`：把串行 `std::transform` 改成 `std::execution::par` 版本，验证 `out_seq == out_par`（确定性映射，逐元素相等）+ 计时。
2. `// TODO [必做 2]`：把 `std::sort` 改成 `std::execution::par` 版本，验证 `v_seq == v_par`（排序结果唯一）+ 计时。

## 验收点

- 并行 `transform` / `for_each` / `sort` 的结果与串行**逐元素一致**。
- 能说清 `seq`/`par`/`par_unseq`/`unseq` 的语义差异，以及 `par_unseq`/`unseq` 为何禁止加锁/分配。
- 能说出 MSVC 内置 vs GCC 需 TBB 的差异。

## 对应官方参考

- cppreference [`std::execution` 策略](https://en.cppreference.com/w/cpp/algorithm/execution_policy_tag_t) / [`for_each`](https://en.cppreference.com/w/cpp/algorithm/for_each) / [`transform`](https://en.cppreference.com/w/cpp/algorithm/transform) / [`sort`](https://en.cppreference.com/w/cpp/algorithm/sort)
- 《C++ Concurrency in Action, 2nd ed.》(Williams) 第 10 章

## 构建运行

```bash
cmake --build build-vs2026 --target L1_par_algorithms --config Release
./build-vs2026/L1_par_algorithms/Release/L1_par_algorithms.exe
```
