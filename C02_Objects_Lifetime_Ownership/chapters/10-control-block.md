# 10 控制块模型：实现单线程 rc_ptr 与 weak_rc

L09 从使用者角度解释 `shared_ptr`。L10 写一个小型单线程模型，把控制块的生命周期完整拆开。目标不是复刻标准库，而是让 strong、weak、对象析构、控制块释放的因果关系变成可调试的代码。

本章类型名为 `rc_ptr<T>` 与 `weak_rc<T>`。它们只支持单线程；不支持 aliasing constructor；不支持 `enable_shared_from_this`；不提供自定义删除器和分配器。边界写清楚，是为了避免把教学模型误认为标准库替代品。

## 最小控制块需要什么

一个共享所有权模型至少需要：

- strong 计数：当前有多少强 owner。
- weak 计数：当前有多少 weak observer，加上一个隐含 weak 保证对象存活时控制块也存活。
- 对象存储：`T` 的实际存储或指针。
- 销毁动作：strong 归零时析构对象；weak 归零时释放控制块。

为什么需要隐含 weak？因为只要对象还活着，控制块必须活着；最后一个 strong 释放时，需要先析构对象，再释放这份隐含 weak。如果还有用户 weak，控制块留下；如果没有，控制块也随之删除。

## strong=0 只析构对象，不一定释放控制块

错误实现常把“对象”和“控制块”当作同一件事：最后一个 strong 消失就把整个块 delete。这样若还有 weak 指向它，weak 就成了悬空指针，之后 `expired()` 或 `lock()` 都会读已释放内存。

正确流程：

```text
strong: 1 -> 0
析构 T 对象
weak: implicit + user_weak -> user_weak
if weak == 0: delete control block
```

这里 `T` 的析构必须只执行一次。之后 `weak_rc::lock()` 看到 strong 为 0，只能返回空 owner。它不能重新构造 T，也不能把 strong 从 0 加回 1。

## copy、move、reset 的规则

`rc_ptr` copy 增加 strong。move 转移控制块指针，不改变计数，并把源置空。`reset()` 释放当前 strong；如果这是最后一个 strong，就触发对象析构和隐含 weak 释放。

copy assignment 不能先覆盖自己再增加计数，否则旧资源会泄漏；也不能先释放目标再读取 RHS，因为合法表达式可能让 RHS 位于旧对象内部，例如 `p = p->child`。正确顺序是先保存并增加 RHS 控制块的 strong，再释放 LHS 旧资源，最后安装新控制块。move assignment 同理，`p = std::move(p->child)` 必须先把 RHS 控制块交换到局部变量，再释放旧 LHS。self assignment 要保持计数稳定。

`weak_rc` copy 增加 weak。move 转移观察状态，不改变计数。`weak_rc` 析构或 reset 释放一个 weak；若此时 strong 已为 0 且 weak 变成 0，释放控制块。

## 构造失败必须清理控制块

`make_rc<T>(args...)` 通常先分配控制块，再在控制块内部构造 T。如果 T 构造函数抛异常，控制块构造没有完成；已经完成的基类或成员会析构。教学实现把控制块计数登记放在控制块基类构造/析构里，使对象构造失败也能回滚控制块登记。

不能先把“对象 alive”或“控制块 alive”写进全局状态，再执行可能分配或抛异常的日志操作。本批次 fixture 的日志和计数使用固定状态，析构和 release 不分配。

## 与真实 std::shared_ptr 的差距

真实标准库还要处理：

- 原子引用计数和内存序。
- `make_shared` 的对象/控制块合并分配。
- 自定义删除器、分配器和异常安全。
- aliasing constructor 的 stored pointer/control block 分离。
- `enable_shared_from_this` 的 weak 状态接线。
- 数组、函数类型、转换构造、owner ordering 等边界。

本章模型故意删掉这些能力，只保留两阶段生命周期。这样 checker 能用真实对象析构计数和控制块计数定位错误：漏析构对象、提前释放控制块、weak 复活对象、copy/move 计数错误。

## 本机源码导读边界

本章源码导读固定到本机 MSVC STL：

- 本机头文件：`D:\VisualStudio2026\Installed\VC\Tools\MSVC\14.51.36231\include\memory`
- SHA256：`8955101828FE9D46C77E214E50415BAC5B1E59697E0F59BC46FA145AA1E8AE4C`
- 上游对照：Microsoft STL `msvc-build-tools-14.51`，GitHub release 指向提交 `edd1486`

对照阅读时，可以把本章 `control_block_base` 映射到 MSVC `_Ref_count_base` 一类结构，把 `rc_ptr` 的 copy/reset 映射到 shared strong 递增/递减，把 `weak_rc` 映射到 weak 递增/递减和 lock。不要把字段名、继承层级或调试宏当标准接口。标准要求行为，库实现可变。

## 练习 L10 的 Part 解析

Part 1：实现 `make_rc`。成功时 strong 为 1、用户 weak 为 0、对象和控制块各 alive 1。构造失败时异常传播，二者都归零。

Part 2：实现 `rc_ptr` copy/move/reset。copy 增加 strong；move 清空源但不改计数；reset 释放一个 strong；最后 strong 析构对象并在没有用户 weak 时释放控制块。checker 额外覆盖 `p = p->child` 和 `p = std::move(p->child)`：RHS 成员位于旧 pointee 内部，赋值必须先保存 RHS 控制块再释放旧对象。

Part 3：实现 `weak_rc` 从 `rc_ptr` 构造、copy、move、reset。weak 不延长对象生命，但延长控制块生命。

Part 4：实现 `lock()`。对象活着时，lock 增加 strong 并返回 `rc_ptr`；对象已经析构时，lock 返回空，不能复活。

Part 5：复验错误变体。`bad_leak_control_block` 漏释放控制块；`bad_weak_resurrect` 在 weak 存在时保留对象或允许过期 weak 变回 strong。checker 通过受信 fixture 的对象/控制块计数拒绝这些实现。

Student 只编辑 `src/student/rc.hpp`。公共检查先从相对路径加载 `checks/support/rc_support.hpp`，再包含 `<rc.hpp>`。实现头若需要 fixture，也应使用相对路径指向真实 support，不依赖可被 validation 目录遮蔽的搜索顺序。
