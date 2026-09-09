# 03. 生命周期与借用

C++ 里很多对象不直接拥有资源：指针、引用、iterator、`std::span`、`std::string_view` 和大多数 ranges view 都只是入口。入口本身可以复制、保存、传递；它能否使用，取决于被指向的对象是否仍在生命期内，以及原来的访问范围是否仍然有效。

本章的主线是 owner 和 borrower。owner 负责对象或资源何时开始、何时结束；borrower 不延长 owner 的生命期，除非它触发了标准明确列出的临时生命期延长规则。

## 存储期不是生命期

存储期说明一段字节能保留多久；生命期说明某个类型的对象何时在这段存储中存在。很多局部变量里两者看起来同步：

```cpp
void f() {
    std::string s = "abc"; // 进入声明后对象生命期开始
}                           // 离开作用域，析构后生命期结束
```

但它们不是同一个概念。动态存储、placement new、union 成员切换、临时对象和部分构造失败都会让“字节还在”和“对象还活着”分离。借用者关心的是对象生命期，不是地址是否看起来还能打印。

```cpp
std::string* p = new std::string("abc");
std::string_view v = *p;
delete p;          // 字符串对象生命期结束
// v 还保存指针和长度，但已经悬空
```

悬空借用不一定马上崩溃。它可能读到旧值、乱码、另一个对象的数据，或在优化后表现完全不同。因此练习不会用“运行悬空程序看是否崩溃”当验证方法。

## 作用域结束与析构

自动对象按作用域结束逆构造顺序销毁：

```cpp
void f() {
    A a;
    B b;
} // 先销毁 b，再销毁 a
```

这给借用关系一个自然约束：后声明对象可以安全借用先生命期更长的对象，前提是不要把借用逃出作用域。

```cpp
void ok() {
    std::string text = "abc";
    std::string_view view = text;
    use(view); // text 仍活着
}
```

如果 borrower 活得比 owner 久，就会悬空：

```cpp
std::string_view bad;
{
    std::string text = "abc";
    bad = text;
} // text 结束
// bad 悬空
```

## 临时对象和完整表达式

临时对象通常在创建它的 full-expression 结束时销毁：

```cpp
std::string_view v = std::string("abc");
// 这一行结束时 string 临时销毁，v 悬空
```

full-expression 大致是一条完整语句、初始化器或控制语句中的完整表达式。临时销毁点不是“变量名离开作用域”这么简单。`std::string_view v` 这个 view 活着，但它借用的 `std::string` 临时已经在初始化语句结束时销毁。

## 临时生命期延长：正例和边界

`const T& r = T{};` 会把临时对象生命期延长到 `r` 的生命期结束：

```cpp
T const& r = T{}; // T{} 延长到 r 的生命期结束
```

这个规则不是“所有引用都延长”。它只在标准列出的绑定位置生效。聚合里有引用成员时，初始化语法会改变结论：

```cpp
struct Holder {
    T const& r;
};

Holder a{T{}}; // brace aggregate initialization：临时延长到 a 的生命期结束
Holder b(T{}); // paren aggregate initialization：临时只活到完整表达式结束
```

`Holder a{T{}}` 是安全延长的正例。`Holder b(T{})` 是 C++20 起可写的 paren aggregate initialization，但引用成员绑定到临时时没有同样的延长效果。它不是因为 `Holder` 坏，而是因为延长规则匹配的语法位置不同。

`new` 初始化还有另一条边界：

```cpp
auto* p = new Holder{T{}}; // 引用成员悬空：临时不会延长到动态对象生命期
```

动态 `Holder` 对象活着，不代表初始化它时出现的 `T{}` 临时会跟着动态对象走。这里 `p->r` 在完整表达式结束后悬空。

返回引用也不会把临时交给调用者：

```cpp
T const& bad_return() {
    return T{}; // C++23：不会延长给调用者；调用者拿到悬空引用
}
```

C++26 方向通过 P2748 把“返回语句中把返回引用绑定到临时表达式”的一部分情况改成 ill-formed，但本课主线固定在 C++23：不要写、不要运行、不要靠警告。

引用参数只把临时延长到包含调用的 full-expression 结束，不能二次转交：

```cpp
T const& id(T const& x) {
    return x;
}

T const& r = id(T{}); // T{} 只覆盖这条初始化语句；r 随后悬空
```

把引用传给另一个函数、再返回、再存入对象，都不能重新获得临时延长资格。判断方法是问：临时是否直接绑定在标准允许延长的目标上？如果中间经过函数参数，通常已经失败。

## initializer_list 的双重边界

`std::initializer_list<T>` 常被说成“视图”：它通常保存指向编译器生成数组的指针和长度。这个说法有用，但不能推成“initializer_list 从不延长底层数组”。

直接用 braced-init-list 初始化一个 `initializer_list` 对象时，底层数组会延长到这个 `initializer_list` 对象生命期结束：

```cpp
std::initializer_list<int> xs = {1, 2, 3}; // 底层数组跟 xs 一起活
for (int n : xs) { use(n); }               // OK
```

复制 `initializer_list` 不会复制元素，也不会再次延长底层数组：

```cpp
std::initializer_list<int> saved;
{
    std::initializer_list<int> local = {1, 2, 3};
    saved = local; // 只复制指针和长度
}
// saved 指向的数组已经结束生命期
```

函数调用同理：

```cpp
void take(std::initializer_list<int> xs);
take({1, 2, 3}); // 底层数组覆盖这次调用
```

如果 `take` 保存 `xs.begin()` 或保存 `xs` 到调用之后使用，就保存了悬空借用。`initializer_list` 的元素本身还是 `const`，它也不是可修改数组的 owner。

## range-for 的 C++23 补强

range-for 会把 range initializer 绑定到一个隐藏变量。C++23 通过 P2718/P2644 方向延长 for-range-initializer 中某些临时对象，让常见链式写法覆盖整个循环：

```cpp
for (char ch : std::string("abc")) {
    use(ch); // C++23：range initializer 的 string 覆盖循环
}
```

也可以理解为循环内部有一个隐藏的 `auto&& range = std::string("abc");`，这个绑定让临时 string 活到循环结束。

但补强不穿透已经结束生命期的函数参数对象。官方 wording 示例区分了按引用返回和按值参数返回引用的情况：

```cpp
std::vector<int> make_vector();
std::vector<int> const& ok(std::vector<int> const& v) { return v; }
std::vector<int> const& bad(std::vector<int> v) { return v; }

for (int x : ok(make_vector())) {
    use(x); // C++23：make_vector() 的返回临时可覆盖循环
}

// for (int x : bad(make_vector())) {
//     use(x); // 不安全：bad 返回对按值参数 v 的引用，v 已在 callee 结束
// }
```

C++20/23 在这里有版本差异；当前练习默认按 C++23 构建，并且不运行仍会悬空的变体。安全代码不能依赖“某个编译器扩展刚好没炸”。

## lambda、闭包和协程帧

lambda 按引用捕获不会延长被捕获对象生命期：

```cpp
auto make_bad() {
    int local = 1;
    return [&] { return local; }; // 返回后 local 已死
}
```

lambda 对象本身可以活得很久；悬空的是闭包对象内部保存的引用。按值捕获会把值存进闭包对象：

```cpp
auto make_ok() {
    int local = 1;
    return [local] { return local; }; // 闭包拥有一份 int
}
```

协程 lambda 多一层边界。调用协程 lambda 后，执行状态在协程帧里；如果帧里保存的是对闭包成员的引用，而闭包对象先销毁，稍后恢复协程就会访问已死闭包成员。典型问题是“协程帧借用了已死闭包成员”，不是简单的“闭包不保活协程帧”。安全写法是在协程帧中拥有需要跨 suspend 使用的数据，或保证闭包对象活到协程完成。

## view 不拥有底层对象

`std::string_view` 和 `std::span` 是常见 borrower：

```cpp
void parse(std::string_view text); // text 必须在调用期间有效
void fill(std::span<int> out);     // out 指向的元素必须仍有效
```

安全用法：

```cpp
std::string text = load_text();
parse(text); // parse 不保存 view；调用期间 text 活着

std::vector<int> values(8);
fill(values); // fill 不保存 span；调用期间 values 活着
```

危险用法：

```cpp
std::string_view v = std::string("abc"); // string 临时结束后 v 悬空

std::vector<int> values = {1, 2, 3};
std::span<int> s = values;
values.push_back(4); // 可能重分配，旧 span 失效
```

Ranges view 也类似。`borrowed_range` 是 ranges 里一个更精细的概念，用来描述 range 对象销毁后 iterator/sentinel 是否仍可用；它不表示 view 会保活底层容器，也不替代 owner 设计。

## 接口设计：把 owner 和 borrow 写进类型

接口如果保存数据，优先接收 owner 或自己复制：

```cpp
class Document {
public:
    explicit Document(std::string text) : text_(std::move(text)) {}
private:
    std::string text_;
};
```

接口如果只在调用期间读取，可以接收 borrower：

```cpp
int count_words(std::string_view text); // 不保存 text
```

接口如果返回数据，要避免返回对局部对象、临时对象或按值参数的引用：

```cpp
std::string make_name();             // 返回 owner
std::string_view view_name(Config&); // 返回借用，Config 必须活着且不重分配底层存储
```

文档要写清楚 borrower 是否会被保存、保存到何时、哪些操作会使它失效。后续协程、Ranges、Execution 都在扩展这条主线：跨异步边界的数据必须被拥有，或者借用边界必须被类型和协议严格限定。

## L03 练习怎样验证

L03 不运行真实悬空访问。它用一个可信 fixture 模拟 owner slot、generation 和 alive 状态：

- `Owner` 拥有一个槽位和 generation。
- `BorrowedInt` 只能保存槽位编号和 generation，不能拥有值。
- checker 创建 owner、取得 borrow、修改 owner、让 owner 失效，再检查 borrow 是否拒绝访问。
- bad variant 故意忽略 generation/alive，只保存槽位，因此在 owner 失效后仍读旧槽位；checker 用固定诊断拒绝它。
- student 占位实现独立构建，但默认行为明确失败，不调用 reference，也没有 `implemented` 标记。

这套模型不是替代真实 C++ 生命周期规则；它把“借用必须受 owner 生命期约束”做成可重复、不会 UB 的练习。

## 自测

1. `std::string_view v = make_string();` 为什么危险？
2. `Holder h{T{}};` 和 `Holder h(T{});` 对引用成员绑定临时有什么差别？
3. 复制 `std::initializer_list<int>` 是否复制底层数组或再次延长生命期？
4. C++23 range-for 补强为什么救不了 `bad(std::vector<int> v) -> const&`？
5. 引用捕获 lambda 返回到外部后，可能悬空的是 lambda 对象还是被捕获对象？
6. `std::span<int>` 是否让 `std::vector<int>` 活得更久？

## 解析

1. `string_view` 借用字符，不拥有 `std::string`。`make_string()` 的返回临时结束后，view 仍有指针但没有活着的字符串。
2. brace aggregate initialization 中引用成员直接绑定临时，可把临时延长到 aggregate 对象生命期；paren aggregate initialization 没有这条延长效果。
3. 不会。直接 braced-init-list 初始化 `initializer_list` 对象会延长底层数组；复制 `initializer_list` 只复制指针和长度。
4. 因为返回引用指向 callee 的按值参数对象，参数在函数返回时已经结束生命期。range-for 只能延长 for-range-initializer 中仍符合规则的临时，不能复活已死参数。
5. lambda 对象本身可能仍活着；悬空的是闭包里保存的引用所指向的局部对象。协程场景还可能是协程帧稍后借用已死闭包成员。
6. 不会。`span` 只保存指针和长度，vector 销毁、清空或重分配后，旧 span 的对应借用失效。
