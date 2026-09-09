# 02：ODR、符号与头文件里的定义

C++ 的 `#include` 不是“导入一个已经编译好的模块”。在普通头文件模型里，预处理器只是把头文件文本复制进当前 `.cpp`，然后每个 `.cpp` 独立形成一个翻译单元。这个事实很小，却会直接决定函数应该放在头文件还是 `.cpp`，也决定链接器为什么会在“每个源文件都能单独编译”之后才报错。

本章只解决一个具体问题：一个普通函数 `int lesson_value()` 返回 `42`，最开始只有一个调用者时，把它直接定义在头文件里看起来完全合法；当第二个 `.cpp` 也包含这个头文件时，本机 MSVC 链接阶段会看到两个同名外部符号定义并拒绝生成可执行文件。我们先复现实验，再解释 ODR、声明、定义、linkage、include guard 和 `inline` 的边界，最后给出两种独立正确修复。

## 从一个能工作的头文件定义开始

先看这个头文件：

```cpp
// lesson.hpp
#ifndef LESSON_HPP
#define LESSON_HPP

int lesson_value()
{
    return 42;
}

#endif
```

如果只有一个 `.cpp` 包含它：

```cpp
#include "lesson.hpp"

int main()
{
    return lesson_value() == 42 ? 0 : 1;
}
```

这个程序在普通实现上可以编译、链接、运行成功。原因不是“头文件里定义函数总是安全”，而是当前程序只有一个翻译单元包含了这段定义。预处理之后，这个翻译单元里确实只有一个 `lesson_value` 的外部定义；链接器没有看到第二份同名定义。

本章配套练习里的观察程序 `C1_odr_observation_single` 就是这个阶段。它打印 `single consumer header definition: 42`，说明单消费者场景本身可以成立。

## 加第二个翻译单元后，问题出现

现在保留同一个头文件，再加两个调用者：

```cpp
// caller_a.cpp
#include "lesson.hpp"

int call_a()
{
    return lesson_value();
}
```

```cpp
// caller_b.cpp
#include "lesson.hpp"

int call_b()
{
    return lesson_value();
}
```

每个 `.cpp` 仍然能单独编译，因为每个翻译单元内部只看见一个 `lesson_value` 定义。问题在链接阶段出现：`caller_a.obj` 里有一份外部符号 `lesson_value` 的定义，`caller_b.obj` 里也有一份同名外部符号定义。链接器要把这些对象文件合成一个程序时，不能选择其中一个并丢弃另一个，因为普通非 `inline` 外部函数在整个程序里只能有一个定义。

在 MSVC 上，配套负例会在链接时出现类似 `LNK2005` / `already defined` / `multiply defined` 的诊断。GCC/Clang ELF 工具链通常会报 `multiple definition of lesson_value`。错误文字属于实现诊断，不是标准固定文本。

这也是 ODR 问题容易误判的地方：编译通过不代表整个程序满足 ODR。普通多文件 C++ 工程至少有前端编译和后端链接两个阶段；名字能在每个 `.cpp` 中被解析，不代表这些 `.obj` 能共同组成一个合法程序。

## 预处理和符号表能证明什么

预处理输出能证明 `#include` 的文本复制效果。对 `caller_a.cpp` 做预处理后，可以在输出里看到头文件里的函数体已经进入这个翻译单元：

```cpp
int lesson_value()
{
    return 42;
}
```

对 `caller_b.cpp` 做同样操作，也会看到一份相同的函数体。include guard 只防止同一个翻译单元里重复展开同一个头文件；它不能阻止另一个翻译单元也展开一次。

对象文件符号表能证明链接器看到的是两个外部定义。MSVC 下可以用 `dumpbin /symbols caller_a.obj` 和 `dumpbin /symbols caller_b.obj` 观察；两个对象文件都会包含名字中带 `lesson_value` 的外部符号。GCC/Clang 下可用 `nm -C` 观察同类结果。本章的验证脚本在 Windows 主环境使用 `dumpbin` 做这一步；GCC/ELF 命令保留在练习说明里，本次不把它伪装成已验证。

这些证据各自证明的对象不同。预处理输出证明“头文件文本进入了每个翻译单元”；符号表证明“每个对象文件都产生了一个可链接的同名定义”；链接失败证明“当前工具链拒绝把这些定义合成一个程序”。标准层面的原因是普通外部函数的 ODR 约束，不是 include guard 失效。

## ODR、声明、定义和 linkage

声明告诉编译器“这个名字存在，类型是什么”。定义进一步提供实体本身。下面这一行只是函数声明：

```cpp
int lesson_value();
```

下面这段是函数定义：

```cpp
int lesson_value()
{
    return 42;
}
```

普通命名空间作用域函数默认有 external linkage。external linkage 的意思是：不同翻译单元里的同名声明可以指向同一个程序实体。正因为它们能跨翻译单元相连，整个程序里不能同时出现多份互相冲突的普通外部函数定义。

ODR，One Definition Rule，约束的是“一个程序中哪些实体可以有几个定义，以及这些定义要满足什么条件”。对本章这个普通非 `inline` 函数来说，安全规则很直接：头文件放声明，某一个 `.cpp` 放唯一函数体。多个 `.cpp` 都包含声明没有问题，因为声明可以重复；多个 `.cpp` 都包含普通函数体就会变成多份外部定义。

不是所有 ODR 违规都要求实现必须诊断。尤其是跨翻译单元的某些不一致定义，工具链可能无法或不会完整检查。本章选择普通外部函数重复定义，是因为它能在本机链接阶段稳定给出诊断，适合作为第一章工程实验。不能从这个例子反推“所有 ODR 违规都会这样报错”。

## include guard 的局限

include guard 解决的是同一个翻译单元内的重复包含：

```cpp
#ifndef LESSON_HPP
#define LESSON_HPP
// header body
#endif
```

如果 `caller_a.cpp` 通过两个路径间接包含了 `lesson.hpp`，guard 会让头文件体在 `caller_a.cpp` 里只保留一次。这很好，但它的作用范围只在当前预处理过程。`caller_b.cpp` 是另一次独立预处理，宏状态重新开始，所以 guard 不会阻止 `caller_b.cpp` 也得到一份函数体。

因此，“我已经写了 include guard”不能回答“这个定义能不能放在头文件里”。前者是文本展开去重问题，后者是程序实体定义数量问题。

## 修复一：头文件声明，`.cpp` 单一定义

第一种修复最适合普通函数：

```cpp
// lesson.hpp
#ifndef LESSON_HPP
#define LESSON_HPP

int lesson_value();

#endif
```

```cpp
// lesson.cpp
#include "lesson.hpp"

int lesson_value()
{
    return 42;
}
```

`caller_a.cpp` 和 `caller_b.cpp` 都包含头文件，但只得到声明。链接时，两个调用点都解析到 `lesson.cpp` 产生的唯一外部定义。这个方案让 ABI 边界更清楚，也能减少调用者因函数体变化而需要重编译的情况。缺点是多一个 `.cpp` 需要进入构建系统；忘记把它加入 target 时会从重复定义错误变成 unresolved external。

本章的 `C1_odr_reference` 采用这个修复。它通过两个调用路径分别取得 `42`，检查器要求两次返回值和调用路径都正确，不能只在 `main` 里硬编码答案。

## 修复二：把头文件定义声明为 `inline`

第二种修复适合很小、必须在头文件中提供定义的函数：

```cpp
#ifndef LESSON_HPP
#define LESSON_HPP

inline int lesson_value()
{
    return 42;
}

#endif
```

`inline` 的工程含义不是“强制编译器内联展开调用”。它允许这个函数定义出现在多个翻译单元中，只要这些定义满足 ODR 对 inline 定义的一致性要求。优化器是否真的把调用展开，是另一个独立问题。

C++17 还引入了 inline variable。像 CPO、头文件常量对象这类需要在头文件中定义的变量，常见写法是 `inline constexpr`。没有 `inline` 的命名空间作用域变量如果被多个翻译单元包含，也会遇到类似的多定义问题。模板也常把定义放在头文件里，因为实例化模型要求使用点能看到定义；但模板的名字查找、偏序、特化和实例化边界比本章复杂，放到 C04 系列完整讲。

`inline` 不是“头文件里所有东西都加上它”的免死牌。多个翻译单元中的 inline 定义仍然要等价，且所有 odr-use 的地方都必须能看到定义。对于有状态、较重、需要稳定 ABI 或不希望暴露实现的函数，分离单一定义通常更合适。

## 原输入复验

修复后仍然使用原来的两个调用者：

```cpp
int call_a();
int call_b();

int main()
{
    return call_a() == 42 && call_b() == 42 ? 0 : 1;
}
```

这一步很重要。不能只编译新写的 `lesson.cpp`，也不能只运行一个调用者。原始问题是“第二个翻译单元加入后链接失败”，所以修复验证必须回到两个调用者共同链接、共同运行的输入。配套练习中 Reference 和 Student 都有两个调用路径；Reference 应通过，Starter Student 会返回非零，直到你把学生侧实现修好。

## 自测

1. 为什么单个 `.cpp` 包含普通函数头文件定义时可以运行，而两个 `.cpp` 包含时会链接失败？

   **解析：** 单个 `.cpp` 预处理后只有一份外部函数定义。两个 `.cpp` 各自预处理、编译后，会在两个对象文件中产生同名外部函数定义。普通非 `inline` 外部函数在整个程序中只能有一个定义，链接器因此拒绝合并。

2. include guard 为什么不能修复跨翻译单元重复定义？

   **解析：** include guard 的宏状态只存在于一次预处理过程。它能阻止同一个 `.cpp` 内重复展开同一头文件，但另一个 `.cpp` 会重新开始预处理，因此仍会得到自己的函数体副本。

3. `inline` 在这里解决的是什么？它是否保证机器码中没有函数调用？

   **解析：** `inline` 允许函数定义出现在多个翻译单元里，并要求这些定义满足一致性约束。它不保证优化器把调用展开；是否内联调用由优化器、调用上下文和编译选项决定。

4. 为什么本章不说“所有 ODR 违规都会报错”？

   **解析：** 标准并不要求所有 ODR 违规都被诊断。某些跨翻译单元不一致定义可能没有稳定诊断，甚至表现为运行期错误或未定义行为。本章选用普通外部函数重复定义，是为了得到稳定、可教学的链接阶段证据。

5. 什么时候优先选择“声明 + `.cpp` 单一定义”，什么时候可以选择 `inline`？

   **解析：** 普通函数、较重实现、需要隐藏实现或维护 ABI 边界时，优先声明加单一定义。小型头文件工具函数、模板相关函数或确实需要在头文件中提供定义时，可以使用 `inline`，但仍要保证所有翻译单元看到的一致定义。

