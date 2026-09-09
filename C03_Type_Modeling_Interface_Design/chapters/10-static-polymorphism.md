# 10：静态多态、约束与 CRTP

静态多态把“某个类型能不能用”放在编译期判断。调用者写模板函数，实参类型在实例化时决定代码。它不需要基类对象，也不需要虚派发；代价是错误发生在模板实例化点，约束必须写清楚。

本课只讲 C03 用到的机制：函数模板、`requires`、concept、必要的两阶段查找，以及 CRTP 的成立前提。复杂名字查找、偏特化和模板元编程留给 C04。

函数模板的主体不是立刻对所有类型检查完整语义。只有被实例化时，依赖于模板参数的表达式才按实际类型检查：

```cpp
template<class T>
Dimensions dimensions_of(const T& shape) {
    return shape.dimensions();
}

Rectangle r;
auto d = dimensions_of(r); // 此处实例化 T = Rectangle
```

若 `T` 没有 `dimensions()`，错误会出现在调用处。concept 把这个要求写到接口上：

```cpp
template<class T>
concept ShapeLike = requires(const T& value) {
    { value.name() } -> std::convertible_to<std::string>;
    { value.dimensions() } -> std::same_as<Dimensions>;
};

template<ShapeLike T>
std::string describe(const T& shape) {
    Dimensions d = shape.dimensions();
    return shape.name() + ":" + std::to_string(d.width) + "x" + std::to_string(d.height);
}
```

这个 concept 只证明语法：表达式存在，返回类型能匹配。它不能自动证明语义公理。C03 的 `ShapeLike` 仍要在正文和 checker 中声明：`dimensions()` 的宽高为正，`name()` 稳定且非空，调用不修改对象。concept 负责把明显不合格的类型挡在编译期；语义仍靠类型作者和测试证据维护。

重载解析会优先选择约束满足的候选。约束越具体，接口越清楚：

```cpp
template<ShapeLike T>
int width_of(const T& shape) {
    return shape.dimensions().width;
}
```

不要写“万能模板里用一堆 if constexpr 猜能力”，除非确实需要兼容多个概念。一个概念能表达当前接口，就用一个概念。

CRTP 用基类模板拿到派生类型：

```cpp
template<class Derived>
class ShapeFacade {
public:
    std::string describe() const {
        const Derived& self = static_cast<const Derived&>(*this);
        Dimensions d = self.dimensions_impl();
        return self.name_impl() + ":" + std::to_string(d.width) + "x" + std::to_string(d.height);
    }
};

class Rectangle : public ShapeFacade<Rectangle> {
public:
    std::string name_impl() const { return "rectangle"; }
    Dimensions dimensions_impl() const { return {3, 4}; }
};
```

`static_cast<const Derived&>(*this)` 只有在对象真的是 `Derived` 子对象时成立。正确写法是 `class Rectangle : public ShapeFacade<Rectangle>`，不要让 `Circle` 继承 `ShapeFacade<Rectangle>`。CRTP 基类也不要在自己的构造或析构函数里调用派生实现；那时派生部分尚未完成构造或已经开始销毁，和动态多态的构造期问题一样危险，只是错误不会由 virtual 派发保护。

模板基类中的名字查找还有一个常见坑：依赖基类成员要写 `this->` 或限定名：

```cpp
template<class T>
struct Base {
    void mark();
};

template<class T>
struct Derived : Base<T> {
    void run() {
        this->mark(); // mark 来自依赖基类
    }
};
```

没有 `this->`，第一阶段查找可能找不到 `mark`。C04 会系统讲两阶段查找；本章只要求你在当前例子里知道为什么要写。

[练习 L10](../exercises/L10_static_polymorphism/README.md) 是观察型程序。它用一个最小 `ShapeLike` concept、一个 CRTP facade 和一个依赖基类 `this->` 例子，展示静态多态如何表达接口承诺。它不是动态多态的性能替代排名，而是另一种扩展契约：调用点编译期知道具体类型。

