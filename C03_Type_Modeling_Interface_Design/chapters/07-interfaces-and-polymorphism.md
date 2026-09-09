# 07：接口形状、借用与最小公开面

接口设计先问调用者需要什么稳定承诺，再决定参数、返回值和成员限定。C03 到这里已经有两个前提：01 章要求公开入口保护不变量，06 章要求失败后状态有明确保证。本章把这些要求落到函数签名上。

签名不是语法装饰。`void set_name(std::string name)`、`std::string name() const`、`std::string_view name_view() const` 暴露的是三种不同契约。第一个按值接收，调用者可以传临时对象，函数内部拥有一份参数；第二个按值返回，调用者得到 snapshot，之后对象怎么改都不影响返回值；第三个返回借用，调用者必须知道它依赖 owner 的生命期和修改规则。

## 参数：值、借用和接管

参数按三类看最清楚：

- `T value`：函数获得一份值。适合小值、需要保存副本、或要用 move 接管的输入。
- `const T&` / `std::span<const T>` / `std::string_view`：函数只借用调用者的数据，不保存到调用之后。
- `T&&` 或专门的 owner 类型：函数表达接管或消耗，调用后源对象进入有效但未指定或被定义的 moved-from 状态。

错误反例是把借用保存进对象：

```cpp
class User {
public:
    void set_name(std::string_view name) { name_ = name; } // wrong if name_ is string_view

private:
    std::string_view name_;
};
```

如果调用者传 `set_name(std::string{"tmp"})`，函数返回后临时字符串销毁，`name_` 悬垂。正确接口要么复制成 `std::string`，要么明确要求调用者保证外部存储活得更久，并把类型命名为 view/borrower。

## 返回值：snapshot 与 view

返回值也按两类看：

```cpp
std::vector<int> snapshot() const;              // caller owns returned value
std::span<const int> view() const & noexcept;   // caller borrows from lvalue owner
std::span<const int> view() && = delete;        // temporary owner cannot lend view
```

`snapshot()` 返回独立值。它可能分配和抛异常，但返回后不受 owner 后续修改影响。`view()` 返回借用，通常不分配，可以 `noexcept`，但只在 owner 活着且没有让 view 失效的修改前有效。

从 L06 的 `Table::view()` 可以看到边界：`replace_all_strong(table.view())` 必须先 prepare，因为修改自身可能让旧 view 失效。L07 更进一步：对临时对象调用 `Owner{1,2}.view()` 没有稳定 owner，接口应直接删除 rvalue 调用，让错误停在编译期。

## `const`、ref 限定和最小公开接口

`const` 成员承诺不修改对象的可观察值。它不等于线程安全，也不等于返回的借用永远有效。`view() const &` 同时表达两件事：不修改 owner；只能从 lvalue owner 借用。

ref 限定用于保护调用对象类别：

```cpp
std::span<const int> view() const & noexcept;
std::span<const int> view() && = delete;
```

这比在文档里写“不要对临时对象调用 view”更小、更可靠。编译负例证明接口确实拒绝临时借用。

最小公开接口只暴露调用者需要的操作。`Owner` 若只要读数据，就公开 `view()` 和 `snapshot()`；不公开可变内部 `std::vector<int>&`，不公开 `data()` 加 `size()` 两套重复入口，不公开“为了测试”破坏不变量的 setter。测试可以通过 checker fixture 完成。

## `noexcept` 与异常保证

`noexcept` 是“这个函数不抛异常”的类型级承诺，不是“失败后状态可用”。`snapshot()` 可以提供 strong guarantee：若复制失败，原对象不变；但它仍可能抛异常，所以不能标 `noexcept`。`view()` 只构造一个 `std::span`，不分配，不复制元素，可以声明 `noexcept`。06 章的 `swap` 适合作为提交点，也是因为它不抛。

反例：

```cpp
std::vector<int> snapshot() const noexcept { return values_; } // wrong
```

复制 `values_` 可能分配并抛异常。错误的 `noexcept` 会把异常变成 `std::terminate()`，调用者失去错误通道。

## 练习

L07 实现 `l07::Owner`：

```cpp
Owner(std::initializer_list<int>);
std::span<const int> view() const & noexcept;
std::span<const int> view() && = delete;
std::vector<int> snapshot() const;
void replace_all(std::vector<int> values) noexcept;
```

Part 1：`view()` 从 lvalue owner 返回借用，内容和 owner 当前值一致，并且声明为 `noexcept`。

Part 2：`snapshot()` 返回独立值。拿到 snapshot 后再修改 owner，snapshot 不变。

Part 3：`replace_all()` 按值接收新值，再 `swap` 到内部存储。调用者可以传临时 vector；函数不分配，只交换，声明 `noexcept`。

Part 4：删除 rvalue `view()`。编译负例会尝试 `Owner{1, 2}.view()`，期望编译失败。这个负例不运行真实悬垂程序。

Part 5：坏实现会让 `snapshot()` 返回空快照或让 rvalue `view()` 可用。checker 必须拒绝代表性缺陷。

解析：`view()` 是借用，所以只给 lvalue；`snapshot()` 是值，所以修改 owner 后不变；`replace_all()` 的参数按值进入函数，提交点是 `swap`；`noexcept` 只用于不会抛的入口，不能用来描述 strong guarantee。
