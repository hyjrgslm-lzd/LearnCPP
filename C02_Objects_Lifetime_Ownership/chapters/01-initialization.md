# 01. 初始化

初始化不是“给已有变量赋值”的同义词。声明先决定类型、存储期和名字；初始化决定对象生命期怎样开始、初始值怎样形成、哪些转换被允许、初始化是否必须在编译期完成。后面章节会把对象交给构造、析构、复制、移动和借用规则；本章先把“对象怎样开始”讲清楚。

## 声明、存储和值的起点

一条声明至少包含类型和声明符：

```cpp
int count;
std::string name;
```

执行到自动存储期局部变量声明时，程序取得存储并开始对象生命期。类类型对象会运行构造；标量类型不一定写入确定值。静态存储期对象在程序启动阶段处理：先零初始化，再进入常量初始化或动态初始化路径。

“有对象”不等于“值可读”。`int x;` 会创建一个 `int` 对象，但自动存储期标量 default-initialization 不写确定值。读取这个值在 C++23 主线中落入 indeterminate/undefined 边界，本课不运行这种程序。C++26 方向引入 erroneous behavior 来描述部分未初始化读，见 P2795；这不会把 C++23 代码里的未初始化读变成可依赖的值。

```cpp
void f() {
    int x;      // 对象存在；值不适合读取
    int y{};    // 对象存在；值为 0
}
```

## default-initialization

没有 initializer 时，通常是 default-initialization：

```cpp
int n;              // 自动标量：不初始化值
std::string s;      // 类类型：调用默认构造，得到空字符串
Widget w;           // 调用 Widget::Widget()
```

对类类型，default-initialization 会执行默认构造函数。对自动存储期的 `int`、指针、枚举等标量，它不提供确定值。对静态存储期对象，即使写作 `int global;`，也会先零初始化，因为存储期规则先发生。

这解释了一个常见误判：

```cpp
int global; // 静态存储期：先零初始化
void g() {
    int local; // 自动存储期：值未初始化
}
```

同样的 `int name;`，存储期不同，结果不同。

## value-initialization 与零初始化

空花括号或某些 `T()` 形式会触发 value-initialization：

```cpp
int a{};          // 0
int b = int();    // 0
std::string s{};  // 默认构造，结果为空
```

对标量，value-initialization 给零值。对类类型，它会按规则先零初始化再调用默认构造，或直接调用构造。实践里，局部标量默认写 `T{}` 比 `T name;` 更安全，因为它把初始状态写进代码。

不要把“零初始化”当成所有初始化形式的别名。`T{}` 是否为零，取决于 `T`；类类型可以在默认构造里建立任何合法不变量。

## copy/direct/list 初始化

C++ 有多种初始化语法，它们会影响 overload resolution、`explicit` 构造函数和窄化检查。

```cpp
T a = expr;    // copy-initialization
T b(expr);     // direct-initialization
T c{expr};     // direct-list-initialization
T d = {expr};  // copy-list-initialization
```

copy-initialization 不是必然复制对象。它描述初始化规则入口，编译器仍可直接构造目标对象。区别在于候选构造函数和转换能否被使用。例如 copy-initialization 不允许用 `explicit` 构造函数做隐式转换，而 direct-initialization 可以直接选择它。

```cpp
struct Port {
    explicit Port(int value) : value(value) {}
    int value;
};

Port a(80);     // OK
Port b{80};     // OK
// Port c = 80; // ill-formed：explicit 不参与这种隐式初始化
```

## list-initialization 与窄化

花括号列表会拒绝 narrowing conversion：

```cpp
int a = 3.14;  // 允许转换，结果截断
int b{3.14};   // ill-formed：narrowing
```

narrowing 不是编译器洁癖。它把“可能丢失信息”的转换放到调用点暴露出来。它也适用于函数实参、返回对象、aggregate 成员初始化等 braced-init-list 场景。

```cpp
void set_retry_count(int);
// set_retry_count({3.14}); // ill-formed
```

如果确实要截断，写显式转换：

```cpp
int b{static_cast<int>(3.14)};
```

这样审阅者能看出丢失小数部分是作者的决定。

## aggregate initialization

aggregate 是一种可以按子对象列表初始化的类或数组。C++20 后 aggregate 条件比早期版本宽一些，但主线规则仍是：不能有用户声明或继承来的构造函数等会破坏 aggregate 的条件，初始化按 aggregate element 顺序匹配。

```cpp
struct Packet {
    int id;
    double weight;
};

Packet p{1, 2.5};              // id=1, weight=2.5
Packet q{.id = 2, .weight = 3.5}; // C++20 designated initializer
```

designated initializer 在 C++23 中只能命名直接非静态数据成员，并且顺序必须和声明顺序一致：

```cpp
// Packet bad{.weight = 1.0, .id = 2}; // ill-formed：顺序错误
```

有基类的 aggregate 可以用位置初始化基类子对象：

```cpp
struct Base { int a; };
struct Derived : Base { int b; };

Derived d{{1}, 2}; // Base{1}, b=2
```

但 C++23 不能用成员 designator 穿过基类写 `Derived{.a=1, .b=2}`。P2287 是后续标准方向，提议让 designated initialization 覆盖基类/间接成员；本课程默认不把它当成 C++23 能力。

## static、dynamic initialization 与 constinit

静态存储期变量初始化分阶段：

```cpp
int zero;                    // 先零初始化；值为 0
constinit int ready = 42;    // 必须静态初始化
int runtime_value();
int late = runtime_value();  // dynamic initialization
```

constant initialization 可以在程序进入动态初始化阶段前完成。dynamic initialization 需要执行代码，跨翻译单元的动态初始化相对顺序容易产生问题。`constinit` 的作用是禁止变量走动态初始化路径：

```cpp
int runtime_value();
// constinit int x = runtime_value(); // ill-formed
```

`constinit` 不是 `const`：

```cpp
constinit int counter = 0;
void tick() { ++counter; } // OK
```

它也不是 `constexpr`。`constexpr` 变量是 `const` 对象并要求可用于常量表达式；`constinit` 只约束初始化阶段，变量本身可改。

## cv 限定和初始化

`const`、`volatile` 是类型限定。初始化 `const` 对象时必须给出能建立值的 initializer，之后不能通过该对象名修改：

```cpp
const int limit = 4;
// const int missing; // ill-formed：const 标量需要初始化
```

`volatile` 让访问保持可观察，适合某些内存映射寄存器场景，但它不是线程同步。多线程共享数据需要 atomic、mutex 或更高层同步。

cv 会影响引用绑定和 overload resolution：

```cpp
void inspect(int&);
void inspect(int const&);

const int n = 1;
inspect(n); // 只能选 const&
```

02 章会继续讲：表达式的值类别和 cv 类型一起决定能否绑定到 `T&`、`T const&` 或 `T&&`。

## 初始化器中的求值顺序

初始化不是只看最终值，还要看 initializer 表达式什么时候求值。braced-init-list 的 initializer-clause 按从左到右求值：

```cpp
Pair p{mark(1), mark(2)}; // mark(1) sequenced before mark(2)
```

这和函数实参不同。C++23 中函数实参之间不互相交错，但谁先求值没有固定顺序：

```cpp
f(mark(1), mark(2)); // 不要依赖 1 一定先于 2
```

构造函数 mem-initializer-list 的写法也容易误导。成员实际初始化顺序按成员声明顺序，不按冒号后文本顺序：

```cpp
struct S {
    int first;
    int second;
    S() : second(mark(2)), first(mark(1)) {}
};
```

这里 `first` 仍先初始化。04 章会把 base/member 构造与析构顺序展开。

## L01 练习怎样验证

L01 是观察和编译反例，不要求学生实现类型。

- `l01_observation` 只观察安全规则：value-initialization 产生零值、aggregate/designated initialization 赋值、`constinit` 静态初始化、braced-init-list 左到右求值。
- `l01_negative_narrowing` 编译 `int value{3.14};`，要求构建失败并匹配 narrowing 诊断。
- `l01_negative_constinit_dynamic` 编译 `constinit int value = runtime_value();`，要求构建失败并匹配 `constinit` 相关诊断。

这些检查不会读取未初始化局部变量。宏、编译器版本和一次运行输出只能证明当前工具链行为；课程正文只把 C++23 标准保证当成结论。

## 自测

1. `int x;` 和 `int x{};` 对自动存储期标量有什么区别？
2. `constinit` 是否让变量变成只读？
3. `Packet{.weight = 1.0, .id = 2}` 在 C++23 是否合法？
4. `Derived{.a=1, .b=2}` 在 C++23 是否能 designated-initialize 基类成员？
5. 花括号列表和函数实参的求值顺序能否按同一规则理解？

## 解析

1. `int x;` 创建对象但不写确定值，不能安全读取；`int x{};` value-initialize，结果为 0。
2. 不是。`constinit` 只要求静态或 thread 存储期变量必须静态初始化，不添加 `const`。
3. 不合法。C++23 designated initializer 必须按成员声明顺序。
4. 不能。C++23 designator 只能命名直接非静态数据成员；基类/间接成员属于后续提案方向。
5. 不能。braced-init-list 的 initializer-clause 从左到右；函数实参之间相对顺序不固定。
