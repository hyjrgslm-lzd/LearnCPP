# L01: 稳定归约

实现 `student::stable_sum(std::span<const double>)`。

输入是有限 `double` 序列。空输入返回 `0.0`。本练习用 `1e16, 1, -1e16` 和多组三元组制造抵消；解析真值由构造本身给出，不靠另一个浮点实现当标准答案。

- `student/solution.hpp`：起点调用 naive 左折，会失败。
- `reference/solution.hpp`：调用公共 `c13::neumaier_sum`。
- `good/solution.hpp`：独立写 Neumaier 补偿，不调用 Reference。
- `bad/solution.hpp`：故意左折，检查器必须拒绝。

要点：Kahan 对常见小量丢失有帮助，但 `1e16, 1, -1e16` 这组顺序下仍会返回 `0`；Neumaier 在新项幅度大于当前和时把旧和补回，能保留这个小量。MSVC 上 `long double` 与 `double` 同精度，不能把它当更高精度真值。
