# 08 unique_ptr：独占所有权、删除器与异常边界

L07 先把 RAII 的根钉牢：资源获取后，清理动作必须绑定到已经构造完成的对象。L08 把这个规则推进到标准库设施。`std::unique_ptr` 不是“更安全的裸指针”这么简单；它表达的是一个独占责任：此对象在自己的生命周期结束时负责把当前持有的指针交给删除器。

本章只讨论单个独占资源。共享所有权、弱引用和控制块留到 L09/L10。

## 从裸指针失败路径开始

先看一个看似正常的函数：

```cpp
auto* p = new Widget(1);
do_something_that_may_throw();
delete p;
```

成功路径没有问题。但如果中间语句抛异常，控制流不会继续执行 `delete p`。异常展开只会销毁已经构造完成的 C++ 对象，裸指针变量本身没有析构责任。裸指针的值会消失，堆上的 `Widget` 还活着。

`std::unique_ptr<Widget>` 修复的是这条因果链：

```cpp
auto p = std::make_unique<Widget>(1);
do_something_that_may_throw();
```

`p` 是局部对象。异常展开会调用 `p` 的析构函数。析构函数检查是否持有指针，若持有，就调用删除器。清理动作不再依赖后续普通语句是否还能执行。

这里没有魔法。所有权只是一个对象里的状态：一个指针，加上删除策略。语言保证对象析构，库把释放动作写进析构。

## 独占责任意味着不可复制

如果两个 `unique_ptr` 同时认为自己拥有同一地址，两个析构函数都会删除同一对象。这通常走向 double delete。标准库选择从类型层面阻止这类状态：`unique_ptr` 不可复制，只能移动。

```cpp
auto a = std::make_unique<Widget>(1);
// auto b = a;          // 编译失败
std::unique_ptr<Widget> b = std::move(a);
```

移动后，责任从 `a` 转到 `b`。`a` 仍是有效对象，但通常为空。可以析构、赋新值、`reset`，但不能当作还有对象来解引用。

这也是 `unique_ptr` 与“借用裸指针”的界线：

```cpp
Widget* raw = b.get();   // 观察，不取得所有权
```

`raw` 只是一张地址纸条。它不延长对象生命，也不负责释放。只要 `b` reset 或析构，`raw` 就悬空。`get()` 应用于调用不接管所有权的旧接口；它不是逃逸所有权的常规工具。

## get、release、reset 的区别

`get()` 返回当前地址，但 owner 不变。

`release()` 返回当前地址，并把 `unique_ptr` 置空。调用者从这一刻起必须接管释放责任：

```cpp
auto p = std::make_unique<Widget>(1);
Widget* raw = p.release();
// p 为空；raw 必须被 delete 或交给另一个 owner
std::unique_ptr<Widget> q(raw);
```

`reset(new_ptr)` 先释放旧资源，再持有新资源。`reset()` 无参数就是释放旧资源并置空。

最常见错误是把三者混用：用 `get()` 把地址交给会删除它的接口，会导致 `unique_ptr` 之后再次删除；用 `release()` 之后忘记立刻交给新 owner，会泄漏；在已有 owner 的情况下把同一裸指针构造成另一个 `unique_ptr`，会形成两个 owner。

## 删除器是所有权语义的一部分

默认 `unique_ptr<T>` 的删除器是 `std::default_delete<T>`，对单对象执行 `delete p`。但很多资源不是用 `delete` 释放：文件句柄要 `fclose`，C API 句柄要对应 `destroy`，数组要 `delete[]`，自定义池要归还池。

`unique_ptr` 把删除策略放进类型：

```cpp
using file_ptr = std::unique_ptr<FILE, int(*)(FILE*)>;
```

删除器类型会影响 `unique_ptr` 的大小、移动成本和 ABI。无状态删除器通常可被空基类优化掉，`unique_ptr<T>` 往往只有一个指针大小。函数指针删除器需要额外存一枚函数指针；带状态删除器还要保存状态。教学上可以先把它理解为“指针 + 删除器”，再用实际 `sizeof` 验证。

析构函数、`reset` 和 move assignment 中调用删除器时，`unique_ptr` 的析构不能抛异常离开。删除器应视为 `noexcept` 责任；让删除器抛异常会把程序带向终止或未定义边界，本课程不把这类真实 UB 放进默认路径。

## 数组特化不是容器

`std::unique_ptr<T[]>` 使用 `delete[]`，提供 `operator[]`，但没有长度。它能表达“独占拥有一段动态数组”，不能表达“安全序列”。多数业务代码应优先使用 `std::vector<T>`。只有和 C API、固定 ABI 或教学验证资源释放时，`unique_ptr<T[]>` 才是合适模型。

```cpp
auto xs = std::make_unique<int[]>(4);
xs[0] = 1;
```

这里的长度 4 不在 `xs` 中。任何边界检查都要由外部逻辑负责。

## 不完整类型与 pImpl

`unique_ptr<T>` 可用于成员中持有不完整类型，这使 pImpl 成为常见写法：

```cpp
class Owner {
public:
    Owner();
    ~Owner();
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
```

关键点是：析构 `unique_ptr<Impl>` 的地方必须看得到完整 `Impl`，因为默认删除器要生成 `delete Impl*`。所以 `Owner` 的析构函数通常放在 `.cpp` 中，在那里定义 `Impl`。如果把析构函数默认在头文件里，而头文件只前置声明 `Impl`，常会遇到静态断言或编译错误。

## 异常路径中的 make_unique

`std::make_unique<T>(args...)` 把分配和对象构造封装成一个表达式。如果对象构造抛异常，分配出来的存储会按语言和库规则清理，不会生成半初始化的 `unique_ptr`。这比手写 `std::unique_ptr<T>(new T(args...))` 更少暴露裸指针窗口。

对本章练习，失败路径是 `Tracked` 构造函数按指定值抛异常。正确实现必须传播异常，同时保证没有 `Tracked` 存活、没有删除器事件伪造、没有 Student 自己提供计数事实。

## 本机源码导读边界

本批次源码导读固定到本机安装的 MSVC STL：

- 本机头文件：`D:\VisualStudio2026\Installed\VC\Tools\MSVC\14.51.36231\include\memory`
- SHA256：`8955101828FE9D46C77E214E50415BAC5B1E59697E0F59BC46FA145AA1E8AE4C`
- 上游对照：Microsoft STL `msvc-build-tools-14.51`，GitHub release 指向提交 `edd1486`

阅读 `<memory>` 时，不要把实现细节误当标准要求。标准要求的是所有权语义：不可复制、可移动、析构/reset 时释放、release 后不再拥有、数组特化使用数组删除、删除器参与类型。MSVC 源码中能看到 `_Compressed_pair` 一类布局优化，它说明实现如何压缩空删除器；但其他 STL 可用不同内部名字和布局达成同一契约。

## 练习 L08 的 Part 解析

Part 1 观察 baseline：用 `make_tracked` 创建对象，确认构造计数和 alive 计数增加；离开作用域后，析构计数和删除器计数匹配。它证明成功路径清理存在。

Part 2 复现 release 责任转移：调用 `release_tracked` 后，原 owner 必须为空，返回的裸指针必须立即交给 `CountingDelete` 或新 owner。bad 变体会返回裸指针却保留 owner，checker 会在 delete 前拒绝它，避免默认路径制造 double delete。

Part 3 复现构造失败：fixture 让指定 value 的 `Tracked` 构造抛异常。正确实现不能产生 owner，也不能遗留 alive 计数。

Part 4 覆盖数组：`make_array` 返回 `unique_ptr<int[]>`，`sum_array` 只按传入长度读取。它展示数组 owner 不保存长度。

Part 5 覆盖不完整类型：`incomplete_owner` 表达 pImpl 风格的独占成员。移动后源为空，目标继续拥有状态，析构时计数归零。

本章通过 `checks/support/unique_support.hpp` 掌握真实构造、析构和删除器计数。Student 只能实现 owner 操作，不能提供完成标记或伪造报告。
