# 02 浮点误差与稳定归约

先修：C02 的值与对象、C06 的顺序算法、本课第 01 章的测量边界。目标不是把浮点数讲成“都不准”，而是让读者能在改写循环、并行化或向量化之前，先说清结果允许偏离多少，哪些输入会放大误差，检查器应该比较什么。

配套 [L01 练习](../exercises/L01_stable_reduction/README.md) 和 [公共实现](../exercises/include/c13/numerics.hpp) 使用同一组接口：`naive_sum`、`pairwise_sum`、`kahan_sum`、`neumaier_sum`；综合项目复用 `c13::neumaier_sum(std::span<const double>) -> double`。

## 有限集合里的实数近似

`double` 不是实数。它只表示有限个二进制数；两个有限值相加时，精确数学结果通常还要舍入回某个可表示值。只要一次运算发生舍入，后续运算读到的就是舍入后的数，而不是原来的数学结果。

这件事在大数与小数相加时很明显。`1.0e16 + 1.0` 的数学结果比 `1.0e16` 大 1，但在常见 IEEE 754 double 中，这个 1 小到无法改变当前可表示值，运算结果仍可能是 `1.0e16`。如果下一步再减去 `1.0e16`，小量已经丢了：

```cpp
double x = 1.0e16;
double y = x + 1.0;
double z = y - 1.0e16; // 常见结果是 0，不是 1
```

这不是编译器 bug，也不是优化级别造成的随机行为。它来自格式容量和舍入规则。允许重结合的快速浮点选项以及手写并行归约可以改变表达式分组，进而改变舍入发生的位置；严格浮点语义下不能无条件重结合加法，所以性能优化与数值检查不能拆开看。

## naive 左折为什么容易坏

最直接的归约是左折：

```cpp
double sum = 0.0;
for (double x : xs) {
    sum += x;
}
```

它的优点是可读、分配为零、顺序确定。缺点也来自这个顺序：每个新值都加到已经舍入过的 `sum` 上。若输入按 `1e16, 1, -1e16` 排列，小量先被大数吞掉，再也没有机会恢复。

[L01 checks](../exercises/L01_stable_reduction/checks.cpp) 使用 `c13::cancellation_input(8)` 生成八组三元组。每组三元组的解析真值是 1，总和解析真值是 8。检查器比较这个解析真值，不用另一个可能同样错误的浮点实现当答案。

## pairwise 改变误差增长路径

pairwise 归约把序列分半，分别求和，再合并：

```cpp
return pairwise_sum(xs.first(mid)) + pairwise_sum(xs.subspan(mid));
```

它通常比长左折好，因为依赖链变短，大小相近的部分更可能先相加。但它不是“稳定”的同义词。分组仍可能让小量在局部被吞掉；并且递归切分改变了运算顺序，所以结果不应拿 naive 的逐位结果当契约。

pairwise 的教学价值在于提醒读者：优化经常改变顺序。并行归约、SIMD 水平加法、分块矩阵乘里的累加顺序都属于同一类问题。正确性检查要写成“在误差预算内接近独立参考”，而不是“与某个旧实现 bitwise 相同”。

## Kahan 与 Neumaier 的差异

Kahan 求和维护一个补偿项。每次把上一步估计丢掉的低位从新输入里扣回来：

```cpp
const double y = x - compensation;
const double t = sum + y;
compensation = (t - sum) - y;
sum = t;
```

它能修复很多“小值连续加到大和上”的情况，但并不覆盖所有抵消顺序。`1e16, 1, -1e16` 这组输入下，Kahan 仍会得到 0。原因是第三个数幅度很大，新的和从 `1e16` 跳回 0，原有补偿没有以足够的形式保存下来。

Neumaier 是本课选作公共入口的原因：它在每一步比较当前和与新项的幅度。若新项更大，就把旧和也纳入补偿：

```cpp
const double t = sum + x;
if (std::abs(sum) >= std::abs(x)) {
    compensation += (sum - t) + x;
} else {
    compensation += (x - t) + sum;
}
sum = t;
```

对 `1e16, 1, -1e16`，Neumaier 能返回 1。它仍不是任意精度算法，也不会让 overflow、NaN、无限值消失。本练习限定有限输入和小规模解析检查；真实数值库还要按领域定义特殊值、溢出、舍入模式和性能预算。

## long double 不是本课真值来源

有些平台的 `long double` 比 `double` 精度更高，有些不是。MSVC 上 `long double` 与 `double` 使用同一表示。课程检查不能写成“转成 long double 就是真值”，否则在本机不会得到额外精度，还会把平台差异藏起来。

本章的解析真值来自输入构造：每组三元组数学和为 1。后续矩阵和布局实验同样要把“参考答案怎么来”说清楚。若参考答案来自更高精度库、符号推导或小规模手算，应分别说明；不能靠 Reference 与 Student 调用同一个函数来证明正确。

## 练习映射

L01 的 `student/solution.hpp` 起点调用 `naive_sum`，因此会在抵消输入上失败。`reference/solution.hpp` 调用公共 `neumaier_sum`，为综合管线提供统一入口。`good/solution.hpp` 独立写同一补偿算法，不通过调用 Reference 作弊。`bad/solution.hpp` 故意左折，证明检查器能拒绝代表性错误。

通过 L01 后，读者应该能回答三个问题：这组输入的解析真值为什么是 8；Kahan 为什么没有覆盖 `1e16, 1, -1e16`；如果后续优化改变归约顺序，应该先定义什么误差预算再谈速度。
