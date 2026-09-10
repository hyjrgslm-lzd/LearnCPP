# 04. 名字查找、依赖名与 ADL

模板代码里的名字不是在一个时刻一次性找完的。普通函数里写 `f(x)`，编译器通常在看到函数定义时就能决定 `f` 这一串名字指向哪些声明，再用参数做重载决议。模板里多了一层：有些名字和模板参数无关，仍在模板定义点查找；有些名字依赖模板参数，要等实例化时再把实参类型带来的声明加入候选。很多泛型代码的错误都来自把这两个阶段混成一个阶段。

本章先把三种普通查找分开：限定名查找、非限定名查找和 ADL。然后把它们放进模板的两阶段查找里，解释为什么 `this->`、`typename` 和 `template` 不是语法噪声，而是在告诉编译器“这个名字要等模板参数进来后再解释”。最后用一个小练习实现 `c04_lookup::inspect(x)`：成员 `inspect()` 优先，否则走 ADL `inspect(x)`，无路径时在候选集阶段拒绝。

## 从非模板代码看名字来源

限定名写法带着作用域：

```cpp
namespace app {
int value = 1;
}

int x = app::value;
```

`app::value` 只在 `app` 里找。它不会因为局部也有 `value` 就改变含义。限定名适合表达“我知道这个名字在哪个作用域里”，代价是它不会触发 ADL，也就不适合写类型旁扩展点。

非限定名没有显式作用域：

```cpp
int value = 1;

void run() {
    int value = 2;
    (void)value;
}
```

函数体里的 `value` 先看局部作用域，再看外层。找到一层有同名声明后，这个普通查找阶段就停止；外层同名声明不会继续混入。这个“停止”很重要：模板里的 poison pill 就利用它阻止查找跑回 CPO 对象本身。

函数调用的非限定名还有第三步：ADL。写 `inspect(x)` 时，普通查找先找当前作用域可见的 `inspect`，ADL 还会根据实参类型，把关联 namespace 或关联类里的同名函数加入候选。例如：

```cpp
namespace model {
struct Item { int value{}; };
int inspect(Item&) noexcept;
}

void f(model::Item& item) {
    inspect(item);
}
```

即使 `model::inspect` 没有被 `using` 到当前作用域，`model::Item` 的关联 namespace 是 `model`，ADL 仍能找到它。扩展函数、运算符和 hidden friend 都依赖这一点。

## 非依赖名在定义点绑定

模板里不依赖模板参数的名字，定义模板时就查找。后面实例化时，即使调用处有同名函数，也不会补进来。

```cpp
void log(int);

template<class T>
void write(T value) {
    log(0);
    (void)value;
}
```

`log(0)` 里的 `log` 和 `T` 无关，所以它在模板定义点绑定到当时可见的声明。调用者在别的文件里再声明一个更合适的 `log(long)`，不会影响这个模板体。这条规则让模板定义可诊断、可复现；否则同一个模板在不同调用点可能因为周围名字不同而变成不同程序。

依赖名不同。`value.inspect()` 里的成员名是否存在，取决于 `T` 是什么；`inspect(value)` 的 ADL 候选也取决于 `T` 的关联 namespace。它们要等实例化时才能完成检查。

两阶段查找的实用结论是：模板定义点能确定的错误，应尽早报；模板实参才能决定的错误，要留到实例化点。练习里的 checker 会用 `requires` 检查“是否可调用”，就是把依赖表达式关在候选边界里，而不是让函数体深处报硬错误。

## 依赖基类需要 `this->`

模板继承里最常见的查找坑是依赖基类：

```cpp
template<class T>
struct Base {
    int value() const { return 1; }
};

template<class T>
struct Derived : Base<T> {
    int broken() const {
        return value();
    }
};
```

`Base<T>` 是依赖基类。定义 `Derived<T>` 时，编译器还不知道 `Base<T>` 的完整成员集合，因为 `Base` 可能有特化。未限定的 `value()` 不会自动进入依赖基类查找。写成 `this->value()` 后，`this` 的类型依赖 `T`，整个成员访问表达式变成依赖表达式，查找推迟到实例化时。

也可以写 `Base<T>::value()`。这会走限定名查找，表达“我要这个基类作用域里的成员”。但限定写法会影响虚调用和重载集合，教学上先用 `this->` 建立模型：不是为了“更面向对象”，而是为了让查找阶段正确。

## `typename` 与 `template` 是消歧信息

依赖限定名里，编译器不知道后面的名字是类型、对象还是模板。默认规则保守：`T::value_type` 不是已知类型，除非你说它是类型。

```cpp
template<class T>
void use_type() {
    typename T::value_type x{};
    (void)x;
}
```

没有 `typename` 时，编译器不能把 `T::value_type x;` 当成声明稳定解析，因为某个实例化里 `value_type` 也可能是静态数据成员。`typename` 把语法树固定成“这里需要类型”，真实存在性和可访问性仍留到实例化检查。

依赖对象后调用成员模板，也需要 `template`：

```cpp
template<class T>
void use_template(T& object) {
    object.template get<int>();
}
```

`object` 的类型依赖 `T`。没有 `template`，`<` 可能被当成小于号。`template` 告诉解析器 `get` 是模板名，后面的 `<int>` 是模板实参列表。它同样不保证 `get<int>` 一定存在，只负责消除语法歧义。

## ADL 的关联实体与抑制条件

ADL 从实参类型出发找关联 namespace 和关联类。类类型会关联它所在的 namespace、直接和间接基类、模板实参中出现的类型等。枚举关联声明它的 namespace。指针、数组、函数类型会继续看它们指向或组成的类型。这个集合的目的很朴素：扩展函数通常放在类型旁边。

但 ADL 不是无条件启动。未限定调用的普通查找如果找到类成员、块作用域函数声明，或找到的不是函数/函数模板的声明，ADL 会被抑制。CPO 实现里最危险的情况是当前作用域有一个同名变量对象：

```cpp
namespace c04 {
inline constexpr struct inspect_fn {
    template<class T>
    void operator()(T&& object) const {
        inspect(object);
    }
} inspect{};
}
```

这里 `inspect` 既是 CPO 对象名，又想作为 ADL 函数名。未限定查找可能先看到对象名，随后表达式不再按作者期望进入“只找函数”的路径。更糟时，调用会回到 CPO 自己，形成递归或难读的候选错误。

样章 L11 采用的最小隔离手法同样适用于本章：把 ADL 探测放到 detail namespace，并放一个不能匹配的同名函数声明。

```cpp
namespace c04_lookup::detail {
void inspect();

template<class T>
concept adl_inspectable = requires(T&& object) {
    inspect(std::forward<T>(object));
};
}
```

`void inspect();` 让普通查找在 detail 里找到函数名，不会继续向外层爬到 `c04_lookup::inspect` 对象。它没有参数，不能匹配一元调用；真正的一元候选必须来自 ADL。这样既保留 ADL，又不让 CPO 自己污染候选集。

## hidden friend 与访问不是同一件事

hidden friend 写在类体内：

```cpp
namespace model {
struct Token {
    int value{};
    friend int inspect(Token&) noexcept { return 1; }
};
}
```

这个 friend 函数属于围绕 `Token` 的普通函数，但不一定能被 `model::inspect(token)` 这种限定名查找找到。它能通过 ADL 被发现，因为实参类型 `Token` 关联到它的类和 namespace。练习 checker 会覆盖 hidden friend；只写成员路径或限定 namespace 的实现会漏掉它。

访问控制发生在查找之后。一个 private 成员可能被找到，但不可访问；一个 hidden friend 可通过 ADL 找到，但函数体能访问哪些私有成员取决于 friend 关系。不要把“查找不到”和“找到但不可访问”混成一种失败。模板诊断里这两类错误的位置不同：前者通常是候选集为空或约束不满足，后者是访问检查失败。

## 练习目标

L04 要实现 `c04_lookup::inspect(object)`。规则和 L11 的 CPO 形状相同，但本题关注名字查找而不是完整 CPO 设计：

1. 当前 cv/ref 条件下 `object.inspect()` 可调用时，优先调用成员。
2. 成员不可调用时，尝试 ADL `inspect(object)`，必须支持 namespace 自由函数和 hidden friend。
3. 没有合法路径时，`requires { c04_lookup::inspect(object); }` 为 `false`。
4. 返回类型和值类别用 `decltype(auto)` 保留；`noexcept` 来自实际选择的表达式。

配套观察程序先演示 `this->`、`typename` 和 `template` 的正例，再用安全模型展示未隔离 ADL 会接受无路径对象。诊断 case 使用一个缺失 `typename` 的独立源文件：control 证明工具链和 include 正常，subject 预期在 MSVC 报依赖类型名需要 `typename` 的语义错误。这个负例不靠缺头文件或链接失败。
