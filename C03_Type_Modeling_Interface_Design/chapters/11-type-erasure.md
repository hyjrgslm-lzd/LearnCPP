# 11：类型擦除与拥有型值语义

类型擦除解决一个具体问题：调用者不想知道具体类型，却还想按值保存、复制和调用。动态多态要求类型继承同一个基类；模板要求调用点知道具体类型。类型擦除把具体类型藏在实现对象后面，对外暴露一个普通值类型。

本章实现最小 `AnyShape`。它拥有一个 heap 上的具体对象，并保存一张操作表：

```cpp
struct Ops {
    void* (*clone)(const void*);
    void (*destroy)(void*) noexcept;
    std::string (*name)(const void*);
    Dimensions (*dimensions)(const void*);
};
```

对象本体只有两个指针：

```cpp
class AnyShape {
public:
    AnyShape() noexcept = default;

    template<class T>
    AnyShape(T value)
        : object_(new T(std::move(value))), ops_(&ops_for<T>) {}

    AnyShape(const AnyShape& other)
        : object_(other.object_ ? other.ops_->clone(other.object_) : nullptr),
          ops_(other.ops_) {}

    ~AnyShape() { reset(); }

private:
    void* object_ = nullptr;
    const Ops* ops_ = nullptr;
};
```

这是固定 heap-only 版本，不做 SBO、不做 allocator 参数、不做插件注册。`new T` 负责满足 `T` 的对齐；`destroy` 用 `delete static_cast<T*>(p)` 回收同一个动态类型。这个实现足够讲清拥有、复制、移动、空态和失败保证。

目标类型必须满足本章的 `ShapeObject` 约束：可复制构造、析构不抛异常，有 `name()` 和 `dimensions()`，并且 `dimensions()` 返回正宽高。concept 能检查语法和一部分类型性质，不能证明宽高一定为正。checker 用具体反例覆盖运行语义。

空态必须定义。默认构造和移动后对象为空，`operator bool()` 返回 false。访问空态不运行 UB；本课选择 checked throw：

```cpp
std::string name() const {
    if (!object_) {
        throw std::logic_error("empty AnyShape");
    }
    return ops_->name(object_);
}
```

复制要深复制。若 `AnyShape b = a;`，`b` 拥有自己的目标对象，后续销毁或移动 `a` 不影响 `b`。`clone` 失败时，正在构造的新对象没有完成；已存在的目标对象不能被破坏。赋值用 copy-and-swap 最短也最稳：

```cpp
AnyShape& operator=(const AnyShape& other) {
    if (this != &other) {
        AnyShape copy(other); // may throw
        swap(copy);           // noexcept commit
    }
    return *this;
}
```

移动只转移两个指针，源对象置空。自移动要保持对象可析构、可继续使用；最简单写法是在 `this != &other` 时才重置并偷指针。析构必须不抛，因为它会出现在栈展开和容器清理中。

`const` 派发也要明确。`AnyShape::name() const` 只能拿到 `const void*`，操作表函数也接收 `const void*`，所以 erased 调用不会借 const 接口修改目标。若目标类型内部用 `mutable`，那是目标自己的承诺，本擦除层不额外放大权限。

`std::function` 是标准库里的类型擦除例子。MSVC `<functional>` 的当前本机实现会用内部存储、操作管理器和间接调用处理不同 callable；是否 SBO、阈值多少、何时分配，都是该实现版本的观察，不是 C++ 标准承诺。源码导读只能固定本机版本和文件入口，不能把它当成所有 STL 的布局规则。本课 `AnyShape` 故意不用 SBO，因为本章目标是所有权语义和失败保证。

C++26 的 `std::indirect` 和 `std::polymorphic` 提供标准化间接值语义方向；本课前沿索引只记录本机能力状态。没有实现时，正文仍讲语义，实验标明不可运行。

[练习 L11](../exercises/L11_type_erasure/README.md) 要实现 heap-only `l11::AnyShape`。checker 会覆盖空态访问、const dispatch、深复制、移动、自赋值、析构计数和 copy 失败保持目标不变。坏例分别只破坏 copy-and-swap 或空态检查。

