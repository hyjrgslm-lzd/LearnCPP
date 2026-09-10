# 02. 推导规则：`T`、`auto` 和 `decltype` 到底记住了什么

模板推导的核心问题是：调用者给了一个表达式，模板参数 `T` 应该被推成什么。很多错误来自把三个东西混在一起：对象的声明类型、表达式的值类别、模板形参写法。`int x` 的声明类型是 `int`，表达式 `x` 却是左值；`const int cx` 的声明类型带 `const`，但按值传参时顶层 `const` 会被复制丢掉。`auto` 和 `decltype` 看起来都在“推类型”，规则却不一样。

先从函数模板形参开始。若形参是 `T value`，这是按值接收。实参数组会退化成指针，函数名会退化成函数指针，顶层 cv 和引用都会被去掉：

```cpp
template<class T>
void by_value(T value);

int x = 0;
const int cx = 0;
int a[3]{};

by_value(x);  // T = int
by_value(cx); // T = int
by_value(a);  // T = int*
```

按值参数创建自己的对象。它不可能保留调用者对象的引用身份，所以推导也不保留顶层引用。数组退化也在这里发生，因为函数参数类型不能真正是按值数组。

若形参是 `T&`，实参必须是左值，`T` 会保留底层 cv：

```cpp
template<class T>
void by_lvalue_ref(T& value);

by_lvalue_ref(x);  // T = int
by_lvalue_ref(cx); // T = const int
```

`T&` 不是转发引用。它只接受左值。`const T&` 又是另一条规则：它能绑定左值、const 左值和临时对象，但 `T` 自身通常不带顶层 const，因为 const 已经写在形参上。理解这一点很关键，否则你会误以为 `const T&` 能告诉你调用者原来是不是 const 对象。

数组和函数若用引用接收，就不退化：

```cpp
template<class T, std::size_t N>
void by_array_ref(T (&arr)[N]);

int data[3]{};
by_array_ref(data); // T = int, N = 3
```

这里 `N` 是非类型模板参数，由数组边界推导出来。很多固定容量接口、span 构造和编译期字符串处理都靠这类形参保留数组长度。函数类型也一样，`R (&)(Args...)` 能保留函数类型，`T value` 只能拿到指针。

非推导上下文是另一类常见坑。不是所有出现 `T` 的地方都参与推导。典型例子是 `std::type_identity_t<T>`：

```cpp
template<class T>
void sink(std::type_identity_t<T> value);

sink(1);      // 不能从 type_identity_t<T> 推出 T
sink<int>(1); // 可以，T 已显式给出
```

这条规则用于避免某些位置反向推导造成歧义。它也能故意用来“冻结”一个类型，让它由别的参数或显式模板实参决定。后续讲重载和 Concepts 时，你会看到非推导上下文常用于让接口错误更可控。

`auto` 的推导大体像函数模板的按值 `T`。`auto a = cx;` 得到 `int`，不是 `const int`。`auto& r = cx;` 得到 `const int&`。`const auto& cr = 1;` 绑定临时，`auto&& f = x;` 对局部变量初始化时也会按转发引用那套折叠规则得到左值引用。花括号是特殊点：`auto xs = {1, 2, 3};` 会推成 `std::initializer_list<int>`，而模板参数 `T` 通常不能从裸 `{1,2,3}` 推导出来。

`decltype` 完全不是这套。`decltype(name)` 对未加括号的 id-expression 返回声明类型；`decltype((expr))` 按表达式值类别返回引用类型。具名变量表达式是左值，所以：

```cpp
int x = 0;
int& r = x;

decltype(x)  a = 0; // int
decltype(r)  b = x; // int&
decltype((x)) c = x; // int&
```

这正是 `decltype(auto)` 容易误用的原因。普通表达式里，`decltype(value)` 看声明类型，`decltype((value))` 看表达式值类别，具名变量表达式通常是左值。`return` 语句还要叠加版本规则：WG21 P2266R3 进入 C++23 后，return operand 中符合条件的 move-eligible id-expression 会被当成 xvalue；P2266R3 给出的例子里，`decltype(auto) { return (x); }` 对局部 `int x` 在 C++20 常推成 `int&`，在 C++23 推成 `int&&`，而显式返回 `int&` 的类似写法还可能变成 ill-formed。局部引用变量、非局部对象和不是 move-eligible 的表达式又要分开判断。

结论没有变：不要用 `decltype(auto)` 返回本地对象或本地对象的括号表达式来“保留类型”。C++20 的风险常是悬垂左值引用；C++23 还可能变成右值引用或直接绑定失败。薄 wrapper 返回下层表达式时适合 `decltype(auto)`；计算出一个本地结果再返回时，多数情况应该用普通 `auto` 或明确返回类型。

本章练习 L02 让你实现几个小型探针函数。`by_value_token` 要展示按值推导会丢顶层 cv/ref 并让数组退化；`by_ref_token` 要保留左值引用下的 cv；`array_extent` 要通过数组引用拿到元素类型和长度；`identity_decltype_auto` 要保留传入引用身份；`freeze_as<T>` 要展示非推导上下文需要显式指定目标类型。checker 不靠打印字符串，而是用 `std::same_as` 和地址检查消费真实类型。

自测：`const int c = 1; auto a = c; decltype(c) b = 1;` 中 `a` 和 `b` 的类型一样吗？不一样，`a` 是 `int`，`b` 是 `const int`。再问：`decltype((c))` 是什么？它是 `const int&`，因为 `(c)` 是一个左值表达式。最后问：`template<class T> void f(T); int arr[4]; f(arr);` 里的 `T` 是数组吗？不是，是 `int*`；要保留长度必须用引用形参。
