# 03：`std::optional` 与显式空状态

空状态如果是业务状态，就应进入类型。用 `-1` 表示“没有用户 ID”会把两个问题混在一起：`-1` 是否可能是合法 ID，以及每个调用者是否都记得检查。`std::optional<T>` 把状态集合写成“没有值”或“有一个 `T`”，让调用点先处理状态，再取得对象。

本章只解决“缺失是否是正常结果”。如果失败需要携带错误原因，比如“格式错误、权限不足、重复 ID”，`optional` 不够，下一章之后用 `std::expected<T, E>`。如果对象有多个互斥形态，而不是“有/无”两种状态，用 `std::variant`。选错类型会让接口撒谎：`optional<User>` 只能说“没有用户”，说不出“数据库断开”；`variant<Rectangle, Circle>` 能说当前是哪种图形，不能说这次编辑为什么失败。

## 拥有状态与对象寿命

`optional<T>` 拥有一个可选的 `T` 子对象。空状态时没有 `T` 对象；有值时 `T` 的生命期已经开始；`reset()` 或赋成 `nullopt` 会结束这个 `T` 的生命期。这个语义来自 C02 的对象生命期，不是指针规则。

```cpp
std::optional<std::string> name;
name.emplace("Ada");       // string 对象在 optional 内部构造
std::string& ref = *name;   // ref 只在 name 仍有值时有效
name.reset();              // string 析构，ref 失效
```

所以 `optional` 适合返回一个值快照，例如 C03 文档模型里的 `find(id) -> optional<Element>`：找到就复制一个 `Element` 给调用者，没找到就是空。这个返回值不借用 `Document` 内部存储；调用者拿到后，后续 `Document` 修改不会让这个快照悬垂。

反过来，如果接口想表达“可能借用某个已有对象”，C++23 的 `std::optional<T&>` 不存在。C++23 可用指针、迭代器、`std::reference_wrapper<T>` 外包在 `optional` 里，或者设计成回调访问。每种方式都必须说清借用对象活多久。

## 构造、转换与 `emplace`

`optional<T>` 可以从 `T` 构造，也可以用 `std::in_place` 或 `emplace()` 在内部直接构造对象。构造和赋值都受 `T` 的构造、转换、移动、拷贝能力约束。`optional<std::unique_ptr<int>>` 可移动但不可复制；`optional<Widget>` 是否能从 `int` 构造，取决于 `Widget` 是否能从 `int` 构造。

```cpp
std::optional<std::string> a = std::string{"abc"};
std::optional<std::string> b{std::in_place, 3, 'x'};
b.emplace(5, 'y'); // 先销毁旧 string，再构造新 string
```

`emplace(args...)` 的状态变化很关键：若原来有值，它会先销毁旧对象，再尝试构造新对象。新构造若抛异常，`optional` 变为空。这个结果对接口设计很实用：`optional` 本身仍合法，但旧值不保留。若调用者需要“构造新值失败时旧值仍在”，不能直接在原对象上 `emplace`，应先在临时 `optional` 或临时 `T` 中准备，成功后再交换或赋值。

```cpp
std::optional<Widget> current = Widget::make_old();
try {
    current.emplace(input); // Widget(input) 抛出时，current 没有 Widget
} catch (...) {
    // current.has_value() 可能已经是 false；这是 emplace 的正常失败边界。
}
```

## 访问：unchecked 与 checked observer

`optional` 的布尔上下文只检查是否有值：

```cpp
if (auto user = find_user(id)) {
    show(user->name());
}
```

`*opt` 和 `opt->member` 是 unchecked observer，前提是 `opt.has_value()` 为真。违反前提是程序错误。它们不做运行时错误通道，适合已经由控制流证明有值的局部代码。

`value()` 是 checked observer：有值时返回内部对象，空状态时按定义抛 `std::bad_optional_access`。它适合表达“按本函数契约此处必须有值，空值说明上层违约”。普通业务分支用 `if (opt)` 更清楚，因为它把空状态作为正常分支处理，不把正常缺失塞进异常路径。

`value_or(default_value)` 返回值而不是引用。它还有一个常见陷阱：参数会先求值，再进入函数调用。即使 `opt` 有值，`expensive_default()` 也已经运行。

```cpp
auto label = maybe_label.value_or(expensive_default()); // default 总会先求值
```

另一个边界是值类别。对左值 `optional<T>` 调用 `value_or` 会复制有值对象；对右值调用会移动有值对象。默认值也要能转换成 `T`。如果默认值构造很贵，先用 `if` 分支；如果要保留内部对象引用，不能用 `value_or`。

## C++23 monadic 操作

C++23 给 `optional` 增加 `and_then`、`transform`、`or_else`。它们把“先判断是否有值，再调用下一步”的分支写成一条链，但不改变状态语义。

`and_then(f)` 只在有值时调用 `f(value)`，且 `f` 必须返回另一个 `optional<U>`。它用于可能继续失败的下一步：

```cpp
auto zip = find_user(id)
    .and_then([](const User& user) { return user.address(); })
    .and_then([](const Address& address) { return address.zip_code(); });
```

`transform(f)` 只在有值时调用 `f(value)`，并把返回值包装成 `optional<U>`。它用于不会新增空状态原因的映射：

```cpp
auto display_name = find_user(id).transform([](const User& user) {
    return user.name() + "#" + std::to_string(user.id());
});
```

`or_else(f)` 只在空状态时调用 `f()`，且 `f` 必须返回同类型 `optional<T>`。它适合 fallback 查找：

```cpp
auto user = find_local(id).or_else([&] { return find_remote_cache(id); });
```

这些操作都短路：空的 `optional` 不调用 `and_then` 和 `transform` 的函数；有值的 `optional` 不调用 `or_else` 的函数。回调若抛异常，异常直接传播；`optional` 不会把异常转换成空状态。若需要“异常或业务错误也成为错误值”，进入 `expected` 或显式 `try/catch`。

回调参数的 cv/ref 和值类别随调用对象传播。对 `std::move(opt).transform(...)`，回调可接收右值并移动内部值；对 `const optional<T>&`，回调只能观察 `const T&`。这不是语法细节，而是所有权设计：链式调用是否消费值，要从调用表达式看出来。

## 可空返回不是业务错误

用 `optional` 的判断句是：调用者问的问题允许“没有”作为普通答案，且不需要知道更多原因。典型例子：

- `find(id)`：没有这个 ID。
- `front()` 的安全包装：容器为空。
- “解析可选字段”：字段不存在，不是格式错误。

不要用 `optional` 吞掉需要处理的失败：

```cpp
std::optional<UserId> parse_user_id(std::string_view text); // 信息不足
```

如果 `text` 可能为空、含非数字、越界、ID 为 0，每个原因都可能影响提示、日志或恢复策略。这时签名应变成 `expected<UserId, ParseUserIdError>`。`optional` 的空状态没有负载，强行用日志或全局错误补充原因，会让错误通道离开类型系统。

## C++26：`optional<T&>` 与 0/1 range

C++26 纳入 `optional<T&>`，它表达“可空、非拥有引用”。它不是 C++23 标准能力；本课把它作为前沿知识讲清，真实本机能力由 `../exercises/F01_frontier` 的 `c26_optional_ref.cpp` 探测。

`optional<T&>` 的关键语义是非拥有与重绑定。它不创建 `T`，只引用别处已经存在的 `T`；赋给另一个引用时，`optional` 重新绑定到新对象，而不是给旧对象赋值。它还拒绝绑定临时对象，否则会制造立即悬垂的引用。

```cpp
int a = 1;
int b = 2;
std::optional<int&> r = a; // C++26
r = b;                    // r 现在引用 b，不是 a = b
*r = 3;                   // b 变成 3
```

C++26 还让 `optional<T>` 成为一个 0/1 range：空时迭代 0 个元素，有值时迭代 1 个元素。这让它能进入 ranges 管线，把“可能有一个值”当作短序列处理。这个能力不表示 `optional` 借用了外部范围；`optional<T>` 仍拥有内部 `T`。borrowed range 关系要按标准和具体类型判断，不能因为能 `for` 就假设从临时 `optional` 取出的迭代器可长期保存。

这条桥接会在 C06 ranges 中继续展开。本章只要求你能说明：`optional` 的 range 能力改变的是访问形式，不改变有/无状态、不改变对象所有权，也不把业务错误变成空序列。

## L03 练习

`L03_optional` 是观察型练习，目标不是补空函数，而是验证 unchecked/checked 访问和短路语义。运行程序会检查：

- `optional` 有值时对象生命期开始，`reset()` 后内部对象析构。
- `emplace` 构造失败后 `optional` 进入空状态。
- `value()` 空状态按定义抛 `std::bad_optional_access`，`*`/`->` 需要先证明有值。
- `value_or` 的默认实参会被求值，右值 `optional` 可移动出内部值。
- C++23 `and_then/transform/or_else` 的返回要求、短路和异常传播。
- C++26 `optional<T&>` 与 optional range 只作为前沿能力说明，真实支持由 F01 探测。

完整解析：当“没有”是结果集合的一员时，用 `optional` 能把检查集中到类型和控制流；当失败原因会影响恢复策略时，空状态太窄，应使用带错误负载的类型。`optional` 拥有对象，所以它能安全返回值快照；借用语义必须单独声明。
