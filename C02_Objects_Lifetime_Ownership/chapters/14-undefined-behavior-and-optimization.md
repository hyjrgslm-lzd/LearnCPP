# 14 未定义行为、诊断与优化

C++ 的性能来自一个交换：标准给实现很大的优化空间，程序员负责不越过语义边界。你不能把“我测了一次没崩”当成正确性证据，也不能把“编译器没报警”当成合法性证明。本章给每类行为建立可审查的判断方法。

## 行为分类

| 类别 | 标准含义 | 编译器是否必须诊断 | 课程处理方式 |
| --- | --- | --- | --- |
| well-defined | 标准规定结果 | 不需要报错 | 可运行，可写 checker |
| implementation-defined | 实现必须选择并记录一种行为 | 不一定报警，但需文档化选择 | 记录工具链/ABI，避免跨平台断言 |
| unspecified | 标准允许多个结果，实现不必固定 | 不要求报警 | 不写唯一结果断言 |
| ill-formed | 程序不符合语法或语义规则 | 通常要求诊断；实现可能诊断后作为扩展继续接受 | 用 compile-fail 或“诊断后接受”分类 |
| IFNDR | ill-formed no diagnostic required | 不要求诊断 | 通过代码审查和结构约束避免 |
| undefined behavior | 标准不再约束执行 | 不要求诊断 | 默认不运行真实 UB |
| erroneous behavior | C++26 诊断模型中的错误行为类别 | 允许/鼓励诊断，语义不同于 UB | 只用 compile-only 或安全模型讲解 |

这张表比术语本身更重要。每次写实验都先问：我证明的是标准行为、当前实现选择、编译器诊断，还是某个工具的一次运行现象？

## UB：优化器为什么可以删代码

未定义行为发生后，标准不约束程序。优化器不需要生成“尽量合理”的机器码；它可以假设 UB 路径不会发生。

```cpp
int read_then_check(int* p) {
    int x = *p;
    if (p == nullptr) {
        return 0;
    }
    return x;
}
```

这个函数先解引用 `p`。若 `p == nullptr`，UB 已经发生。优化器可以据此认为后面的空指针分支不可达，从而删除检查。把检查放到前面才是正确程序：

```cpp
int check_then_read(int* p) {
    if (p == nullptr) {
        return 0;
    }
    return *p;
}
```

同类问题还包括：越界访问后再检查下标、signed overflow 后再判断范围、对象销毁后再检查指针是否为空。检查必须发生在危险操作之前。

## IFNDR：没有诊断不表示合法

IFNDR 常见于 ODR 和模板。两个翻译单元里对同一个 inline 函数、类模板或常量给出不一致定义，链接器可能无法发现；程序仍不合法。

```cpp
// a.cpp
inline int scale() { return 2; }

// b.cpp
inline int scale() { return 3; } // 若属于同一程序的同一实体定义，违反 ODR
```

课程不依赖“编译器能不能抓住 IFNDR”。正确策略是减少重复定义来源，用头文件或模块集中定义实体，并让 checker 检查学生提交的真实对象行为，而不是让学生回填一份报告声称自己符合。

## unspecified：不要硬断执行顺序

unspecified behavior 不是 UB。程序仍合法，但标准允许多个结果。例如函数实参求值顺序在一些场景中不固定。下面的代码不适合作为 checker 唯一输出：

```cpp
int i = 0;
auto f = [](int, int) {};
f(++i, ++i); // 哪个实参先求值不应写成唯一教材结论
```

教学上要把它改成显式顺序：

```cpp
int first = ++i;
int second = ++i;
f(first, second);
```

checker 只断言显式顺序版本，不拿 unspecified 行为制造“某编译器输出”。

## implementation-defined：可绑定平台，但要写清

implementation-defined behavior 允许项目绑定实现。例如 `char` 是否 signed、`std::size_t` 的宽度、对象 ABI 布局、异常处理 ABI 等。实现必须文档化选择。

不要把旧经验机械套到当前基线。例如 signed integer 的右移在 C++20 起已经规定为向负无穷舍入的算术右移模型；本课 C++23 baseline 下不再把“signed 右移策略”列为 implementation-defined 示例。若你写的是跨语言或老标准迁移说明，必须标注目标标准版本。

在本课程里，MSVC 19.51 / VS 18 生成器和 Clang 22.1.3 + 当前 MSVC STL 是实测工具链。我们可以记录 `_MSC_VER`、`_MSVC_LANG`、feature-test macro 和本机 `sizeof`。这些记录说明“本机如何表现”，不升级为 C++ 标准保证。

## C++26 erroneous behavior

C++26 引入 erroneous behavior 用于描述一类错误值相关行为，并给诊断更多标准位置。它不是“未初始化读取现在安全”，也不是“所有 UB 都变成 erroneous”。

课程处理规则：

- 对错误初始化、返回临时引用等前沿主题，用 compile-only、warning 观测或安全模型讲解。
- 不通过读取未初始化对象、解引用悬空指针、或执行越界访问来“证明”标准语义。
- 若工具链只给 warning 或没有诊断，报告为“当前工具未诊断/未完全支持”，不推断程序合法。

示例模型：

```cpp
int maybe_erroneous_model(bool initialized) {
    if (!initialized) {
        return -1; // 用显式状态模拟错误路径，不读取未初始化 int
    }
    int x = 42;
    return x;
}
```

这段代码不是标准 erroneous behavior 的实现，它是课堂安全替身，用来讨论诊断边界和状态传播。

## 前沿 compile-only 证据

`CORE_STUDY_ENABLE_FRONTIER` 启用后，L14 会构建固定源 probe。它们使用当前 CMake generator 和 `CMAKE_CXX_COMPILER`，不调用作者机器上的硬编码编译脚本。

baseline probe 是普通 aggregate designated initializer 和 `bit_cast`。它必须编译；若失败，说明基础工具链或配置失败，不能标为 SKIP。

P2287R6 probe 使用 `Derived{.base = 1, .member = 2}`，覆盖“基类的间接成员可被 designator 初始化”和“特定 base 前缀混合”的方向。它不使用基类名作 designator，因为该论文方向不支持 `.Base = ...` 这种形状。当前编译器若不支持，测试输出 capability SKIP；若编译成功，才记录为该能力 PASS。

P2748 return temporary reference probe 使用 `const int& f() { return 42; }` 作为应拒绝形状。若编译器给诊断但 exit 0，报告为“诊断后接受”，不能据此证明 C++26 语义已执行。若编译器非零退出，记录为拒绝能力 PASS。

P2953 defaulted assignment restriction probe 使用 ref-qualified defaulted assignment 的限制形状。当前工具链若接受，记录为 capability SKIP；若拒绝，才记录为限制已执行。pointer provenance、invalid pointer、lifetime-end 相关 DR 没有可靠 runtime 判据，本课只提供 compile-only review model，强调旧指针和整数地址不能复活对象访问权。

## as-if 规则的边界

as-if 规则允许实现做任何不改变可观察行为的转换。可观察行为包括 volatile 访问、I/O、程序终止等标准定义的观察点。对合法程序，优化后结果要等价。对含 UB 的程序，优化器可以从“不发生 UB”出发推导。

```cpp
int sum_positive(int* p, int n) {
    int sum = 0;
    for (int i = 0; i <= n; ++i) { // 若调用者只给 n 个元素，i == n 越界
        sum += p[i];
    }
    return sum;
}
```

这里错误不在优化器，而在循环边界。`<=` 让代码要求 `n + 1` 个元素。若调用者只传 `[0, n)`，最后一次访问 UB。优化器之后的任何奇怪结果都不是可移植事实。

## 诊断工具证明什么

ASan、UBSan、编译器 warning 和静态分析都很有用，但它们证明的东西不同：

- ASan 擅长检测越界、use-after-free、部分 stack/use-after-scope；不覆盖全部 C++ 生命期和别名规则。
- UBSan 能检查一些整数、对齐、vptr、bounds 等问题；不同编译器支持集不同。
- warning 是诊断信号，不是标准解释器。warning 消失不证明无 UB。
- 静态分析能找到路径问题，但也会有误报和漏报。

因此证据报告必须写清“程序是否启动”“测试是否运行”“诊断来自哪一层”。本课保留原始 JSON，避免只看 exit code。若 wrapper 命令拼错导致 exe 没启动，不能把 expected exit 当作 runtime PASS 或 FAIL。

## 本章正反例

正例：把危险前提变成显式检查。

```cpp
int at_or_zero(const int* data, std::size_t size, std::size_t index) {
    if (index >= size) {
        return 0;
    }
    return data[index];
}
```

反例：先访问后检查。

```cpp
int late_check(const int* data, std::size_t size, std::size_t index) {
    int value = data[index];
    if (index >= size) {
        return 0;
    }
    return value;
}
```

`late_check` 即使在某次 Debug 运行中返回 0，也不是合法实现。第 15 章的 `object_buffer<T>` 用同样原则：先判断容量和溢出，再分配；先完成可能抛的构造，再提交 `size_` 或 `capacity_`。

## 本章实验怎么读

`L14_ub` observation 运行安全分类和不触发 UB 的 as-if 观察。真实悬空引用、未初始化读取、越界解引用不在默认 target 中执行。C++26/29 前沿项在探针矩阵里记录为 compile-only 或 unsupported；没有可靠直接运行判据的语义，不编造成运行支持。
