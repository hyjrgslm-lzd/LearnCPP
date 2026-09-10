# 10. `constexpr`、`consteval` 与常量求值边界

优化器把一个表达式折叠成常量，不等于这个表达式发生了语言意义上的常量求值。语言常量求值有自己的入口、对象生命周期和存储限制；优化常量折叠是实现选择，不能让一个本来不能作模板实参的运行时值突然合法。

本章区分 `constexpr`、`consteval`、`if consteval` 和 `std::is_constant_evaluated()`，并用 `vector`/`string` 的临时计算说明：常量求值期间可以分配并释放临时存储，但不能把指向那段暂存的对象持久化到运行时。

## 三个入口

`constexpr` 函数可以在常量求值中调用，也可以在运行时调用。是否常量求值取决于上下文：

```cpp
constexpr int square(int x) { return x * x; }

static_assert(square(4) == 16); // 常量求值
int y = square(read_runtime()); // 运行时调用
```

`consteval` 函数必须立即常量求值。传入运行时值不良构，诊断应停在调用点附近。

`if consteval` 在函数体里分开两条路径。它检查当前执行是否处于常量求值上下文。`std::is_constant_evaluated()` 也能问类似问题，但在普通 `if` 中更容易被试探性常量求值影响；优先用 `if consteval` 表达分支代码。

更精确地说，C++23 按 N4950 的 `expr.const` 模型区分 manifestly constant-evaluated context。典型位置包括 `static_assert` 条件、非类型模板实参、数组边界、`constexpr` 变量初始化、`consteval` immediate invocation 等。普通局部变量初始化即使被优化器折叠，也不是因为语法位置变成了 manifestly constant-evaluated。

immediate invocation 是对 immediate function，也就是 `consteval` 函数的调用。这样的调用必须产生常量表达式，除非调用发生在 immediate function context 中。immediate function context 包括另一个 `consteval` 函数体内，或 `if consteval` 的 true 分支内。这个例子说明边界：

```cpp
consteval int parse_digit(char ch) {
    return ch >= '0' && ch <= '9' ? ch - '0' : throw "not digit";
}

constexpr int wrapper(char ch) {
    if consteval {
        return parse_digit(ch); // immediate function context
    } else {
        return ch - '0';        // runtime path must not call parse_digit(ch)
    }
}
```

`wrapper('7')` 在 `static_assert` 里走 true 分支，可以调用 `parse_digit`。`int x = wrapper(runtime_char);` 走运行时分支，不能把 runtime `ch` 传进 `parse_digit`。所以问题不只是“实参看起来是不是常量”，还要看调用点是否是 immediate invocation，以及它是否位于 immediate function context。

`std::is_constant_evaluated()` 的合法结果范围也要谨慎。标准允许实现为了判断某些初始化能否常量初始化而进行试探性常量求值；在这类试探中，它可能先返回 `true`，若试探失败再放弃。不要写依赖“普通变量初始化时它必为 false”的课程检查。练习里的 `phase_value` 用 `if consteval`，并只在明确的 `constexpr` 初始化和明确的运行时变量调用两个位置检查结果。

## 对象、存储和暂存分配

常量求值不是“没有对象”。局部对象会构造、修改和销毁，生命周期规则仍然适用。C++20 以后，`std::vector` 和 `std::string` 可以在常量求值里临时使用，只要分配的存储在常量求值结束前释放。

```cpp
constexpr int sum_small() {
    std::vector<int> xs{1, 2, 3};
    return std::accumulate(xs.begin(), xs.end(), 0);
}
```

这可以成功，因为返回的是拥有语义明确的 `int`。但不能返回指向 vector 内部缓冲区的指针，也不能把分配存储做成需要跨越常量求值边界的持久对象。语言拒绝的不是“用过 vector”，而是“常量结果包含不允许持久化的存储关系”。

`std::string` 也是同一类例子：

```cpp
constexpr std::size_t name_size() {
    std::string s = "meta";
    s += "data";
    return s.size();
}
```

返回 size 可以；返回 `s.c_str()` 不可以，因为指针会指向常量求值期间的临时存储。若把 `std::vector<int>` 做成 `constexpr` 对象并让它的动态缓冲跨过常量求值边界，也会失败。可持久化的是结果值本身，不是计算过程中分配过的任意存储。

## `constexpr` 容器和算法

标准库越来越多算法和容器可在常量求值里运行。课程使用它们验证规则，但不把某家 STL 的实现细节当标准要求。能不能在本机编译，需要看语言标准、库版本和具体接口；这类能力在规范索引和验证报告里分开记录。

## 可解释诊断

### `consteval` 不把每个局部变量变成模板实参

常量求值解释器执行函数，与编译器检查函数声明/模板实例，是不同的步骤。即使只会传入字面量，这个定义也不合法：

```cpp
consteval int broken(int n) {
    static_assert(n >= 0); // 反例：检查函数定义时，n 不是常量表达式
    return n;
}
```

同样，把循环中的 `ch`、累计中的 `result` 直接放进 `static_assert`，不会因为外层函数是 `consteval` 就成立。普通循环可以参与常量求值，但 `static_assert`、数组类型的大小和模板实参有各自的常量表达式要求。可以先用普通循环完成一轮验证、返回值对象，再把这次调用的结果保存为 `constexpr` 并断言；也可以让位置与累计结果作为模板参数递推。两种组织都要先拒绝非法字符，再检查范围，最后才实例化或执行乘加。

L10 的 Reference 采用后一种组织：`Text`、索引和累计结果都是模板实参，因此每个递归实例都能在声明检查时判断条件。`if constexpr` 使非法分支不再形成下一次乘加实参；仅在乘加前写一条失败的 `static_assert`，不等于已经阻止编译器继续检查后面的表达式。`L10_parameter_constant_diagnostic` 保留上述最小反例与模板参数正例，先证明工具链正常再检查预期拒绝。

好的编译期接口让失败点靠近用户输入。`consteval` 适合“必须是编译期常量”的转换；concept 适合“这个类型不满足调用条件”；`static_assert` 适合给语义约束命名。

C++26 的 constexpr exceptions、placement new 等会在 F01 前沿章节继续讲。这里先把 C++23 的模型讲完整：常量求值可以执行相当多的普通代码，但最终结果必须能作为常量表达式保存；对象生命周期和存储边界不能被优化器绕过。

本课固定规范索引把 C++23、C++26、C++29 和 DR 分开。C++26/DR 中 consteval-only value、constexpr exceptions、placement new 的细节会影响反射章节的 `std::meta::info`、`define_static_*` 等例子，但 L10 的基础练习只要求 C++23 可验证模型。后文引用 L10 时，只引用“立即函数调用、常量求值上下文、对象与存储边界”这条主线，不把前沿规则反向改写成本章已验证事实。

## 练习与反例

L10 要实现三个小工具：`decimal_value<fixed_string>` 在编译期解析非空 ASCII 非负十进制 `int`；`constexpr_vector_sum` 可在常量求值里使用 `vector` 临时计算；`phase_value` 用 `if consteval` 区分常量和运行时调用路径。解析器按字符串字面量的数组长度完整消费 `N - 1` 个字符，所以空串、内嵌 NUL、符号、空白、非数字和超过 `INT_MAX` 的输入都不是合法缩写。

bad 控制实现把运行时路径也写成常量路径，checker 会以 `if consteval branch must distinguish runtime calls` 拒绝。观察程序展示同一个 `constexpr` 函数既能用于 `static_assert`，也能在运行时接收变量。隔离 diagnostic case 先编译合法数字，再分别编译非法数字、空串、内嵌 NUL、符号、空白和溢出输入，要求诊断停在 `decimal_value` 的编译期输入边界。
