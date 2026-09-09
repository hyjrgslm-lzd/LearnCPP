# 02. 表达式与引用

对象有类型和生命期；表达式有类型、值类别和值。值类别回答“这个表达式怎样参与初始化、绑定和重载”：它是标识一个已有对象，还是表示可被移动的已有对象，还是描述一个将要初始化目标的值。

## 三个主值类别

C++17 后标准主线把值类别压成三个核心分支：lvalue、xvalue、prvalue。

```cpp
std::string s = "abc";

s;              // lvalue：名字表达式标识已有对象
std::move(s);   // xvalue：仍标识 s，但表达式可绑定到 T&&
std::string{};  // prvalue：用于初始化结果对象的纯右值
```

`std::move(s)` 不移动 `s`。它只是把表达式转换成 xvalue。后续如果选择了 move constructor 或 move assignment，那个函数才可能修改 `s`。

“表达式是 lvalue”也不等于“类型是引用”。`s` 的类型是 `std::string`，表达式值类别是 lvalue。一个函数返回 `T&` 时，调用表达式是 lvalue；返回 `T&&` 时，调用表达式是 xvalue；返回 `T` 时，多数上下文是 prvalue。

```cpp
int global = 0;
int& lref();
int&& rref();
int value();

lref();  // lvalue expression
rref();  // xvalue expression
value(); // prvalue expression
```

## glvalue、rvalue 和 materialization

lvalue 和 xvalue 都是 glvalue，因为它们关联对象、位域或函数。xvalue 和 prvalue 都是 rvalue，因为它们能绑定到 rvalue reference 或参与移动相关重载。

prvalue 在 C++17 后不总是“先造临时再复制”。同类型 prvalue 可以直接初始化最终目标：

```cpp
T make() {
    return T{}; // 直接初始化返回对象
}
```

当 prvalue 需要被当作对象访问时，会发生 temporary materialization conversion：

```cpp
T{}.member();       // materialize 后访问成员
T const& r = T{};   // materialize 后绑定引用，并可能延长生命期
```

materialization 只说明对象实体出现了，不自动说明生命期能延长多久。03 章会列出哪些引用绑定能延长、哪些不会。

## 引用绑定规则

非 const lvalue reference 只能绑定到 lvalue：

```cpp
int i = 0;
int& a = i;  // OK
// int& b = 1; // ill-formed
```

`T const&` 可以绑定到 lvalue、xvalue、prvalue。绑定到 prvalue 时，若处在标准允许的上下文，临时生命期可能延长：

```cpp
int const& c = 1; // 临时 int 延长到 c 的生命期结束
```

`T&&` 可以绑定到 xvalue 或 prvalue：

```cpp
int&& r = 1;
int&& m = std::move(i);
```

引用不是 owner。`T const&` 延长临时生命期是一条特定规则，不是“引用拥有对象”。把引用传参、返回、存入 view 或跨异步边界时，要重新检查 owner 是否还活着。

## overload resolution 如何看值类别

重载可以按引用类型区分调用对象：

```cpp
void use(std::string&);        // 可修改 lvalue
void use(std::string const&);  // 可读借用
void use(std::string&&);       // 可消费 rvalue

std::string s;
use(s);            // 选 & 或 const&，更匹配 &
use(std::move(s)); // 选 &&
use(std::string{}); // 选 &&
```

这解释了 API 设计的常见形状：只读不保存用 `T const&` 或 `std::string_view`；需要接管资源用 `T&&` 或按值接收；需要修改调用方对象用 `T&`。

## auto 推导

`auto` 变量声明按模板值传递的近似规则推导，会丢掉顶层 cv 和引用：

```cpp
int x = 1;
int& r = x;
const int c = 2;

auto a = r; // int，复制 x 的值
auto b = c; // int，顶层 const 丢掉
```

要保留引用，需要显式写 `auto&` 或 `auto&&`：

```cpp
auto& rr = r;       // int&
auto const& cr = c; // int const&
```

`auto&&` 在变量声明中常是“按 initializer 推导引用”：

```cpp
auto&& a = x; // int&，因为 x 是 lvalue
auto&& b = 1; // int&&
```

但不要把它当成 owner。`auto&& temp = make_string();` 会延长临时到 `temp` 的生命期；`auto&& ref = id(make_string());` 是否安全取决于 `id` 有没有返回悬空引用。

## decltype 与 decltype(auto)

`decltype` 有两套规则。未加括号的 id-expression 得到声明类型；一般表达式按值类别决定引用：

```cpp
int x = 0;

decltype(x) a = 1;    // int
decltype((x)) b = x;  // int&，因为 (x) 是 lvalue expression
```

函数返回类型也一样：

```cpp
int global = 0;

int value() { return global; }
decltype(auto) ref() { return (global); } // int&
```

`decltype(auto)` 很锋利。它能保留引用，适合转发包装；也能把局部对象引用返回出去：

```cpp
// decltype(auto) bad() {
//     int local = 0;
//     return (local); // 推导为 int&，悬空
// }
```

03 章会把这种“类型推导保留引用但 owner 已死”的情况作为生命周期问题处理。

## 引用折叠和 forwarding reference

模板参数推导中的 `T&&` 有特殊规则，通常叫 forwarding reference：

```cpp
template <class T>
void wrapper(T&& value) {
    target(std::forward<T>(value));
}
```

如果传入 lvalue `std::string s`，`T` 推导为 `std::string&`，参数类型经引用折叠变成 `std::string&`。如果传入 rvalue，`T` 推导为 `std::string`，参数类型是 `std::string&&`。

引用折叠规则：只要组合里出现 `&`，结果就是 `&`；只有 `&&` 和 `&&` 折叠仍是 `&&`。

函数体内的 `value` 是有名字的变量表达式，因此它总是 lvalue。`std::forward<T>(value)` 按 `T` 的推导结果恢复传入时的值类别。漏掉 `std::forward`，会把 rvalue 也当 lvalue 传下去；乱用 `std::move`，会把 lvalue 调用方对象也当作可消费对象。

## L02 练习怎样验证

L02 仍以观察和编译反例为主。

- `l02_observation` 用 `decltype` 和重载观察 lvalue、xvalue、prvalue、`auto`、`decltype(auto)`、forwarding reference。
- `l02_negative_bind_rvalue_to_nonconst_lvalue` 编译 `int& ref = 1;`，要求失败。
- `l02_negative_forward_lvalue_to_rvalue` 展示函数体内命名的 `T&&` 参数是 lvalue；漏 `std::forward<T>` 时无法传给只接收 `T&&` 的目标。

这些测试只证明当前源码和工具链的可观察行为；正文结论按 C++23 主线规则解释。

## 自测

1. `std::move(s)` 是否移动了 `s`？
2. `T{}` 是对象、表达式，还是值类别？
3. `decltype(x)` 和 `decltype((x))` 对变量名 `x` 有什么差别？
4. 为什么 forwarding reference 函数体里还要 `std::forward<T>(value)`？
5. `auto a = ref;` 为什么常常不是引用？

## 解析

1. 没有。它只把表达式转成 xvalue；真正移动发生在后续 move 构造或 move 赋值中。
2. `T{}` 是表达式；在多数上下文中是 prvalue。需要绑定引用或访问成员时才物化对象。
3. `decltype(x)` 得到声明类型；`decltype((x))` 按表达式规则，变量名加括号是 lvalue，所以得到 `T&`。
4. 因为 `value` 这个名字在函数体中是 lvalue 表达式。`std::forward<T>` 根据推导结果恢复原调用的 lvalue/rvalue 性质。
5. `auto` 值声明按值推导，会丢掉顶层 cv 和引用并复制/移动初始化新对象。要保留引用需写 `auto&`、`auto&&` 或用 `decltype(auto)`。
