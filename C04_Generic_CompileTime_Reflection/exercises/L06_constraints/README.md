# 练习 L06：Concepts 与约束边界

先阅读 [06. requires、Concepts 与约束偏序](../../chapters/06-constraints.md)。本题只编辑 `src/student/record_range.hpp`。

## 文件位置

- 学生编辑：`src/student/record_range.hpp`
- Reference：`src/reference/record_range.hpp`
- 正确控制：`validation/good/record_range.hpp`
- 错误控制：`validation/bad/record_range.hpp`
- checker：`checks/record_range_checks.cpp`
- 观察程序：`observations/constraints_observation.cpp`
- 编译诊断：`validation/diagnostics/non_dependent_error*.cpp`

## Part 1：字段形状

实现 `field_like<T>`。它要求对象有 `name` 和 `value`，其中 `name` 可转换成 `std::string_view`，`value` 是整数类型。使用 compound requirement 和标准 concept 表达约束。

已提供 `Field`、`MissingName` 和 `NonIntegralValue`。checker 直接检查 concept 真假。学生任务是把 `name` 与 `value` 的形状放在 requires 表达式里，不靠函数体读取时报错。

## Part 2：稳定字段 range

实现 `stable_field_range<R>`。它要求 `std::ranges::forward_range<R>`，并且 `std::ranges::range_reference_t<R>` 满足字段形状。不要只写 `input_range`；本接口后续会多遍遍历字段，单遍 range 不满足语义契约。

已提供 `std::vector<Field>`、`std::array<Field, N>` 和自定义 `TokenStream` 单遍 range。checker 证明 vector/array 可重复计数，`TokenStream` 是 `input_range` 但不是 `forward_range`。bad 控制实现放宽到 `input_range`，应被拒绝。

## Part 3：受约束入口

实现 `c04_constraints::field_count(range)`，只对 `stable_field_range` 可调用，遍历字段并返回数量。遍历时实际读取 `name` 和 `value`，避免约束和函数体脱节。

checker 用 `countable` concept 检查无效字段 range 和单遍 range 不可调用，再对正例运行两次计数。学生任务是让公开入口和 concept 使用同一约束，不额外开放未验证路径。

## Part 4：观察与诊断

`constraints_observation.cpp` 展示 simple/type/compound/nested requirement，并用相同形参列表、复用同一 concept 原子的 overload 展示更受约束候选，避免把结果误归因到函数模板参数偏序。`non_dependent_error_control.cpp` 是依赖表达式正例；`non_dependent_error.cpp` 故意引用定义点不可见的 `missing_global_name`，预期匹配 MSVC `error C3861`。

## 验证入口

Reference/good 应通过；bad 控制实现故意把 `stable_field_range` 放宽到 `input_range`，会被 checker 以 `single-pass range must not satisfy stable_field_range` 拒绝。诊断 case 展示 non-dependent 名字错误不会被 `requires` 变成 false。
