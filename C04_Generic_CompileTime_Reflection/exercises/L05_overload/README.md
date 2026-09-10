# 练习 L05：重载、偏序与 SFINAE

先阅读 [05. 重载决议、偏序与 SFINAE](../../chapters/05-overload-sfinae.md)。本题只编辑 `src/student/describe.hpp`。

## 文件位置

- 学生编辑：`src/student/describe.hpp`
- Reference：`src/reference/describe.hpp`
- 正确控制：`validation/good/describe.hpp`
- 错误控制：`validation/bad/describe.hpp`
- checker：`checks/describe_checks.cpp`
- 观察程序：`observations/overload_observation.cpp`
- 编译诊断：`validation/diagnostics/function_partial_specialization*.cpp`、`validation/diagnostics/sfinae_*.cpp`

## Part 1：成员自描述

实现 `c04_overload::describe(object)`。若对象有零参数 `describe()` 成员，返回 `member_result`，并实际调用该成员。checker 会检查对象里的计数器，不能只按类型返回标签。

已提供 `MemberDescribed` 和同时也是 range 的 `BothMemberAndRange`。学生任务是把成员路径放在最优先的可行候选里，并把成员调用结果带入 `member_result`。

## Part 2：字符串字面量

字符串字面量要匹配数组引用，返回 `literal_result<N>`。`"abc"` 的 `N` 是 4，包含结尾的 `'\0'`。不要把它退化成 `const char*`。

已提供 bad 控制实现，它只接受 `const char*`。checker 用 `decltype(c04_overload::describe("abc"))` 检查必须是 `literal_result<4>`，并检查 `extent == 4`。学生任务是写数组引用 overload，而不是运行时数长度。

## Part 3：整数与 range

整数但不是 `bool` 时返回 `integral_result`。满足 range 且前面几类不成立时返回 `range_result`。无合法分类时，`requires { c04_overload::describe(object); }` 为 `false`。

已提供 `int`、`bool`、`std::vector<int>` 和 no-description 类型。checker 验证 bool 不走 integral，普通 range 只作为 fallback。学生任务是把排除条件写在约束上，避免函数体里才发现分类不成立。

## Part 4：观察与诊断

`overload_observation.cpp` 展示模板 exact match 可以胜过需要转换的非模板候选、两个函数模板如何通过偏序选择 `const T*` 版本、类模板偏特化能表达指针分类、requires 可把缺失表达式留在立即上下文。`function_partial_specialization_control.cpp` 用函数模板重载表达指针分支；`function_partial_specialization.cpp` 故意写非法函数模板偏特化，预期匹配 MSVC `error C2768` 或 `error C2912`。`sfinae_immediate_context_control.cpp` 把 `size()` 检查放在 trailing return type 里并落到 fallback；`sfinae_body_hard_error.cpp` 把同一个检查放进函数体并实际实例化，预期匹配 MSVC `error C2039`。

## 验证入口

Reference/good 应通过；bad 控制实现故意把字面量当指针，被 checker 以 `string literal overload must keep the array extent` 拒绝。诊断 case 展示函数模板不能偏特化，control 用函数模板重载完成相同意图。
