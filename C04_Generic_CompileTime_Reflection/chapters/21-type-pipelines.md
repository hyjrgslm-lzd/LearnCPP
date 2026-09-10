# 21. 类型管线：zip、flatten、笛卡尔积与完成签名

硬先修：[约束与立即上下文](06-constraints.md)、[参数包](07-packs-nttp.md)、[类型列表](08-type-lists.md)。后续对照[Mp11](23-mp11.md)，实际完成协议仍由C10主讲。

L08 已经建立了 `type_list<Ts...>`、`map`、`filter`、`unique` 这些基础。本章继续做更像库代码的组合：两个列表并行推进、递归摊平嵌套列表、生成笛卡尔积，并把这些算法接到 `std::variant` 返回集合和 toy completion signatures 上。

这一章仍然只处理类型层契约。真实 sender 的环境、调度、取消、operation state 留给 C10；这里需要掌握的是：怎样让一个类型算法在合法输入上给出稳定顺序，在非法输入上能被 concept 查询为 false，并且只在调用者强行取别名时产生编译期诊断。

## zip：等长约束先于递归

目标很直接：

```cpp
zip_t<type_list<int, double>, type_list<char, bool>>
// => type_list<type_list<int, char>, type_list<double, bool>>
```

可实现的骨架是：

```cpp
template<class A, class B> struct zip;       // 默认没有 type
template<> struct zip<type_list<>, type_list<>> {
    using type = type_list<>;
};

template<class A, class... As, class B, class... Bs>
    requires (sizeof...(As) == sizeof...(Bs))
struct zip<type_list<A, As...>, type_list<B, Bs...>> {
    using type = concat_t<
        type_list<type_list<A, B>>,
        typename zip<type_list<As...>, type_list<Bs...>>::type>;
};

template<class A, class B>
concept zipable = requires { typename zip<A, B>::type; };
```

关键不是 `concat_t`，而是偏特化上的 `requires`。以 `type_list<int>` 和 `type_list<char, bool>` 为例，首元素可以配成 `type_list<int, char>`，但尾部是 `type_list<>` 与 `type_list<bool>`。如果不在偏特化入口检查长度，递归会走到未定义的 `zip<type_list<>, type_list<bool>>`，有些编译器会在 concept 查询期间直接报硬错误。把 `sizeof...(As) == sizeof...(Bs)` 放在偏特化上后，长度不等时该偏特化不可选，`zipable<...>` 稳定为 false；只有用户写 `zip_t<type_list<int>, type_list<char, bool>>` 才会得到诊断。

手推 `zip_t<type_list<int,double>, type_list<char,bool>>`：

```text
zip([int,double], [char,bool])
= [ [int,char] ] + zip([double], [bool])
= [ [int,char] ] + [ [double,bool] ] + zip([], [])
= [ [int,char], [double,bool] ]
```

这个顺序必须稳定，因为后面的笛卡尔积和完成签名集合都依赖“输入顺序就是输出顺序”。

## flatten：根必须是 type_list，内部 type_list 才递归

`flatten` 的输入边界比看起来更重要。我们允许列表内部出现普通类型，也允许内部再出现 `type_list`：

```cpp
flatten_t<type_list<int, type_list<double, char>, bool>>
// => type_list<int, double, char, bool>
```

但根输入不是列表时不接受：

```cpp
flattenable<int> // false
flatten_t<int>   // 编译期诊断
```

一种清晰写法是把“处理一个元素”和“处理根列表”分开：

```cpp
template<class L> struct flatten;            // 根默认没有 type

template<class T> struct flatten_one {
    using type = type_list<T>;
};

template<class... Ts> struct flatten_one<type_list<Ts...>> {
    using type = typename flatten<type_list<Ts...>>::type;
};

template<> struct flatten<type_list<>> {
    using type = type_list<>;
};

template<class T, class... Rest>
struct flatten<type_list<T, Rest...>> {
    using type = concat_t<
        typename flatten_one<T>::type,
        typename flatten<type_list<Rest...>>::type>;
};
```

手推 `type_list<int, type_list<double,char>, bool>`：

```text
flatten([int, [double,char], bool])
= flatten_one(int) + flatten([[double,char], bool])
= [int] + flatten_one([double,char]) + flatten([bool])
= [int] + flatten([double,char]) + [bool]
= [int] + [double] + [char] + [bool]
```

注意这里不是“遇到任意模板就展开”，只展开 `type_list`。这样学生实现不会把 `std::tuple<int>`、`std::variant<int>` 误当作课程内部列表。

## cartesian_product：递归、单位元和空列表

`cartesian_product` 的零输入不是空结果，而是单位元：

```cpp
cartesian_product_t<>
// => type_list<type_list<>>
```

它表示“已经有一个空组合，后续元素可以往这个组合前面拼”。这个单位元让递归公式统一：

```cpp
template<class T, class Combos> struct prepend_each;

template<class T, class... Combos>
struct prepend_each<T, type_list<Combos...>> {
    using type = type_list<concat_t<type_list<T>, Combos>...>;
};

template<class First, class TailProduct> struct product_step;

template<class... Ts, class TailProduct>
struct product_step<type_list<Ts...>, TailProduct> {
    using type = concat_many_t<typename prepend_each<Ts, TailProduct>::type...>;
};

template<class... Lists> struct cartesian_product;
template<> struct cartesian_product<> {
    using type = type_list<type_list<>>;
};

template<class First, class... Rest>
struct cartesian_product<First, Rest...> {
    using type = typename product_step<
        First,
        typename cartesian_product<Rest...>::type>::type;
};
```

手推 `cartesian_product_t<type_list<int,double>, type_list<char,bool>>`：

```text
product([int,double], [char,bool])
= prepend each element of [int,double] to product([char,bool])

product([char,bool])
= prepend char to product() + prepend bool to product()
= prepend char to [[]] + prepend bool to [[]]
= [[char], [bool]]

product([int,double], [char,bool])
= prepend int to [[char],[bool]] + prepend double to [[char],[bool]]
= [[int,char], [int,bool], [double,char], [double,bool]]
```

因此顺序是 left-major：固定左边当前元素，再按右侧组合顺序展开。若任意输入列表为空，例如 `cartesian_product_t<type_list<int>, type_list<>>`，`product_step<type_list<>, Tail>` 内部的 pack 展开没有任何项，`concat_many_t<>` 得到 `type_list<>`。这正是“有一个维度没有候选值，所以没有组合”。

## 惰性组合：只实例化选中的 provider

惰性不是让所有计算消失，而是避免访问未选择分支的 `::type`。错误写法常见于 `std::conditional_t` 包住已经求值的别名：

```cpp
template<bool B, class Then, class Else>
using eager = std::conditional_t<B, typename Then::type, typename Else::type>;
```

即使 `B == true`，`typename Else::type` 也已经出现在模板实参里，`Else` 没有 `type` 时会炸。正确组织是偏特化只进入一个分支：

```cpp
template<bool B, class Then, class Else> struct lazy_type;

template<class Then, class Else>
struct lazy_type<true, Then, Else> { using type = typename Then::type; };

template<class Then, class Else>
struct lazy_type<false, Then, Else> { using type = typename Else::type; };
```

A04 的 `lazy_type_t<true, provider<int>, Explosive>` 必须得到 `int`，且不能触碰 `Explosive::type`。这不是性能优化，而是让“合法路径”和“未选路径”分开。

## 两层合法性：查询 false 与强行取别名

本章有两层接口：

```cpp
template<class A, class B>
concept zipable = requires { typename zip<A, B>::type; };

template<class A, class B>
using zip_t = typename zip<A, B>::type;
```

`zipable`、`flattenable`、`transformable` 用来写约束和测试 false boundary。`zip_t`、`flatten_t`、`transform_completion_signatures_t` 是“我确认合法，请给我类型”的别名。这样 API 使用者可以先查询，再决定是否进入错误分支；诊断用例则故意强行取别名，确认真实错误来自语义约束，而不是 include path、CMake 或 helper。

## variant 组合：先展开，再复用 product

`std::variant<int, double>` 可以看成一个候选类型列表：

```cpp
template<class V> struct variant_types;
template<class... Ts>
struct variant_types<std::variant<Ts...>> {
    using type = type_list<Ts...>;
};
```

两个 variant 的组合就是：

```cpp
variant_product_t<std::variant<int, double>, std::variant<char, bool>>
// => cartesian_product_t<type_list<int,double>, type_list<char,bool>>
// => type_list<type_list<int,char>, type_list<int,bool>,
//              type_list<double,char>, type_list<double,bool>>
```

这和实际库里“把多个输入 variant 访问后得到的所有可能调用签名列出来”是同一种形状。课程不需要实现 `std::visit`；先把类型集合算对，后面才能讨论运行时分派。

## completion signatures：值通道变换，错误和停止保留

本章使用 toy 签名：

```cpp
template<class... Ts> struct value_sig {};
template<class E> struct error_sig {};
struct stopped_sig {};
```

`transform_completion_signatures<F, Sigs>` 做三件事：

1. 对每个 `value_sig<Ts...>`，检查 `F` 是否能用 `Ts...` 调用。
2. 用 `std::invoke_result_t<F, Ts...>` 生成新的 `value_sig<R>`；若返回 `void`，生成 `value_sig<>`。
3. 若 `std::is_nothrow_invocable_v<F, Ts...>` 为 false，额外加入 `error_sig<std::exception_ptr>`；原有 `error_sig<E>` 与 `stopped_sig` 原样保留。

核心步骤可以拆开写：

```cpp
template<class F, class Sig> struct transform_one;

template<class F, class... Ts>
    requires std::is_invocable_v<F, Ts...>
struct transform_one<F, value_sig<Ts...>> {
    using R = std::invoke_result_t<F, Ts...>;
    using V = std::conditional_t<std::is_void_v<R>, value_sig<>, value_sig<R>>;
    using type = std::conditional_t<
        std::is_nothrow_invocable_v<F, Ts...>,
        type_list<V>,
        type_list<V, error_sig<std::exception_ptr>>>;
};

template<class F, class E>
struct transform_one<F, error_sig<E>> { using type = type_list<error_sig<E>>; };

template<class F>
struct transform_one<F, stopped_sig> { using type = type_list<stopped_sig>; };
```

手推一个会抛的 `ThrowingText`：

```cpp
using input = type_list<value_sig<int>, error_sig<std::logic_error>, stopped_sig>;
using output = transform_completion_signatures_t<ThrowingText, input>;
```

轨迹是：

```text
value_sig<int>
=> invoke_result_t<ThrowingText, int> == std::string
=> type_list<value_sig<std::string>, error_sig<std::exception_ptr>>

error_sig<std::logic_error>
=> type_list<error_sig<std::logic_error>>

stopped_sig
=> type_list<stopped_sig>

flatten + unique
=> type_list<value_sig<std::string>, error_sig<std::exception_ptr>,
             error_sig<std::logic_error>, stopped_sig>
```

`F` 的值类别在这个 toy 版本里作为模板参数传入；`std::is_invocable_v<F, Ts...>` 与 `std::is_nothrow_invocable_v<F, Ts...>` 必须使用同一个 `F`，否则可能出现“能调用性按一种 cvref 判断，noexcept 按另一种 cvref 判断”的不一致。A04 不处理真实 sender 中 receiver/scheduler 的值类别传播，只要求这个类型集合内部自洽。

最终的 `transformable` 不能只写成“能访问总别名”，因为总别名可能在 MSVC concept 查询中产生硬错误，也可能因为不完整主模板被误判。更稳的形态是先定义单步合法性，再对列表里每个签名折叠：

```cpp
template<class F, class Sig>
concept transformable_one = requires { typename transform_one<F, Sig>::type; };

template<class F, class L> struct transformable_impl : std::false_type {};

template<class F, class... Sigs>
struct transformable_impl<F, type_list<Sigs...>>
    : std::bool_constant<(transformable_one<F, Sigs> && ...)> {};

template<class F, class L>
concept transformable = transformable_impl<F, L>::value;
```

这就是“false boundary”：`PointerOnly` 不能处理 `value_sig<int>` 时，`transformable<PointerOnly, input>` 为 false；诊断用例若强行请求别名，才得到清晰编译错误。

[A04练习](../exercises/A04_type_pipelines/README.md)要求实现这些算法，checker 覆盖顺序、空输入、非法输入 false 边界、异常通道和 eager 反例。
