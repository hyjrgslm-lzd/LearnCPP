# 练习 L10：常量求值工具

先读 `../../chapters/10-constant-evaluation.md`。只编辑 `src/student/constexpr_tools.hpp`。

## Part 1：编译期字符串解析

实现 `fixed_string` 和 `decimal_value<"123">`。只处理非空 ASCII 非负十进制 `int`，允许前导零。

解析：字符串字面量通过结构化 NTTP 进入模板实参；解析由 `consteval` helper 完成，所以 `decimal_value<"12x">`、`decimal_value<"">`、`decimal_value<"12\0x">`、带符号/空白输入和超过 `INT_MAX` 的输入都应在编译期报出可解释诊断。diagnostic case 会先编译合法 control，再编译非法 subject。

## Part 2：临时 constexpr 容器

实现 `constexpr_vector_sum({1,2,3})`，内部可以用 `std::vector<int>` 临时累加并返回 `int`。

解析：常量求值期间的动态存储必须在求值结束前释放。返回拥有值可以；返回内部缓冲区指针不行。

## Part 3：`if consteval`

实现 `phase_value(x)`：常量求值分支返回 `x + 1`，运行时分支返回 `x + 2`。

解析：这证明同一个 `constexpr` 函数有两种执行上下文。`constexpr int y = phase_value(10)` 是 manifestly constant-evaluated；`int x = 10; phase_value(x)` 是运行时调用，即使优化器可能折叠它。这里用 `if consteval`，避免把 `std::is_constant_evaluated()` 在试探性常量初始化中的允许结果写成唯一输出。
