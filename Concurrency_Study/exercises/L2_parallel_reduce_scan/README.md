# 练习 L-2：并行归约与扫描

> 详尽版见 `../../15-模块L-并行算法与执行策略.md` 的 练习 L-2。

## 目标

掌握并行归约（reduction）`std::reduce` / `std::transform_reduce` 与并行扫描（scan）`std::inclusive_scan` / `std::exclusive_scan`。并行求和、点积、前缀和，结果对照串行（整型精确相等、浮点用容差）。讲清为何 `reduce` 要求结合律、与 `accumulate` 的区别。

## 前置理解

- **`std::reduce`**（`<numeric>`）：并行求和/聚合。把区间切块、各块各算、再合并；**不保证求值顺序与分组**，因而**可并行**，但要求二元运算满足**结合律（associativity）**（常还需交换律）。
- **`std::accumulate`**：严格**从左到右**顺序累加，顺序确定但**天然串行**（不接受执行策略）。
- **`std::transform_reduce`**：先逐元素映射、再归约的合体，做**点积 / 范数**最顺手。
- **`std::inclusive_scan`**（含当前元素的前缀和）/ **`std::exclusive_scan`**（不含当前元素，首元素为 `init`）：并行前缀和，同样要求结合律。
- **浮点警告**：浮点加法**不满足结合律**（舍入误差），并行分组顺序变化会让浮点结果**产生微小差异**——浮点对照串行**必须用容差比较**，不能 `==`。

## 必做任务

1. `// TODO [必做 1]`：用 `std::reduce(par, ..., 0LL)` 并行求和，整型与 `accumulate` 结果**精确相等**（且等于理论值 `N(N+1)/2`）。
2. `// TODO [必做 2]`：用 `std::transform_reduce(par, a, b, 0.0)` 并行点积（浮点），与串行 `inner_product` 在**容差**内一致。
3. 第三部分演示 `inclusive_scan` / `exclusive_scan`，整型与 `partial_sum` 逐元素对照，并验证 `inclusive[i] - exclusive[i] == a[i]`。

## 验收点

- 整型 `reduce` == `accumulate`；浮点 `transform_reduce` 与串行在容差内一致。
- `inclusive_scan` 与串行 `partial_sum` 逐元素相等。
- 能说清 `reduce` 为何要求结合律（+交换律）、与 `accumulate` 的区别、浮点为何需容差。

## 对应官方参考

- cppreference [`reduce`](https://en.cppreference.com/w/cpp/algorithm/reduce) / [`transform_reduce`](https://en.cppreference.com/w/cpp/algorithm/transform_reduce) / [`inclusive_scan`](https://en.cppreference.com/w/cpp/algorithm/inclusive_scan) / [`exclusive_scan`](https://en.cppreference.com/w/cpp/algorithm/exclusive_scan)
- 《C++ Concurrency in Action, 2nd ed.》(Williams) 第 10 章

## 构建运行

```bash
cmake --build build-vs2026 --target L2_parallel_reduce_scan --config Release
./build-vs2026/L2_parallel_reduce_scan/Release/L2_parallel_reduce_scan.exe
```
