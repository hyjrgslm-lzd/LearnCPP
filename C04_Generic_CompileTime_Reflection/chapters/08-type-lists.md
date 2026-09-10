# 08. Traits、类型列表与完成签名

模板元编程的基本动作只有两类：算类型，和值。`std::is_same_v<T, U>` 是值计算，结果是 `bool`；`std::remove_cvref_t<T>` 是类型计算，结果是另一个类型。复杂的库接口只是把这两类计算组合得更长。

本章把 traits 的机制、惰性实例化、`type_list` 算法和一个下游场景连起来：从 `value_sig<Ts...>`、`error_sig<E>`、`stopped_sig` 推导新的完成签名集合。它只是类型签名练习，桥接 C10 的完成通道概念；它不实现 sender 运行时，也不声称本课 toy 类型就是 stdexec 协议。

## traits 的最小模型

traits 是“用类型承载信息”的约定：

```cpp
template<class T>
struct is_pointer_like : std::false_type {};

template<class T>
struct is_pointer_like<T*> : std::true_type {};

template<class T>
inline constexpr bool is_pointer_like_v = is_pointer_like<T>::value;
```

主模板给默认答案，偏特化覆盖更具体形态。`std::true_type` 和 `std::false_type` 只是带 `value` 的类型。很多标准 traits 都是这个模式。

类型计算也一样：

```cpp
template<class T>
struct add_lvalue_reference { using type = T&; };

template<>
struct add_lvalue_reference<void> { using type = void; };
```

这里要有 `void` 特例，因为 `void&` 不良构。元函数不是魔法；它仍受语言类型规则约束。

## 惰性实例化

模板只在需要时实例化。这个性质让我们能把危险表达式放进受约束的分支：

```cpp
template<class F, class... Ts>
concept invocable_with = requires { typename std::invoke_result<F, Ts...>::type; };
```

若 `F(Ts...)` 不可调用，概念为 `false`。只要后续算法在约束不满足时不去取 `invoke_result_t`，程序就能给出有限诊断，而不是在深层展开里炸掉。

惰性不是“永远不报错”。当你真的请求某个别名或成员 `::type`，相关模板就要实例化。练习的拒绝条件就是这个边界：不可调用的签名变换应在 concept 上显示为不满足，而不是让 Student 头文件自己无法包含。

## `type_list` 的基础算法

`type_list<Ts...>` 是教学容器。它没有对象状态，只把一组类型带过模板边界。常见算法如下：

- `map<F, list<Ts...>>` 生成 `list<F<Ts>...>`。
- `filter<P, list<Ts...>>` 保留 `P<T>::value` 为真的类型。
- `concat<list<A...>, list<B...>>` 生成 `list<A..., B...>`。
- `unique<list<Ts...>>` 保留第一次出现的类型，删除后续重复类型。

这些算法能验证包展开、偏特化和递归。它们也能暴露错误的 eager 实例化：如果 `filter` 在不该访问的分支也取了不存在的成员，类型列表里有一个坏类型就会让整课不可构建。

先从 `map` 手推。目标是把 `type_list<int, double>` 变成 `type_list<F<int>, F<double>>`。主模板只声明形状：

```cpp
template<template<class> class F, class List>
struct map;
```

真正能工作的版本是对 `type_list<Ts...>` 的偏特化：

```cpp
template<template<class> class F, class... Ts>
struct map<F, type_list<Ts...>> {
    using type = type_list<F<Ts>...>;
};
```

当请求 `map_t<add_pointer, type_list<int, double>>` 时，偏特化把 `Ts...` 绑定为 `int, double`，模式 `F<Ts>` 展开成 `add_pointer<int>, add_pointer<double>`。若 `add_pointer<T>` 是别名模板，最终就是 `type_list<int*, double*>`。这里没有递归，因为所有元素都做同一种局部变换。

`filter` 不能简单包展开，因为每个元素可能保留，也可能丢弃。最小形状是空列表基例加递归步：

```cpp
template<template<class> class P>
struct filter<P, type_list<>> {
    using type = type_list<>;
};

template<template<class> class P, class T, class... Rest>
struct filter<P, type_list<T, Rest...>> {
    using tail = typename filter<P, type_list<Rest...>>::type;
    using type = /* P<T>::value ? push_front<T, tail> : tail */;
};
```

手推 `filter_t<is_integral, type_list<int, double, char>>`：

1. 先处理 `int`，递归问题变成过滤 `type_list<double, char>`。
2. 处理 `double`，递归问题变成过滤 `type_list<char>`。
3. 处理 `char`，递归问题变成空列表，得到 `type_list<>`。
4. 回到 `char`，谓词为真，得到 `type_list<char>`。
5. 回到 `double`，谓词为假，仍是 `type_list<char>`。
6. 回到 `int`，谓词为真，得到 `type_list<int, char>`。

`push_front` 是辅助元函数：`push_front<T, type_list<Ts...>>::type` 生成 `type_list<T, Ts...>`。保留分支才需要它；丢弃分支只返回 `tail`。这也是惰性实例化的边界：如果没选中某个分支，那个分支里危险的 `typename X::type` 不应被实例化。

`unique` 的坑是顺序。若从尾部递归并在 tail 中检查重复，很容易保留最后一次出现。要保留第一次出现，可以带一个已见集合：

```text
unique_impl<Seen, Rest>
不变量：Seen 按原顺序保存已经接受的类型，Rest 是还没扫描的后缀。
```

手推 `unique_t<type_list<int, double, int, char>>`：

```text
Seen = type_list<>, Rest = int, double, int, char
int 不在 Seen 里  -> Seen = int
double 不在 Seen 里 -> Seen = int, double
int 已在 Seen 里 -> Seen 不变
char 不在 Seen 里 -> Seen = int, double, char
Rest 为空，结果就是 Seen
```

这个不变量比“递归删除重复”更重要。读者只要能维护 Seen/Rest，就能解释空列表基例、递归步和重复类型的位置，不需要偷看 Reference。

## 完成签名变换

C10 会讲 sender 的完成通道：值、错误、停止分别是不同信号。本章只取类型层面的最小模型：

```cpp
template<class... Ts> struct value_sig {};
template<class E> struct error_sig {};
struct stopped_sig {};
```

给一个函数对象类型 `F` 和签名列表，规则是：

- `value_sig<Ts...>`：若 `F` 可用 `Ts...` 调用，把它变成 `value_sig<R>`；若结果是 `void`，变成 `value_sig<>`。
- `error_sig<E>` 原样保留。
- `stopped_sig` 原样保留。
- 任意 `value_sig<Ts...>` 不满足调用条件时，整个变换不可用。
- 重复签名用 `unique` 合并，空列表得到空列表。

这闭合了类型签名变换练习：值通道被映射，错误与停止通道不丢，`void` 不造假类型，不可调用被拒绝。它只是静态推导。真实 sender 还要处理环境、operation state、调度、取消和运行时完成；这些留给 C10。

这里还需要两层独立的合法性边界。仅在最外层写一个 `requires`，不会自动吞掉内层普通类模板体中的硬错误；这与[第05章的立即上下文](05-overload-sfinae.md)是同一条规则。先让不可调用的单步变换没有 `type`，再让整个列表只对每一步都有 `type` 的情况开放：

```cpp
template<class F, class Sig> struct step; // 不支持的形状没有 type
template<class F, class... Ts>
    requires std::is_invocable_v<F, Ts...>
struct step<F, value_sig<Ts...>> {
    using R = std::invoke_result_t<F, Ts...>;
    using type = std::conditional_t<std::is_void_v<R>, value_sig<>, value_sig<R>>;
};

template<class F, class List> struct checked_transform;
template<class F, class... Sigs>
    requires (requires { typename step<F, Sigs>::type; } && ...)
struct checked_transform<F, type_list<Sigs...>> {
    using type = unique_t<type_list<typename step<F, Sigs>::type...>>;
};
```

错误/停止标签另外提供原样保留的 `step` 偏特化。手推一个只能接收 `int` 的 F 与 `value_sig<std::string>`：单步偏特化约束失败，因此主模板没有 `type`；列表上的类型要求为 false，因此不进入结果列表形成。相反，若先在一个无约束的类模板体内请求 `invoke_result_t`，错误已经发生在那个类的实例化中，外层再检测它的 `type` 就太晚了。L08 的 `transformable` 检查要求的是可查询的 false，不能用一个导致整个翻译单元失败的“拒绝”代替。

再看惰性正反例。定义两个 provider：

```cpp
template<class T>
struct provider { using type = T; };

struct explosive; // 没有 type
```

正确的 `lazy_type_t<true, provider<int>, explosive>` 应得到 `int`，因为 `false` 分支没有被选中，`explosive::type` 不该被访问。若实现写成先同时取 `Then::type` 和 `Else::type`，再按布尔值选择，就会在本来安全的 `true` 场景中报 `missing_type`。练习的 observation 证明正例，diagnostic case 隔离 eager 反例：control 先编译正确版本，subject 再要求错误版本以预期诊断失败。

## 练习与反例

L08 要实现 `type_list` 算法和 `transform_completion_signatures<F, List>`。checker 覆盖空列表、重复类型、`void` 结果、`error_sig`/`stopped_sig` 保留，以及不可调用条件下 concept 为 `false`。

bad 控制实现把错误和停止通道丢掉，checker 会以 `error and stopped signatures must survive value transform` 拒绝。候选复验是遮住答案，只看 README 的规则，手算 `value_sig<int>`、`value_sig<std::string>`、`error_sig<std::exception_ptr>` 和 `stopped_sig` 的输出集合。
