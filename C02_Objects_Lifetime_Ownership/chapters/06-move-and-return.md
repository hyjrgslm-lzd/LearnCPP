# 06. 移动与返回

移动、返回值消除和容器迁移经常被混成“编译器会优化掉”。这会导致两个错误：把 `std::move` 当成真实移动，把 NRVO 当成能让不合法代码合法。正确模型是：表达式值类别决定候选，构造/赋值函数执行真实转移，返回语句有特定消除规则，容器根据异常规格选择迁移策略。

## std::move 只是转换

`std::move(x)` 等价于把 `x` 转成 rvalue reference 对应的 xvalue 表达式。它不访问资源，不修改 `x`，也不保证后续会调用 move constructor。

```cpp
std::string s = "abc";
auto&& r = std::move(s); // s 还没有被移动，只是 r 是可绑定到右值引用的表达式结果
std::string t = std::move(s); // 这里构造 t，可能修改 s
```

真实移动发生在被调用的函数里：

```cpp
Buffer::Buffer(Buffer&& other) noexcept
    : data_(std::exchange(other.data_, nullptr)),
      size_(std::exchange(other.size_, 0)) {}
```

如果没有可用 move constructor，rvalue 也可能走 copy constructor：

```cpp
struct CopyOnly {
    CopyOnly(CopyOnly const&);
};

CopyOnly a;
CopyOnly b(std::move(a)); // 可复制，不证明发生了移动
```

因此不要用 `std::is_move_constructible` 或“代码编译了”断言真实移动。要观察真实移动，必须让类型记录 move constructor 是否被调用，或让 copy 不可用。

## const 对移动的影响

move constructor 通常接收 `T&&`，因为它要修改源对象，把源资源置为空或其他可析构状态。`std::move` 作用在 `const T` 上得到 `const T&&`，不能绑定到 `T&&`：

```cpp
const Buffer c = make_buffer();
// Buffer b = std::move(c); // move-only 类型会失败；可复制类型会尝试复制
```

如果类型有 `T(T const&)`，上面可能调用复制构造。如果类型删除复制，只能报错。L06 的 negative `const_move_only` 就验证这条边界。

实践规则：不要把稍后要转交资源的局部对象声明成 `const`。`const` 适合表达不会修改；移动语义需要允许源对象改变到 moved-from 状态。

## C++17 同类型 prvalue 的必然消除

C++17 起，同类型 prvalue 初始化目标对象时，很多场景没有中间临时对象：

```cpp
struct Token {
    Token();
    Token(Token const&) = delete;
    Token(Token&&) = delete;
};

Token make() {
    return Token{}; // OK：直接构造返回对象
}
```

这里不是“优化器省略了 move”。语言规则直接把 `Token{}` 用来初始化函数返回对象，所以不需要 copy/move 构造可用。这条规则常叫 guaranteed copy elision，但更准确的直觉是：同类型 prvalue 没有先生成一个独立临时再搬运。

同样：

```cpp
Token x = Token{}; // OK
```

## NRVO 是允许优化，不是合法性规则

named local 返回不同：

```cpp
Token make_bad() {
    Token local;
    return local; // copy/move 都删除时 ill-formed
}
```

编译器允许做 NRVO，把 `local` 直接构造成返回对象；但 NRVO 是可选优化。程序语义仍需要存在可用的 copy/move 路径，不能靠“编译器大概率会优化”让代码合法。L06 的 negative `return_named_deleted` 同时包含 `return Token{};` 正例和 `return local;` 反例，确保不会把两者混掉。

写 `return std::move(local);` 通常还会阻止 NRVO：

```cpp
T make() {
    T local;
    return std::move(local); // 强制把表达式变成 xvalue；通常需要 move
}
```

如果 move 很便宜且你明确要这样做，可以写；但“为了优化返回”给 named local 加 `std::move` 通常是反效果。

## C++23 implicit move

返回局部自动对象或参数时，标准有 implicit move 规则：符合条件的 id-expression 会被当成 move-eligible。C++23 简化了这套规则，使一些返回局部和参数的场景更一致。

```cpp
T make(T value) {
    return value; // value 可作为移动候选
}
```

implicit move 不等于 NRVO。它仍需要可用的 move 或 copy 构造。它也不适用于任意表达式：

```cpp
return value.member; // 是否 move-eligible 取决于规则，不要按“局部相关都移动”理解
```

判断返回语句时先分类：

1. `return T{};`：同类型 prvalue，C++17 起直接构造返回对象。
2. `return local;`：named local，可能 NRVO；若不 NRVO，则按 implicit move/copy 规则。
3. `return std::move(local);`：xvalue 表达式，不走 NRVO，要求 move/copy 路径。
4. `return some_reference;`：返回引用时检查被引用对象生命期，不涉及对象返回消除。

## noexcept、move_if_noexcept 和容器增长

标准容器增长时，旧元素要迁移到新存储。若 move constructor 可能抛，而 copy constructor 可用，容器为了保持异常保证可能复制旧元素。`std::move_if_noexcept` 把这条策略暴露成库工具：

```cpp
T& x = element;
auto&& candidate = std::move_if_noexcept(x);
```

如果 `T` 的 move constructor 是 `noexcept`，结果通常是 `T&&`；如果 move 可能抛且 copy 可用，结果可能是 `T const&`。所以资源类型的 move constructor/assignment 在只交换句柄时应写 `noexcept`。

这也解释了 L06 的观察测试：它用自定义 `Tracked` 记录 `std::vector` 扩容迁移时是否使用 move。测试只对本课类型的计数作判断，不用 `std::string` moved-from 是否为空这类非契约状态当证据。

## moved-from 状态

移动后的对象必须仍可析构，并满足类型文档允许的操作。自定义资源类型最好给 moved-from 状态一个简单不变量，例如“空”：

```cpp
Buffer b = make_buffer();
Buffer c = std::move(b);
assert(b.empty()); // 只有当 Buffer 文档承诺 moved-from 为空时才可断言
```

标准库类型通常只承诺 valid but unspecified。它们能析构、能赋新值；其他查询是否有固定结果，要看具体类型说明，不能用某次实现观察推广成标准规则。

## L06 练习怎样验证

L06 是观察和编译反例：

- `l06_observation` 用 `Tracked` 计数验证：`return T{}` 不调用 copy/move；`return local` 可能 NRVO，也可能 move；`return std::move(local)` 会走 move；`std::vector` 对 `noexcept` move 类型扩容时能使用 move。
- `l06_negative_return_named_deleted` 验证同类型 prvalue 返回合法，而 named local 在 copy/move 删除时必须被拒绝。
- `l06_negative_const_move_only` 验证 `std::move(const T)` 不能调用 `T(T&&)`，move-only 类型会编译失败。

测试不会用 `std::is_move_constructible` 断言真实 move，也不会检查标准库 moved-from 字符串是否为空。

## 自测

1. `return T{};` 为什么不需要 copy/move？
2. `return local;` 在 copy/move 都删除时为什么不合法？
3. `std::move(const T&)` 为什么常常不能移动？
4. 为什么给 `return local;` 加 `std::move` 常常不是优化？
5. 为什么 `noexcept` 会影响 `std::vector` 扩容时复制还是移动？

## 解析

1. C++17 起，同类型 prvalue 直接初始化返回对象，没有中间临时需要复制或移动。
2. NRVO 是可选优化；程序语义仍需要可用的复制或移动路径。copy/move 都删除时，不能靠可选优化让程序合法。
3. move constructor 通常接收 `T&&`，需要修改源对象；`const T&&` 不能绑定到它。可复制类型可能退回复制，move-only 类型会失败。
4. `std::move(local)` 把表达式变成 xvalue，通常阻止 NRVO，并要求 move 路径。若目标是返回局部对象，直接 `return local;` 更符合语言规则。
5. 容器增长要维护异常保证。move 可能抛而 copy 可用时，复制旧元素可能更安全；move 是 `noexcept` 时，移动更容易满足保证。
