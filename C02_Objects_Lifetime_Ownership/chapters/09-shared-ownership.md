# 09 shared_ptr：控制块、别名、weak_ptr 与循环

`std::shared_ptr` 解决的是另一类问题：对象的最后一个强所有者何时消失无法由单个 owner 决定。多个组件都需要让对象保持存活时，可以共享同一个控制块。控制块记录强引用数、弱引用数、删除器、分配器等实现需要的信息。

这不是“可以随便传的智能指针”。共享所有权会增加状态、原子计数成本和设计复杂度。能用 `unique_ptr` 表达单一责任时，应优先使用独占所有权。

## stored pointer 与控制块指针

`shared_ptr<T>` 至少有两个概念性指针：

- stored pointer：`get()` 返回的地址，也是 `operator->` 使用的地址。
- control block pointer：引用计数和销毁策略所在位置。

通常二者指向同一个对象相关的数据。但 aliasing constructor 会故意让 stored pointer 指向子对象，而控制块仍属于外层对象：

```cpp
struct Pair { int a; int b; };
auto owner = std::make_shared<Pair>();
std::shared_ptr<int> alias(owner, &owner->b);
```

`alias.get()` 是 `int*`，但它和 `owner` 共享控制块。只要 `alias` 还活着，整个 `Pair` 不会析构。这个机制常用于返回子对象视图，同时保持外层对象生命。

错误心智模型是“shared_ptr 只保存一个 T* 并给它计数”。真实模型是“shared_ptr 的控制块管理一个被拥有对象；stored pointer 可以是另一个相关地址”。

## 强引用、弱引用与两阶段销毁

控制块通常维护两个计数：strong 和 weak。strong 大于 0 时，对象存活。最后一个 strong 释放时，对象析构。但控制块不能马上消失，因为仍有 `weak_ptr` 需要回答 `expired()` 和 `lock()`。

流程是两阶段：

1. strong 从 1 变成 0：销毁被管理对象。
2. weak 用户引用也归零：释放控制块。

`weak_ptr` 不拥有对象。它观察控制块，并在 `lock()` 时尝试取得一个新的 strong。如果对象已经析构，`lock()` 返回空 `shared_ptr`。它不能复活对象。

## 循环引用为什么泄漏

两个 `shared_ptr` 互相持有时，外部 owner 都销毁后，环内 strong 仍互相支撑：

```cpp
struct Node { std::shared_ptr<Node> next; };
auto a = std::make_shared<Node>();
auto b = std::make_shared<Node>();
a->next = b;
b->next = a;
```

离开作用域后，`a` 和 `b` 这两个局部变量各自释放一个 strong，但节点成员里的 `next` 仍让对方 strong 大于 0。对象析构函数不会运行，于是成员也不会释放，环保持下去。

修复不是“手动 delete”。修复是把非拥有反向边改成 `weak_ptr`：父子关系中，父通常拥有子；子观察父。观察边不增加 strong，外部 strong 消失后对象能按拓扑释放。

## enable_shared_from_this 的前提

对象想在成员函数里拿到管理自己的 `shared_ptr`，不能写 `std::shared_ptr<T>(this)`。那会创建第二个控制块，两个控制块都认为自己拥有同一对象，后果是双重释放。

`std::enable_shared_from_this<T>` 的做法是在对象第一次被 `shared_ptr` 管理时，让基类内部 weak 状态绑定到那个控制块。之后成员函数调用 `shared_from_this()`，得到共享同一控制块的新 strong。

前提很硬：对象必须已经由 `shared_ptr` 管理。构造函数里调用 `shared_from_this()`，或对栈对象调用，都会失败。常见表现是抛 `std::bad_weak_ptr`。课程练习只走可复现的安全路径，不把双控制块 UB 放进默认测试。

## 线程安全的边界

`shared_ptr` 控制块计数更新是线程安全的：多个 `shared_ptr` 实例在不同线程复制、销毁，可以安全维护引用计数。但这不代表被管理对象的成员访问线程安全。

```cpp
auto p = std::make_shared<std::vector<int>>();
// 多线程复制 p 是计数安全；多线程同时 push_back *p 仍需要同步。
```

这条边界很重要。所有权机制只管理对象生命，不自动保护对象内部不变量。

## 成本与适用边界

`shared_ptr` 的成本来自控制块分配、计数更新、间接销毁和所有权不清晰。`make_shared` 通常把控制块和对象合并分配，局部性好，分配次数少；但对象内存会等到控制块释放才归还。如果有长寿命 weak 观察者，大对象的存储可能被延后释放。`shared_ptr<T>(new T)` 则通常分开分配对象和控制块。

设计上，`shared_ptr` 应用于共享生命期确实存在的对象，例如缓存条目、异步回调共享状态、图中多个强 owner。函数只临时使用对象时，应传 `T&`、`T*` 或 `std::weak_ptr`/`shared_ptr` 的明确语义，而不是无脑复制 strong。

## 本机源码导读边界

本章源码导读固定到本机 MSVC STL：

- 本机头文件：`D:\VisualStudio2026\Installed\VC\Tools\MSVC\14.51.36231\include\memory`
- SHA256：`8955101828FE9D46C77E214E50415BAC5B1E59697E0F59BC46FA145AA1E8AE4C`
- 上游对照：Microsoft STL `msvc-build-tools-14.51`，GitHub release 指向提交 `edd1486`

MSVC 源码里能看到 shared/weak 的基类层次、引用计数操作和 `_Ref_count` 系列控制块。阅读时抓三条线：shared 复制增加 strong；weak 复制增加 weak；最后 strong 释放对象，最后 weak 释放控制块。内部名字、压缩布局和调试钩子是实现细节，不应写进用户代码假设。

## 练习 L09 的 Part 解析

Part 1：构造 `Node` 并复制 `shared_ptr`，验证 `use_count` 增减和最后析构。fixture 只记录真实 `Node` 构造/析构。

Part 2：创建 aliasing `shared_ptr<int>` 指向 `Node::value`。checker 验证 alias 与 owner use_count 共享，并且 owner reset 后 alias 仍能读到子对象值。

Part 3：用 `weak_ptr` 观察对象。对象销毁后 `expired()` 为真，`lock()` 返回空。bad 变体会假装 lock 成功，checker 必须拒绝。

Part 4：checker 创建真实父子节点并交给 `connect_parent_child(parent, child)`。实现只负责连边：父强拥有子，子用 weak 观察父。checker 直接检查节点成员、外部 weak 和 scope 后 alive 计数；bad cycle 变体用额外 shared 反向边制造环，会被真实析构计数拒绝。

Part 5：checker 创建由 `shared_ptr` 管理的真实对象，再调用 `shared_from_existing(node&)`。实现必须从该对象调用 `shared_from_this()` 并返回同控制块 strong；checker 直接比较地址和 `use_count()`，不接受硬编码结果，也不执行会制造双控制块的真实 UB。

本章 checker 固定从 `checks/support/shared_support.hpp` 读取受信 fixture，再通过 `<owner.hpp>` 消费当前目标实现。Student 不能通过自带计数或 ready 标记绕过契约。
