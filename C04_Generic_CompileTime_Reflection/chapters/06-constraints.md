# 06. requires、Concepts 与约束偏序

约束不是给模板贴一个好看的名字。它决定候选是否存在，影响重载排序，也把一部分错误从函数体挪到接口边界。写得好的约束让调用者看到“这个接口为什么不适用”；写得差的约束只是把 hard error 换成更绕的 hard error。

本章用 `c04_constraints::field_count(range)` 建立模型。它只接受“可多遍历的字段 range”：range 的引用元素有 `name` 和 `value`，`name` 能看作 `std::string_view`，`value` 是整数，range 至少是 `forward_range`。这不是完整反射字段协议，只是后续字段项目的一个最小前置：接口需要多次看同一组字段时，单遍输入 range 不能混进来。

## 四种 requirement

`requires` 表达式里常见四种 requirement。

simple requirement 只检查表达式是否良构：

```cpp
template<class T>
concept has_size = requires(T& x) {
    x.size();
};
```

它不要求表达式返回什么，也不调用表达式。它只问“这个表达式能不能被形成”。

type requirement 检查类型名是否存在：

```cpp
template<class T>
concept has_value_type = requires {
    typename T::value_type;
};
```

这里 `typename` 仍然必要，因为 `T::value_type` 是依赖名。没有它，解析器不知道这是类型。

compound requirement 能检查 `noexcept` 和返回类型约束：

```cpp
template<class T>
concept named = requires(T& x) {
    { x.name } -> std::convertible_to<std::string_view>;
};
```

箭头后面不是“返回类型必须正好是这个类型”，而是一个约束。上例要求表达式结果能转换成 `std::string_view`。

nested requirement 在 requires 里继续写布尔约束：

```cpp
template<class T>
concept small = requires {
    requires sizeof(T) <= 64;
};
```

它适合表达不能写成单个表达式返回约束的条件。练习里的 `field_like` 会用 compound requirement 检查成员形状，再用普通 concept 组合 range 能力。

## dependent 失败和 non-dependent 硬错误

requires 表达式只把依赖检查变成“满足或不满足”。若错误和模板参数无关，它仍是模板定义本身的错误。

```cpp
template<class T>
concept ok = requires(T x) {
    x.begin();       // 依赖 T，失败时概念为 false
};
```

`x.begin()` 是否存在取决于 `T`。对没有 `begin()` 的类型，`ok<T>` 是 false。

但下面不是可延迟失败：

```cpp
template<class T>
concept broken = requires(T x) {
    missing_global_name(x);
};
```

若 `missing_global_name` 的普通查找在定义点就失败，并且没有依赖查找能补上候选，编译器可以直接报错。不要把 requires 当成 try/catch。它不会修复不依赖模板实参的拼写错误、缺失声明或语法错误。

## concept-id 像布尔表达式，但不只是布尔表达式

可以在 requires 子句里写：

```cpp
template<class R>
    requires std::ranges::forward_range<R>
std::size_t count(R&& r);
```

`std::ranges::forward_range<R>` 看起来像一个布尔值，语义上确实可用于判断是否满足。但编译器还会保留它展开后的原子约束，用来做约束包含和偏序。这个内部结构会影响重载选择。

如果你把同一个语义拆成两个不同表达式，结果可能逻辑等价，却不能互相包含：

```cpp
template<class T>
concept A = sizeof(T) > 1;

template<class T>
concept B = sizeof(T) > 1;
```

`A<T>` 和 `B<T>` 对所有类型返回同样布尔值，但它们来自不同 concept 定义，原子约束身份不同。重载偏序比较的是归一化后的原子约束集合，不做数学定理证明。编译器不会因为两个表达式逻辑等价就推导它们是同一个约束。

这就是为什么库代码会复用小 concept，而不是到处重写同样的 requires 表达式。复用不是为了少打字，而是为了保留可比较的原子约束身份。

## 原子约束、归一化和 subsumption

约束会被归一化成由原子约束组成的逻辑式。`C<T> && D<T>` 包含两个原子；`C<T> || D<T>` 是析取。重载排序时，一个候选的约束若 subsume 另一个，且其他条件相当，更受约束的候选可优先。

```cpp
template<class R>
concept field_range = requires(std::ranges::range_reference_t<R> field) {
    { field.name } -> std::convertible_to<std::string_view>;
};

template<class R>
concept stable_field_range =
    std::ranges::forward_range<R> && field_range<R>;
```

这里 `stable_field_range<R>` 明确包含 `field_range<R>`，所以在约束排序中能表达“稳定字段 range 是字段 range 的更窄版本”。如果另一处重新手写 `requires { field.name; } && std::ranges::forward_range<R>`，语义近似，但不一定和 `field_range<R>` 形成同一组原子身份。

约束映射还会把 concept 参数替换成实际模板参数。`field_range<R>` 的原子不是裸的 `field_range<X>` 字符串，而是 concept 定义里的表达式带着从 `R` 到实参的映射。不同映射可能让看似相同的表达式不再是同一个原子。

## 语法满足不等于语义公理

Concepts 检查的是语法和一部分可静态表达的语义约束。标准库 concepts 还带有语义要求，例如 equality-preserving、多遍历、迭代器失效边界等。编译器通常无法证明这些公理。

`std::forward_iterator` 不只是“有 `++` 和 `*`”。它承诺多遍遍历同一范围时能看到稳定序列。一个类型可以写出满足语法的操作，却在运行时每次解引用都改变值；编译器未必能拦住。库算法仍按概念的语义使用它，违反语义要求就是调用者类型的 bug。

L06 的 `field_count` 因为要给后续字段工具提供稳定入口，所以约束到 `forward_range`。checker 只能证明代表性反例被拒绝、正例可重复计数；它不能形式化证明所有用户 range 都 equal-preserving。正文必须把这条边界说清楚：约束是契约入口，不是证明器。

## C++23、C++26 与包折叠约束

C++23 里包折叠约束有一个实际坑：两个逻辑上相同的折叠，若来自不同表达式，可能不能按读者直觉形成 subsumption。C++26 对 fold-expanded constraints 做了改进，使约束归一化能更好展开包折叠里的原子，减少一些“更约束的重载没有胜出”的情况。

这不是说 C++26 会证明任意逻辑等价。它改进的是包折叠约束的归一化规则，不是给编译器加通用定理证明器。写库时仍应复用概念、避免重复手写等价表达式，并在文档里固定讨论的标准版本。

本课程核心代码按 C++23 构建，所以不会把 C++26 行为伪装成本机已验证行为。涉及 C++26 的段落只说明规则差异和后续验证入口。

## 练习目标

L06 要实现三个公开名字：

```cpp
namespace c04_constraints {
template<class T>
concept field_like = /* ... */;

template<class R>
concept stable_field_range = /* ... */;

inline constexpr field_count_fn field_count{};
}
```

`field_like<T>` 检查 `T` 有 `name` 和 `value`，其中 `name` 可转成 `std::string_view`，`value` 是整数类型。`stable_field_range<R>` 要求 `R` 是 `std::ranges::forward_range`，并且 `std::ranges::range_reference_t<R>` 满足字段形状。`field_count(r)` 只对 `stable_field_range` 可调用，遍历并返回元素数量。

checker 覆盖四类事实：正例 vector/array 可调用且能重复得到同一计数；缺 `name`、非整数 `value` 和 no-path 类型不可调用；单遍 input range 不满足 `stable_field_range`；bad 控制实现若只写 `input_range` 会被拒绝。这个题刻意不写完整字段序列化，因为那属于后续 P1。这里的最小目标是把约束放在接口边界，并让读者看清语法检查与语义承诺的分界。
