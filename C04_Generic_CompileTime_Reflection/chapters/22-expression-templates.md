# 22. 表达式模板：固定向量的延迟求值

硬先修：[转发](03-forwarding-ctad.md)、[tuple与借用表达式](09-tuple-traversal.md)，以及C02的对象生命周期和独占/借用边界。[显式对象参数](20-explicit-object-forwarding.md)是建议对照；数值、SIMD和GPU应用由C13/C14深入。

表达式模板不是为了让代码看起来高级。它解决一个很具体的问题：`a + b * 2.0 + reverse(c)` 如果每个运算符都立刻生成中间 `vec`，会有多次遍历和多个临时结果；如果运算符只生成表达式节点，最后 `eval` 再遍历一次，就能把组合表达式留到求值点处理。

本章只做固定大小 `double` 向量、加法、标量乘、`reverse`。不引入 Eigen，不讲矩阵分解、SIMD、GPU 或数值稳定性；这些留给 C13/C14。这里要学的是泛型表达式节点如何保存子表达式、生命周期如何由左值/右值决定、`assign` 如何处理别名。

## 正确 eager 基线

先写能被直接验证的 eager 版本：

```cpp
template<std::size_t N>
struct vec {
    std::array<double, N> data{};
    double& operator[](std::size_t i) { return data[i]; }
    const double& operator[](std::size_t i) const { return data[i]; }
};

vec<3> add_scale_reverse(const vec<3>& a, const vec<3>& b, const vec<3>& c) {
    vec<3> out{};
    for (std::size_t i = 0; i < 3; ++i) {
        out[i] = a[i] + b[i] * 2.0 + c[2 - i];
    }
    return out;
}
```

对 `a={1,2,3}`、`b={4,5,6}`、`c={10,20,30}`，结果是：

```text
i=0: 1 + 4*2 + c[2] = 39
i=1: 2 + 5*2 + c[1] = 32
i=2: 3 + 6*2 + c[0] = 25
```

A05 的 `eager_numeric_reference.cpp` 保留这个数值基线。后面的表达式模板必须先等价，再谈遍历形态。不要从这个小例子推导速度结论。

## 表达式节点保存什么

表达式节点不是数组；它保存子表达式，并在 `operator[](i)` 时递归计算。一个加法节点可以长这样：

```cpp
template<class T>
using store_t = std::conditional_t<
    std::is_lvalue_reference_v<T>,
    T,
    std::remove_cvref_t<T>>;

template<class T>
constexpr decltype(auto) at(T&& e, std::size_t i) {
    return std::forward<T>(e)[i];
}

template<class L, class R>
struct add_expr {
    store_t<L> left;
    store_t<R> right;
    static constexpr std::size_t size = expr_size_v<L>;

    constexpr double operator[](std::size_t i) const {
        return at(left, i) + at(right, i);
    }
};
```

`store_t` 是生命周期策略：

```text
L = vec<3>&          => store_t<L> = vec<3>&       // 左值借用
L = const vec<3>&    => store_t<L> = const vec<3>& // const 左值借用
L = vec<3>           => store_t<L> = vec<3>        // 右值按值拥有
L = scale_expr<...>  => store_t<L> = scale_expr<...>
```

因此 `a + vec<3>{{1,1,1}}` 中右侧临时会被移动进表达式节点，不会在表达式对象求值前悬垂；`a + b` 中两个左值则只保存引用，后续修改 `b` 会影响还未 `eval` 的表达式。这是表达式模板的正常借用语义。

## 大小约束：运算符入口拒绝不匹配

每种表达式都有 `expr_size`：

```cpp
template<class T> struct expr_size;
template<std::size_t N>
struct expr_size<vec<N>> : std::integral_constant<std::size_t, N> {};

template<class L, class R>
struct expr_size<add_expr<L, R>>
    : std::integral_constant<std::size_t, add_expr<L, R>::size> {};
```

加法运算符在入口做约束：

```cpp
template<expr L, expr R>
    requires (expr_size_v<L> == expr_size_v<R>)
constexpr auto operator+(L&& left, R&& right) {
    return add_expr<L, R>{std::forward<L>(left), std::forward<R>(right)};
}
```

这样 `vec<2> + vec<3>` 不会生成一个半坏的表达式节点，而是在重载解析阶段被拒绝。A05 的诊断用例匹配 MSVC 的 `C2676/C2672`，确认失败来自大小约束。

## 手推 `a + b * 2.0`

给定：

```cpp
vec<3> a{{1, 2, 3}};
vec<3> b{{4, 5, 6}};
a + b * 2.0;
```

类型推导轨迹是：

```text
b * 2.0
=> operator*<vec<3>&>(vec<3>&, double)
=> scale_expr<vec<3>&>{ inner = b&, factor = 2.0 }

a + (b * 2.0)
=> operator+<vec<3>&, scale_expr<vec<3>&>>(a&, temporary scale_expr)
=> add_expr<vec<3>&, scale_expr<vec<3>&>>{
       left  = a&,
       right = owned scale_expr{inner=b&, factor=2.0}
   }
```

求值 `expr[1]` 时递归展开：

```text
add_expr[1]
= at(left,1) + at(right,1)
= a[1] + scale_expr[1]
= 2 + b[1] * 2.0
= 12
```

再加上 `reverse(vec<3>{{10,20,30}})` 时，`reverse_expr` 会按值拥有这个右值 `vec<3>`：

```text
reverse(temp)[0] = temp[2] = 30
reverse(temp)[1] = temp[1] = 20
reverse(temp)[2] = temp[0] = 10
```

完整表达式 `a + b * 2.0 + reverse(temp)` 的 `eval` 结果仍然是 `{39, 32, 25}`。

## `eval`：一次物化，结果拥有数据

`eval` 是表达式树到真实数组的边界：

```cpp
template<expr E>
constexpr auto eval(E&& e) {
    vec<expr_size_v<E>> out{};
    for (std::size_t i = 0; i < out.data.size(); ++i) {
        out[i] = at(e, i);
    }
    return out;
}
```

`eval` 返回新的 `vec<N>`。表达式节点里可能借用了 `a`、`b`，也可能拥有右值临时；这些关系不会泄漏到返回结果。A05 checker 会先 `eval(temp + b)`，再修改 `b`，确认已物化结果不再变化。

## `assign`：先物化，再写回

`assign(out, expr)` 不能直接边读边写 `out`。坏例子：

```cpp
std::array<double, 4> v{1, 2, 3, 4};
for (std::size_t i = 0; i < v.size(); ++i) {
    v[i] = v[v.size() - 1 - i];
}
```

手推：

```text
初始: [1,2,3,4]
i=0: v[0]=v[3] => [4,2,3,4]
i=1: v[1]=v[2] => [4,3,3,4]
i=2: v[2]=v[1] => [4,3,3,4]  // 读到的是已写过的 v[1]
i=3: v[3]=v[0] => [4,3,3,4]
```

正确写法是：

```cpp
template<std::size_t N, expr E>
    requires (N == expr_size_v<E>)
constexpr void assign(vec<N>& out, E&& e) {
    out = eval(std::forward<E>(e));
}
```

这一步先把 `reverse(out)` 按旧值求成临时 `vec<N>`，再整体赋值，所以 `assign(v, reverse(v))` 得到 `{4,3,2,1}`。这个规则也覆盖更一般的重叠别名：只要表达式读到了输出对象，直接写回都可能污染后续读取。

## 实际反例、修正和复验

这些公开包装没有声明 `noexcept`，因此调用者不能以它们满足“不抛表达式”的约束。本章保证的是不吞异常，以及 `assign` 在求值完成后才写目标；不把当前 double 算术的性质升级为更宽泛的异常规格承诺。如何从实际表达式推导条件 `noexcept`，见11/20章。低层 `operator[]` 与数组一样要求 `i < N`；空向量允许参与组合，`eval` 的零次循环不会访问元素。

A05 保留两个观察程序：

- `eager_numeric_reference.cpp`：不用表达式模板，直接算出 `{39,32,25}`，作为数值参考。
- `direct_write_bad.cpp`：故意边读边写反转数组，观察别名污染。

checker 再覆盖完整实现：嵌套临时不会悬垂，左值操作数保持借用，`eval` 返回拥有结果，`assign` 对 `reverse` 别名安全。ASan 验证重点是临时生命周期：如果表达式节点一律保存 `const E&`，`a + reverse(vec<3>{...})` 很容易在求值时读到已经结束生命周期的临时对象。

硬边界也要明确：本章不支持动态长度，不支持混合 element type，不支持矩阵，不提供通用 view 系统，不承诺性能收益。它只要求表达式树的类型、生命周期和别名语义正确。

[A05练习](../exercises/A05_expression_templates/README.md)先跑 eager 数值参考和坏的直接写回反例，再实现延迟表达式。checker 覆盖嵌套临时、左值借用、右值拥有、eval 拥有结果、reverse alias assign 和 ASan。
