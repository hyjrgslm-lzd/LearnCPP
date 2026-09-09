# 09：动态多态、切片与 clone

动态多态把“调用哪个实现”推迟到运行期。基类提供稳定接口，派生类提供不同实现。调用者持有 `Shape&`、`Shape*` 或 `std::unique_ptr<Shape>`，不需要知道对象实际是 `Rectangle` 还是 `Circle`。

最小基类要把三件事说清楚：可调用操作、析构责任、复制策略。

```cpp
class Shape {
public:
    virtual std::string name() const = 0;
    virtual Dimensions dimensions() const = 0;
    virtual std::unique_ptr<Shape> clone() const = 0;
    virtual ~Shape() = default;
};
```

`virtual` 让调用通过动态类型派发。`override` 让编译器检查派生函数确实覆盖了基类虚函数。`final` 可以关闭继续派生：不是性能承诺，而是接口承诺，表示这个派生类型的行为边界到此为止。

```cpp
class Rectangle final : public Shape {
public:
    std::string name() const override { return "rectangle"; }
    Dimensions dimensions() const override { return {width_, height_}; }
    std::unique_ptr<Shape> clone() const override {
        return std::make_unique<Rectangle>(*this);
    }

private:
    int width_ = 1;
    int height_ = 1;
};
```

虚析构和拥有指针必须一起看。`std::unique_ptr<Shape>` 析构时会通过 `Shape*` 删除对象；若 `Shape::~Shape()` 不是 virtual，派生析构不会按契约执行，资源释放和统计都会错。若接口从不拥有对象，可以用引用或不拥有指针表达借用，不要让删除责任含糊。

对象切片是动态多态里最常见的值语义陷阱：

```cpp
void store_by_value(ShapeBase shape); // 只能保存基类子对象

Rectangle r;
ShapeBase s = r; // Rectangle 部分被切掉
```

一旦按基类值复制，只剩基类那一段。抽象基类能阻止一部分切片，因为不能构造 `Shape` 值；但只要基类不是抽象的，切片就可能发生。动态多态的值容器通常存 `std::unique_ptr<Shape>`，复制时用 `clone()` 创建新的动态对象。

`clone()` 是“复制动态类型”的接口。它不能返回 `this` 的别名，也不能总是返回某个固定派生类型。正确 clone 的结果与源对象观察值相同，地址独立，动态类型保持一致：

```cpp
std::unique_ptr<Shape> copy = shape.clone();
check(copy.get() != &shape);
check(copy->name() == shape.name());
```

RTTI 提供运行期类型查询。`dynamic_cast<Rectangle*>(p)` 失败时返回 `nullptr`；`dynamic_cast<Rectangle&>(r)` 失败时抛 `std::bad_cast`。它适合处理少数必须按具体类型分支的边界，例如调试、迁移或兼容旧接口。若主体逻辑到处 `dynamic_cast`，通常说明基类缺少一个真正稳定的虚操作，或继承层次不该承担这项变化。

多继承和虚基类也是标准语义，不是某个编译器的 vtable 布局承诺。多继承可以表达“同时满足多个接口”：

```cpp
struct Drawable { virtual void draw() const = 0; virtual ~Drawable() = default; };
struct Measurable { virtual Dimensions dimensions() const = 0; virtual ~Measurable() = default; };
struct Rectangle : Drawable, Measurable { /* ... */ };
```

虚基类解决菱形继承中的共享基类子对象问题。它影响构造责任和对象模型，但标准不要求你依赖具体指针偏移、vptr 数量或内存布局。教学代码只讨论语义边界：谁拥有基类子对象、哪个构造函数初始化虚基、哪些接口能替换调用。

[练习 L09](../exercises/L09_dynamic_polymorphism/README.md) 要实现 `Shape`、`Rectangle`、`Circle` 和 `clone()`。checker 会拒绝空 clone、错误动态类型、非独立复制和基类删除未触发派生析构的实现。练习目标是把动态多态的拥有边界写完整。

