# 练习 K-3：SIMD select / where 掩码

> 详尽版见 `../../14-模块K-数据并行与std-simd.md` 的 练习 K-3。

## 目标

解决 SIMD 里的“if 怎么办”：向量一次处理 `W` 条 lane，但每条 lane 的条件可能不同，CPU 无法对一个向量“分叉跳转”。改用**分支无关（branch-free）**做法——比较得**掩码（mask）**、用 **select（条件选择）** 按掩码逐 lane 挑值。用 `abs` / `clamp` / `relu` 三种条件运算练手，并与标量对照。

## 前置理解

- **比较得掩码**：`va < vb` 得到 `xsimd::batch_bool<float>`（每 lane 一个 bool）。
- **select 选值**：`xsimd::select(mask, x, y)` 逐 lane——mask 真取 `x`、假取 `y`。本质是 then/else **两边都算**再按掩码挑，没有真正跳转，因此**无分支预测失败**，对随机条件常比标量 `if` 更快。
- **min/max** 本身就是分支无关的逐 lane 运算，clamp 用 `min(max(x,lo),hi)` 即可。
- 标准目标：C++26 `std::simd` 用 **where 表达式 / `std::simd::select`** + `basic_mask<T>`（别名 `mask<T,N>`）。本题用 `batch_bool` + `xsimd::select` 等价实现。
- select/min/max 不改变数值，故对照可**严格相等**（与 K-2 的容差不同）。

## 必做任务

1. `// TODO [必做 1]`：`abs` = `select(x<0, -x, x)`。
2. `// TODO [必做 2]`：`clamp` 到 `[lo,hi]` = `min(max(x,lo),hi)`。
3. `// TODO [必做 3]`：`relu` 条件赋值 = `select(x>0, x, 0)`；三者均与标量逐元素对照、计时。

## 验收点

- 三种条件运算的 SIMD 结果与标量结果逐元素完全一致（不一致数 = 0）。
- 能说清“为什么 SIMD 不能逐 lane 跳转”，以及“掩码 + select 如何替代 if”。
- 能把 `batch_bool` ↔ `basic_mask`、`select` ↔ `where/select` 对应起来。

## 对应官方参考

- 提案 [`P1928R15`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p1928r15.pdf)（`where` / `select` / `mask`）
- cppreference [`<simd>`](https://en.cppreference.com/w/cpp/header/simd)
- [xsimd select 文档](https://xsimd.readthedocs.io/en/latest/api/cond_index.html)

## 构建运行

```bash
cmake --build build-vs2026 --target K3_simd_where_select --config Release
./build-vs2026/K3_simd_where_select/Release/K3_simd_where_select.exe
```
