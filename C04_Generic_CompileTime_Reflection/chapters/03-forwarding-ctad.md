# 03. 转发与 CTAD：保存调用者给你的形态

泛型包装函数最容易犯的错，是把自己当成透明层，实际却改变了调用。调用者传来左值，你把它移动了；调用者传来右值，你在函数体里把它当左值；底层函数返回引用，你拷成了值；底层本来 `noexcept`，你丢掉异常规格；构造对象时模板实参本该由构造参数推出，CTAD 却走了另一条候选。转发和 CTAD 都是在处理同一个问题：从调用现场保留足够信息，让后续选择仍然符合原始意图。

先分清 `T&&` 的两种身份。若 `T` 是函数模板参数，并且形参直接写成 `T&&`，它是 forwarding reference：

```cpp
template<class T>
void wrapper(T&& value);
```

左值实参会让 `T` 推成 `U&`，右值实参会让 `T` 推成 `U`。随后发生引用折叠：`U& &&` 折成 `U&`，`U&& &&` 折成 `U&&`。四条折叠规则可以记成一句话：只要出现左值引用，结果就是左值引用；只有 `&& &&` 仍是右值引用。

但不是所有 `T&&` 都是 forwarding reference：

```cpp
template<class T>
struct box {
    void set(T&& value); // T 已经由类模板固定，不是从这个函数形参推导
};
```

在 `box<std::string>` 里，`set` 的参数就是 `std::string&&`，它只接受右值。它不是万能引用。判断方法很机械：`T` 是否是当前函数模板的待推导参数，形参是否直接是 `T&&`。中间套了 `const T&&`、`std::vector<T>&&`、类模板已固定的 `T&&`，都不是 forwarding reference。

`std::move` 和 `std::forward` 也要分清。`std::move(x)` 无条件把表达式转成右值，意思是“允许从它移动”。`std::forward<T>(x)` 按 `T` 恢复值类别；当 `T` 来自本次推导，或你明确保存了调用者的 cv/ref 形态时，它是把原始调用形态交给下一层的工具。普通已确定的右值引用成员、没有推导来源的 `T&&`，不会因为用了 `std::forward` 就自动变成 forwarding reference；用错 `T` 还会伪造值类别。

```cpp
template<class F, class... Args>
decltype(auto) call(F&& f, Args&&... args)
    noexcept(noexcept(std::forward<F>(f)(std::forward<Args>(args)...)))
{
    return std::forward<F>(f)(std::forward<Args>(args)...);
}
```

这个 wrapper 有四个契约。`F&&` 和 `Args&&...` 保存 callable 和参数的值类别；`std::forward` 把它们恢复给真实调用；`decltype(auto)` 保存返回引用或值；`noexcept(noexcept(...))` 让异常规格和底层表达式一致。少掉任一项，checker 都可以构造一个只在该点失败的类型。

花括号初始化是转发的边界。裸 `{1, 2, 3}` 没有类型，模板参数包不能直接从它推导：

```cpp
template<class T>
void take(T&&);

take({1, 2, 3}); // 不可推导
```

若形参明确是 `std::initializer_list<T>`，编译器才有规则从列表元素推导 `T`。所以接受列表和透明转发不是一回事。许多容器的 `emplace`、工厂函数和 CTAD guide 都要为 initializer_list 单独写入口，否则 `{}` 会选择不到候选，或者选到和你以为不同的构造函数。

CTAD 是 class template argument deduction。写 `std::pair p{1, 2.0};` 时，编译器从构造函数和 deduction guide 合成一组候选，推导出 `std::pair<int, double>`。对聚合类，C++20 起也能从聚合初始化形成隐式 guide：

```cpp
template<class T>
struct aggregate_box {
    T value;
};

aggregate_box b{42}; // aggregate_box<int>
```

有构造函数时，隐式 guide 来自构造函数形参；你也可以写显式 guide：

```cpp
template<class T>
struct holder {
    T value;
    explicit holder(T v) : value(std::move(v)) {}
};

template<class T>
holder(T) -> holder<T>;
```

deduction guide 参与的是推导候选，不是运行时构造代码。它能改变类模板实参的选择，但对象仍按构造函数构造。多个 guide、构造函数和 initializer_list 候选同时存在时，会进入正常重载决议。initializer_list 构造在列表初始化中优先级很高，所以 `box{1,2,3}` 常会走列表路径；如果你不想支持列表，就不要用一个过宽的 guide 假装支持。

练习 L03 分两块。第一块实现 `invoke_preserving`，它只做透明调用，必须保留参数值类别、返回引用和异常规格。第二块实现一个小 `holder<T>`，让 `holder h{42}` 推成 `holder<int>`，让 `holder hs{std::initializer_list<int>{1,2,3}}` 或 `make_list_holder({1,2,3})` 明确得到拥有型 `std::vector<int>`。这里故意把裸 `{}` 放到专门函数里处理，避免让转发包装承担它推导不了的输入。

自测：`template<class T> void f(T&&); int x; f(x);` 里的参数类型最终是什么？`T=int&`，形参 `T&&` 折成 `int&`。`std::move(x)` 和 `std::forward<T>(x)` 在这里一样吗？不一样，前者一定把 `x` 当右值，后者按 `T=int&` 转回左值。再问：为什么 `return (local);` 配合 `decltype(auto)` 危险？在 C++20 前它可能按左值推成引用而悬垂；在 C++23 的 P2266R3 规则下，move-eligible 的 return operand 可能被当成 xvalue，结果变成右值引用或绑定失败。无论哪个版本，它都不是返回局部对象的安全写法。
