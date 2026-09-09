# 01. 翻译单元、预处理与宏状态

编译器看到的不是你打开的一个“项目”，而是一个个翻译单元。每个 `.cpp` 经过预处理，得到一份完整文本；编译器再把这份文本编译成 object。头文件、宏、条件编译和 include guard 都发生在“生成这份文本”之前。理解这一层，才能解释为什么同一个头在两个 `.cpp` 里可能得到不同代码，也能解释为什么 include guard 不能解决跨翻译单元的重复定义。

## 预处理先于语义分析

预处理器做的是文本级工作：展开 `#include`，替换宏，选择条件编译分支，处理 `#line` 等指令。它不做类型检查，也不理解函数重载。

```cpp
#define SCALE 2
#define MAKE_VALUE(x) ((x) * SCALE)

int value()
{
    return MAKE_VALUE(21);
}
```

预处理后，编译器看到的核心表达式接近：

```cpp
return ((21) * 2);
```

宏没有作用域类型，也不会只求值一次。`MAKE_VALUE(i++)` 会把副作用带进展开文本。函数、`constexpr`、模板和枚举常量通常比宏更安全；宏仍有价值，但应主要用于条件编译、平台适配、include guard 和少数确实需要生成源码文本的位置。

## `#include` 是文本包含

`#include "config.hpp"` 的结果不是链接到一个共享头文件对象，而是把头文件文本插入当前翻译单元。两个 `.cpp` 包含同一头文件，会各自得到一份文本。头文件里的声明通常安全，因为声明可以重复出现；头文件里的普通外部函数定义会让每个包含它的翻译单元都生成一个定义，后面 ODR 章节会看到重复符号。

include guard 的作用范围是单个预处理过程：

```cpp
#ifndef PROJECT_CONFIG_HPP
#define PROJECT_CONFIG_HPP

int declared_function();

#endif
```

它防止同一个 `.cpp` 在一次预处理中重复包含同一头文件。它不记录“别的 `.cpp` 已经包含过”。`#pragma once` 也是常见工程实践，语义上依赖编译器按文件身份判断只包含一次；它不是 C++ 标准指令。可移植课程代码仍优先展示标准 include guard，并说明 `#pragma once` 的工程便利和边界。

## 每个翻译单元有自己的宏状态

宏状态从当前翻译单元开始，按包含顺序传播。两个 `.cpp` 可以在包含同一头之前定义不同宏，得到不同代码。若这个头生成的是外部 linkage 的 `inline` 函数，两个翻译单元会得到同名但函数体不同的定义，这是 ODR 违规；Release 优化可能掩盖它，不能当成正确。B1 因此把演示函数写成 `static inline`，让每个翻译单元拥有自己的内部 linkage 函数定义：

```cpp
// a.cpp
#define LOCAL_OFFSET 10
#include "generated_value.hpp"

// b.cpp
#define LOCAL_OFFSET 20
#include "generated_value.hpp"
```

如果头文件根据 `LOCAL_OFFSET` 生成 `inline int local_value()`，两个翻译单元看到的函数体不同，就会触碰 ODR 的边界。更常见的安全做法是让宏只控制本地 `constexpr` 数据、编译期开关或只在 `.cpp` 中使用；公共头里的宏条件要非常谨慎，因为消费者的宏状态会改变库接口。

## 条件编译的成本

条件编译可以隐藏平台差异：

```cpp
#if defined(_WIN32)
constexpr char platform_name[] = "windows";
#else
constexpr char platform_name[] = "posix";
#endif
```

这很有用，但它让“同一份源码”在不同环境下变成不同程序。调试宏问题时不要只看原始 `.hpp`，要保存预处理输出。预处理输出能回答三个问题：头文件是否真的进入了翻译单元、宏展开成了什么、条件分支选中了哪条。

## B1 练习如何验证

B1 有两个观察翻译单元：`macro_a.cpp` 和 `macro_b.cpp`。它们包含同一头文件，但在包含前定义不同宏。Reference 检查两个函数的返回值不同，证明宏状态属于各自翻译单元。预处理检查会保存 `macro_a` 与 `macro_b` 的展开文本，并验证输出中确实出现不同的常量。

Student 部分默认不注册。显式打开学生测试时，starter 会失败；学生必须在自己的 `.cpp` 中用安全的局部常量或函数实现结果，不能修改公共头或检查器。

本章结论只到预处理和翻译单元。宏影响 ODR、inline 和模板实例化的工程边界会在后续章节继续展开；完整名字查找、模板偏序和约束归一化留给 C04。
