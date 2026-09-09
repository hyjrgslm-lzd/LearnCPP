# 05. 特殊成员

C++ 类有六类特殊成员：默认构造、析构、复制构造、复制赋值、移动构造、移动赋值。它们决定对象能不能被默认创建、销毁、复制、移动，以及这些操作怎样处理成员和资源。资源类型写错特殊成员，最常见后果是 double free、leak、浅复制共享可变资源或 moved-from 状态不可析构。

## 六个特殊成员各自负责什么

```cpp
struct X {
    X();                         // 默认构造
    ~X();                        // 析构
    X(X const&);                 // 复制构造
    X& operator=(X const&);      // 复制赋值
    X(X&&) noexcept;             // 移动构造
    X& operator=(X&&) noexcept;  // 移动赋值
};
```

构造函数建立不变量；析构函数释放对象拥有的资源；复制构造从另一个对象建立新对象；复制赋值让已有对象变成另一个对象的值；移动构造从可消费对象转交资源建立新对象；移动赋值把已有对象的资源替换成另一个对象转交来的资源。

“可移动”不等于“一定有 `T(T&&)`”。如果一个类型只有 `T(T const&)`，rvalue 也能绑定到 `const&`，所以 `std::is_move_constructible_v<T>` 可能为 true。这个 trait 只说明 `T(std::declval<T&&>())` 这个构造表达式可行，不能证明真实调用了 move constructor。

## 隐式声明、定义、删除和抑制

你不写特殊成员时，编译器可能隐式声明它们。是否能真正定义，要看成员和基类能否执行对应操作。

```cpp
struct HasReference {
    int& r;
};

// HasReference 的复制构造可复制引用绑定；
// 默认赋值通常会被删除，因为引用成员不能重新绑定。
```

常见规则脉络：

- 没有用户声明构造函数时，编译器可以声明默认构造函数；成员如果没有默认初始化路径，默认构造会被删除。
- 没有用户声明复制构造时，编译器可以逐成员复制构造；若某成员不可复制，对应函数被删除。
- 没有用户声明复制赋值时，编译器可以逐成员赋值；`const` 成员、引用成员、不可赋值成员会让它被删除。
- 用户声明析构、复制构造或复制赋值会影响隐式移动成员生成。资源类型一旦写了析构，就不能假装默认移动仍然自然正确。
- `= default` 要求编译器按规则生成；`= delete` 明确禁用。

```cpp
struct UniqueFd {
    UniqueFd(UniqueFd const&) = delete;
    UniqueFd& operator=(UniqueFd const&) = delete;
    UniqueFd(UniqueFd&&) noexcept = default;
    UniqueFd& operator=(UniqueFd&&) noexcept = default;
};
```

这里的 `= default` 只有在成员本身移动安全时才成立。如果成员是裸 `int fd`，默认 move 只是复制整数，不会把源对象置为无效；通常需要手写 move。

## Rule of 0

Rule of 0 是首选：类不直接管理资源，把资源交给标准库成员。

```cpp
struct User {
    std::string name;
    std::vector<int> scores;
};
```

这个类不需要手写析构、复制、移动。`std::string` 和 `std::vector` 已经实现资源管理。少写特殊成员能减少错误，也让编译器保留最自然的生成规则。

如果你想表达独占资源，用 `std::unique_ptr`、文件句柄封装类等 owner 成员：

```cpp
struct Node {
    std::unique_ptr<Node> next;
};
```

这会自动删除复制、允许移动，行为来自成员类型，不需要自己维护裸指针协议。

## Rule of 5

如果类直接管理资源，析构、复制构造、复制赋值、移动构造、移动赋值必须一起审视。不是每个操作都要允许，但每个都要有决定。

```cpp
class Buffer {
public:
    ~Buffer();
    Buffer(Buffer const&);
    Buffer& operator=(Buffer const&);
    Buffer(Buffer&&) noexcept;
    Buffer& operator=(Buffer&&) noexcept;
private:
    int* data_ = nullptr;
    std::size_t size_ = 0;
};
```

只写析构不写复制，默认复制会逐成员复制 `data_`。两个 `Buffer` 指向同一块数组，析构时 double delete。只写 move 不处理源对象，源对象析构也可能释放已经转交出去的资源。

## 深复制和强异常边界

深复制要求新对象拥有独立资源：

```cpp
Buffer a{1, 2, 3};
Buffer b = a;
a[0] = 9;
// b[0] 仍是 1
```

复制构造可以直接分配新资源并复制元素。复制赋值更难，因为目标对象已经拥有旧资源。安全顺序是先取得新资源，成功后释放旧资源并提交新状态：

```cpp
Buffer& Buffer::operator=(Buffer const& other) {
    if (this == &other) {
        return *this;
    }
    int* fresh = allocate_and_copy(other.data_, other.size_); // 可能抛
    delete[] data_;                                          // fresh 成功后再释放旧资源
    data_ = fresh;
    size_ = other.size_;
    return *this;
}
```

也可以用 copy-and-swap，但核心仍是：失败不能破坏原目标对象；成功后不能泄漏旧资源。

## 移动：转交资源，不承诺标准库源对象为空

移动构造通常接管源对象资源，再把源对象放进可析构、可赋值状态：

```cpp
Buffer::Buffer(Buffer&& other) noexcept
    : data_(std::exchange(other.data_, nullptr)),
      size_(std::exchange(other.size_, 0)) {}
```

移动赋值要先释放目标旧资源，再接管源资源，同时处理 self-move：

```cpp
Buffer& Buffer::operator=(Buffer&& other) noexcept {
    if (this == &other) {
        return *this;
    }
    delete[] data_;
    data_ = std::exchange(other.data_, nullptr);
    size_ = std::exchange(other.size_, 0);
    return *this;
}
```

本课自定义 `IntBuffer` 约定 moved-from 后为空，是为了让练习能精确检查状态。标准库类型通常只保证 moved-from 对象 valid but unspecified。比如 `std::string s; auto t = std::move(s);` 之后，不能断言 `s.empty()` 一定为 true；只能析构、赋值，或调用该类型说明允许的操作。

## noexcept 为什么属于特殊成员设计

`noexcept` 是移动构造/移动赋值的重要契约。标准容器扩容时要维护异常保证。若元素 move constructor 可能抛，而 copy constructor 可用，容器可能复制旧元素；若 move 是 `noexcept`，容器通常可以移动旧元素。

```cpp
static_assert(std::is_nothrow_move_constructible_v<Buffer>);
```

这个断言只适合你的类型确实能无抛移动时使用。对裸资源句柄，移动通常只是交换指针/编号，可以 `noexcept`。对移动时要分配内存的类型，不要假写 `noexcept`。

## L05 练习怎样验证

L05 的 `IntBuffer` 是真实资源题，但资源由可信 fixture 管理，避免学生通过伪造报告过关。

checker 拥有这些观察：

- 分配槽位数、释放槽位数、无效释放次数。
- 每个 buffer 的大小和值。
- copy 后两个 buffer 是否拥有独立槽位。
- move 后资源是否转交、源对象是否进入本课约定的空状态。
- copy/move assignment 是否释放目标旧资源。

bad variants 覆盖两个典型错误：浅复制共享同一槽位、move assignment 覆盖目标句柄导致旧资源泄漏。student 占位实现能独立构建，但会在 checker 的真实操作下失败；它不能填写 `implemented` 标记，也不能调用 reference。

## 自测

1. 为什么有析构函数的裸资源类不能依赖默认复制？
2. `std::is_move_constructible_v<T>` 为 true 是否说明 `T(T&&)` 被调用？
3. 深复制和共享所有权的差别是什么？
4. 为什么复制赋值要先分配新资源，再释放旧资源？
5. 为什么本课能断言自定义 `IntBuffer` moved-from 后为空，却不能断言 `std::string` moved-from 后为空？

## 解析

1. 默认复制会逐成员复制裸句柄，两个对象可能释放同一个资源。
2. 不能。`const T&` 复制构造可以绑定 rvalue，trait 只证明构造表达式可行。
3. 深复制创建独立资源；共享所有权让多个对象共同拥有同一资源，需要引用计数或其他共享协议。
4. 如果分配或复制抛异常，目标对象仍保持原状态；先释放旧资源会让失败路径破坏对象。
5. `IntBuffer` 是本课定义的类型，移动后空是它的契约；标准库 moved-from 对象通常只保证 valid but unspecified。
