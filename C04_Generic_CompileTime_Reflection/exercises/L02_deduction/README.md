# 练习 L02：推导探针

先读 [02. 推导规则](../../chapters/02-deduction.md)。本题只编辑 `src/student/deduction_probe.hpp`。

实现 `type_token<T>` 和几个返回 token 的函数。checker 用返回类型验证推导规则，不看字符串。

Part 1：`by_value_token(T)` 返回 `type_token<T>`。按值推导应让 `const int` 变成 `int`，数组变成指针。

Part 2：`by_ref_token(T&)` 返回 `type_token<T>`。左值引用形参应保留底层 `const`。

Part 3：`array_ref_token(T (&)[N])` 返回 `array_token<T, N>`，保留元素类型和数组长度。

Part 4：`identity_decltype_auto(T&&)` 返回 `decltype(auto)` 并用 `std::forward<T>` 保留引用身份和值类别。

Part 5：`freeze_as<T>(std::type_identity_t<T>)` 使用非推导上下文，调用方必须显式写出 `T`。这题只检查显式指定后的转换结果。

`validation/bad` 故意让数组引用退化，checker 必须拒绝。observation 单独演示 `decltype(name)` 与 `decltype((expr))` 的差别。


## IDE 入口

从本课 `exercises` 根目录或本题目录生成 Visual Studio 18 2026 x64 工程。主项目是 `L02_deduction_student`；默认学生测试关闭时仍生成该项目，但它是 `EXCLUDE_FROM_ALL`，需显式构建。
学生只编辑：`src/student/deduction_probe.hpp`。 `checks/`、`validation/`、`src/reference/`、diagnostic、support 目标是只读对照/验证/实验入口，保留在题目分组内。
修改 Student 后先重新构建对应目标，再运行 CTest 或 README 中列出的检查命令。
