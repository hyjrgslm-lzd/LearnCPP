# 08：组合、继承与替换性

第 07 章把接口看成公开承诺。本章把承诺放到两种代码关系里：组合和继承。组合表达“我有一个成员”；继承表达“我可以当成那个基类使用”。这两个关系都能复用代码，但语义完全不同。

组合是默认选择。一个图形文档元素有 `ElementId` 和 `Shape`，所以它持有两个成员：

```cpp
class Element {
public:
    Element(ElementId id, Shape shape) : id_(id), shape_(std::move(shape)) {}

    ElementId id() const noexcept { return id_; }
    const Shape& shape() const noexcept { return shape_; }

private:
    ElementId id_;
    Shape shape_;
};
```

`Element` 不应继承 `ElementId`。元素不是 ID；元素只是有一个 ID。若用继承，调用者可能把 `Element` 当作纯 ID 使用，绕开元素自己的不变量。组合把内部辅助能力留在内部，外部接口只暴露业务需要的观察。

继承适合替换性。若 `Rectangle` 公开继承 `Shape`，意思不是“复用 `Shape` 的代码”，而是“任何只要求 `Shape` 契约的地方，`Rectangle` 都能替换进去”。这就是 LSP 的核心：派生类不能让基类调用者的合法程序失效。

替换性主要看前置条件、后置条件和不变量：

- 派生类不能收紧前置条件。若 `Shape::draw(Canvas&)` 允许任意非空画布，`FancyShape::draw()` 不能要求画布必须先启用阴影。
- 派生类不能放松后置条件。若 `Shape::dimensions()` 承诺宽高为正，派生类不能返回 0。
- 派生类必须维护基类不变量。若基类说 `name()` 非空，派生类不能在某个状态返回空字符串。

违反替换性的代码常常能编译：

```cpp
struct Shape {
    virtual Dimensions dimensions() const = 0; // width > 0, height > 0
    virtual ~Shape() = default;
};

struct HiddenShape : Shape {
    bool revealed = false;

    Dimensions dimensions() const override {
        if (!revealed) {
            return {0, 0}; // 破坏 Shape 的后置条件
        }
        return {3, 4};
    }
};
```

调用者按 `Shape` 契约写 `area(shape) = width * height`，它不应额外知道 `HiddenShape` 的内部开关。派生类如果需要更强前置条件，通常不该放进这个继承层次；用组合包一层，或把能力拆成另一个接口。

`public` 继承表达 is-a。`private` 继承不表达替换性，只是实现复用和定制访问。大多数时候组合更清楚：

```cpp
class LabelledRectangle {
public:
    Dimensions dimensions() const { return rect_.dimensions(); }

private:
    Rectangle rect_;
    std::string label_;
};
```

`private Rectangle` 继承会让读者猜它是不是某种 Rectangle；组合直接说明它拥有一个 Rectangle。

NVI（non-virtual interface）常用于把基类不变量固定在非虚函数里，把可变步骤放到私有虚函数里：

```cpp
class ShapeView {
public:
    Dimensions dimensions() const {
        Dimensions d = do_dimensions();
        if (d.width <= 0 || d.height <= 0) {
            throw std::logic_error("invalid dimensions");
        }
        return d;
    }

    virtual ~ShapeView() = default;

private:
    virtual Dimensions do_dimensions() const = 0;
};
```

外部只调用 `dimensions()`。基类统一做检查，派生类只实现 `do_dimensions()`。这不能替代正确的派生实现，但能把公开承诺集中在一个入口。

构造和析构阶段不要依赖虚派发完成派生行为。C++ 在基类构造函数中调用虚函数时，不会派发到尚未构造好的派生部分；在基类析构函数中调用虚函数时，派生部分已经开始销毁，也不会按完整派生对象使用。下面代码不会调用 `Rectangle::name()`：

```cpp
struct Shape {
    Shape() { log(name()); }          // 调用 Shape::name
    virtual std::string name() const { return "shape"; }
    virtual ~Shape() = default;
};

struct Rectangle : Shape {
    std::string name() const override { return "rectangle"; }
};
```

需要构造后的派生行为，就提供工厂或显式 `init()`，并让对象先完整构造。需要析构记录，就把责任放在派生析构函数或外部拥有者中。

析构接口也属于类型契约。若调用者会通过 `Shape*` 或 `std::unique_ptr<Shape>` 删除对象，基类析构必须是 `virtual`。若基类只是非拥有借用视图，可以把析构设为 `protected` 非虚，禁止通过基类删除：

```cpp
class ShapeBorrow {
protected:
    ~ShapeBorrow() = default;
};
```

选择哪一种，要看接口是否承担删除责任。不要把“当前没有 delete”当作省略 virtual 析构的理由；公开拥有指针一旦出现，非虚析构就是接口漏洞。

[练习 L08](../exercises/L08_composition/README.md) 是观察型程序。它把同一个矩形标签模型分别写成组合和继承反例，观察 `public` 继承、`private` 继承、NVI 检查和构造期虚调用的行为。重点不是背语法，而是判断类型关系有没有公开替换承诺。

