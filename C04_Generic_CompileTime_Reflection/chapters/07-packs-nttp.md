# 07. 参数包、折叠表达式与 NTTP

泛型代码遇到“不知道有几个类型或值”时，就进入参数包。参数包不是运行时容器；它是一组模板实参，在实例化期间被展开成若干个语法片段。能写对包，核心不是记住 `...` 放哪，而是能说明展开发生在什么语法环境、空包时剩下什么、每个元素的求值顺序是否被语言保证。

本章先修 C02 的表达式和值类别，和本课前面关于模板实例化、重载与约束的规则。

## 包是什么

`class... Ts` 声明一个类型参数包，`auto... Vs` 声明一组非类型模板参数，`template<class> class... Fs` 声明模板模板参数包。包本身不是类型，也不是变量；只有在展开时，才为每个元素生成一个模式。

```cpp
template<class... Ts>
constexpr auto count_types = sizeof...(Ts);

template<auto... Values>
constexpr auto count_values = sizeof...(Values);
```

`sizeof...(Ts)` 不展开包，只读取元素个数。它是少数能直接作用于包名的语法。

展开需要一个模式。`std::type_identity<Ts>{}...` 的模式是 `std::type_identity<Ts>{}`，展开后每个 `Ts` 生成一个对象。函数参数列表、模板实参列表、基类列表、lambda 捕获和初始化列表等位置都有自己的展开规则。先问“模式是什么”，再问“展开成什么分隔形式”，能少掉很多错误。

## 折叠表达式的四种形式

C++17 给二元运算符增加了 fold expression。四种形式是：

```cpp
(pack op ...)          // unary right fold
(... op pack)          // unary left fold
(pack op ... op init)  // binary right fold
(init op ... op pack)  // binary left fold
```

对 `Ns... = 1, 2, 3`，`(Ns + ...)` 等价于 `(1 + (2 + 3))`，`(... + Ns)` 等价于 `((1 + 2) + 3)`。对加法这种结合影响小；对 `-`、`/`、流输出、短路逻辑和逗号，差异会直接改变结果或执行顺序。

空包必须单独想。只有 `&&`、`||` 和逗号的一元 fold 有语言给定 identity：空 `&&` 为 `true`，空 `||` 为 `false`，空逗号表达式为 `void()`。其他一元 fold 遇到空包不良构。想让求和空包为 `0`，就写二元 fold：`(0 + ... + values)`。

## 求值顺序不是展开顺序

展开产生语法，不自动给所有运算符排序。`(f(args) + ...)` 会生成一串 `+` 表达式；`+` 的左右操作数求值顺序没有按左到右保证，所以不能用它记录访问顺序。

需要顺序时，用被语言排序的环境。初始化列表按元素顺序求值，逗号 fold 也保证左表达式先于右表达式：

```cpp
template<class... Fs>
void run_left_to_right(Fs&&... fs) {
    (std::forward<Fs>(fs)(), ...);
}
```

这里用逗号 fold，不是因为它短，而是因为顺序是契约的一部分。若副作用、日志、锁顺序或字段遍历依赖次序，写错运算符就是行为 bug。

## `auto` NTTP 与结构化模板参数

非类型模板参数早期主要是整数、指针和枚举。C++20 扩大了可用范围，`auto` NTTP 让模板从实参推导值类型：

```cpp
template<auto V>
struct constant {
    static constexpr auto value = V;
};
```

`constant<42>`、`constant<'x'>`、`constant<true>` 是不同实例。值参与类型身份，适合表达容量、维度、flag 和策略值。它不适合塞运行时配置；模板实参必须在编译期已知。

C++20 还允许部分结构化类型作为 NTTP。常见教学用途是固定字符串：

```cpp
template<std::size_t N>
struct fixed_string {
    char data[N]{};
    constexpr fixed_string(const char (&text)[N]) {
        std::copy_n(text, N, data);
    }
};

template<fixed_string Name>
struct field_name {};
```

`field_name<"id">` 的名字成为类型身份的一部分。它适合后续反射和字段工具讲“名称在编译期参与选择”；它不是运行期 schema 兼容的完整方案。字符串拼写变了，类型也变了。

## 模板模板参数

有时你要接收的不是一个类型，而是“能从类型生成类型的模板”：

```cpp
template<template<class> class F, class T>
using apply_one = F<T>;
```

模板模板参数常用于把 `add_pointer`、`type_identity` 这类变换传入 type_list 算法。它只描述模板形状；形状不匹配时不会因为名字相似而接受。下章会用它实现 `map`。

## 练习与反例

L07 要实现包计数、空包 identity、四种 fold 的可观察差异、左到右调用、`auto` NTTP、`fixed_string` 和模板模板参数应用。bad 控制实现故意把空 `all` 做成 `false`，checker 会以 `empty all fold identity must be true` 拒绝它。

观察程序展示两件事：一元 fold 没有给 `+` 空包 identity；有副作用时逗号 fold才是顺序证据。候选复验是把 callback 改成记录数组下标，检查得到 `0,1,2`，而不是只看最终计数。
