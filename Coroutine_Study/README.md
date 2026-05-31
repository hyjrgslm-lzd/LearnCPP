# C++ 协程练习包

## 这套文档要解决什么问题

这不是一套"背关键字"的笔记，而是一套"通过亲手编码理解协程设计"的练习包。

协程是 C++20 引入、C++23/C++26 持续完善的核心异步原语。截至本练习包整理时，市面上的 C++ 协程教程大多存在两个问题：

- 要么停在 `co_yield` 和 `co_return` 的语法讲解，缺少对协程帧、promise 钩子、awaiter 三层协议等底层机制的系统拆解。
- 要么直接跳到 cppcoro 或 folly 源码阅读，中间缺少"从应用到原理到工程"的递进路径。

这套文档的目标就是填补这个空白：让你从"会写 co_await"出发，经过编译器变换、promise_type 8 个 hook、awaiter 三方法、symmetric transfer、HALO 等原理层面拆解，最终能在生产工程中正确使用 asio coroutine、folly coro、stdexec task，并独立实现一个 mini 协程库与 RPC 框架。

## 三阶段定位

### 第一阶段：你会学到"怎么用"

你不需要理解协程帧在堆上的布局细节，也不需要知道 `await_suspend` 为什么能返回 `coroutine_handle`。你只需要做到：能用 `std::generator<T>` 写惰性序列，能用附带的最小 `lazy_task<T>` 写顺序异步组合，能用 `co_await` 把回调 API 包成协程可消费的 awaiter，能用 `stop_token` 写出协作式取消，能用 `when_all` / `when_any` / `async_scope` 管理并发生命周期。

这个阶段的核心目标是建立"协程是表达异步控制流的语法糖"这一直觉，同时让 `co_await`、`co_yield`、`co_return` 三个关键字的语义刻进肌肉记忆。

### 第二阶段：你会学到"怎么实现"

这个阶段你会正面面对编译器替你生成的那些代码：`promise_type` 的 8 个 hook 分别在什么时间点被调用、`await_suspend` 的三种返回类型（void / bool / coroutine_handle）分别对应什么调度语义、symmetric transfer 如何解决多层协程互相 resume 导致的栈溢出、协程帧在堆上的布局到底是怎样的、HALO 在什么条件下能被编译器触发。

你会从零实现一个 `lazy_task<T>`（不再是第一阶段的"附带 30 行"），然后逐步加上 `shared_task`、`when_all`、`sync_wait`、`async_scope` 等基础设施。到这个阶段结束，你就不再"用 cppcoro"，而是"知道 cppcoro 为什么这样写"。

### 第三阶段：你会学到"怎么在工程里用"

协程真正走进生产代码时，问题就不只是"怎么写对"了。你会面对：协程如何与 stdexec sender-receiver 模型互操作（P3175 桥接）、协程如何绑定到 Asio 的 io_context 上做真正的异步 I/O、协程帧的分配如何通过自定义 allocator（P0912）进行池化优化、跨编译器的 ABI 差异如何影响你的库边界设计、协程中的生命周期陷阱（lambda 引用跨 co_await 悬挂、临时量在 co_await 表达式中析构、lock_guard + co_await 的 mutex UB）如何系统地诊断和规避。

到这个阶段，你就不只是"会用协程写异步代码"，而是能在生产工程中设计协程 API、诊断协程性能问题、并独立实现一个带超时/取消/重试的 RPC 框架。

## 你会得到什么

这套练习包分为三个阶段：

### 第一阶段：会用协程

- 1 份心智模型总说明。
- 3 个模块（A/B/C），共 9 道练习题。
- 1 个结课项目（异步小爬虫）。
- 每题统一的复盘框架。

### 第二阶段：懂协程实现

- 4 个模块（D/E/F/G），共 12 道练习题。
- 全程手写实现，不再依赖附带头文件。
- 逐层扩展 task、shared_task、when_all、sync_wait。

### 第三阶段：工程价值

- 3 个模块（H/I/J），共 12 道练习题（含结课）。
- 2 个结课项目（RPC 框架 + mini 协程库实现）。
- 1 份源码阅读路线。

### 总计

- **33 道练习题 + 5 个结课项目**

## 阅读顺序

### 第一阶段

1. `01-心智模型.md`
2. `02-模块A-三关键字与最小协程.md`
3. `03-模块B-generator与task的使用.md`
4. `04-模块C-取消与组合.md`
5. `05-第一阶段结课-异步小爬虫.md`

### 第二阶段

6. `06-模块D-promise_type全解.md`
7. `07-模块E-awaitable三层与co_await变换.md`
8. `08-模块F-协程帧与allocator.md`
9. `09-模块G-symmetric_transfer与高级task.md`

### 第三阶段

10. `10-模块H-协程与sender_receiver桥接.md`
11. `11-模块I-真实异步IO与并发框架.md`
12. `12-模块J-陷阱诊断与跨编译器.md`
13. `13-第三阶段结课-RPC框架.md`
14. `14-第三阶段结课-mini协程库实现.md`
15. `15-源码阅读路线.md`

## 统一技术基线

本练习包默认你已经具备本地编码条件，因此这里不写安装和工程搭建，只固定练习边界。

- 语言基线：C++20，**强制要求 C++23**（需要 `<generator>`），**期望 C++26**（关注 P2300/P3552 进展）
- 编译器：MSVC 17.10+、Clang 17+、GCC 14+ 至少其一
- 第一阶段：禁止任何第三方依赖，仅使用 C++23 标准库 `<generator>` + 每篇题目随附的 30 行 minimal `lazy_task<T>` 头文件
- 第二阶段：全程手写实现，不使用第三方库，不使用附带头文件
- 第三阶段：允许使用 stdexec、Asio、folly coro、Boost.Cobalt 等第三方框架
- 排除范围：GPU 协程、Unity/UE 协程、JS/Python 协程类比超过半页的讨论
- 线程要求：除非题目指定（如 A-3 跨线程 resume），不要手写 `std::thread`

## 每题统一交付物

每完成一道题，至少留下四样东西：

1. 一份可运行代码。
2. 一张协程帧/状态机/调用链草图。
3. 一段 5 到 10 行的观察记录。
4. 一段复盘结论：这一题到底让你理解了什么设计点。

## 每题统一模板

所有练习题都按同一模板组织：

- 目标
- 前置理解
- 必做任务
- 进阶任务
- 验收点
- 观察点
- 常见坑
- 提示
- 复盘问题
- 对应官方参考

你做题时也尽量按这个模板留笔记。这样你后面回看时，会非常容易发现自己到底卡在"概念没懂"，还是"API 没用熟"，还是"实现模式没理解"。

## 推荐做题方法

每题都按下面的顺序推进：

1. 先用一句话写出你认为这题在训练什么。
2. 先画协程帧／调用链草图，再落代码。
3. 先做"必做任务"，不要一开始就追进阶。
4. 跑通后，不马上进入下一题，先回答复盘问题。
5. 每做完一个模块，回去重读一次 `01-心智模型.md`。

## 最终验收清单

### 第一阶段验收

- 你能用 `std::generator<T>` 写出惰性序列，并解释 `co_yield` 与普通 return 的本质区别。
- 你能用附带的最小 `lazy_task<T>` 写出顺序异步组合，并指出 initial_suspend 和 final_suspend 在这个最小实现中分别承担什么角色。
- 你能给任意回调式 API 写出一个最小 awaiter（三方法），让它可以被 `co_await` 消费。
- 你能解释 `stop_token` 的协作式取消模型，以及为什么取消不是"杀死协程"，而是"协程在检查点自愿停止"。
- 你能画出一张含多个并发 task 的协程调用图，标注每条 task 的 resume/lazy start/final_suspend 位置。

### 第二阶段验收

- 你能从零写出 `promise_type` 的 8 个 hook，并解释每个 hook 被调用的时机和目的。
- 你能实现 eager 版和 lazy 版的 task，并解释 `initial_suspend` 的 `suspend_always` vs `suspend_never` 对协程启动语义的影响。
- 你能实现 `await_suspend` 返回 `coroutine_handle` 的 symmetric transfer，并解释它为何能避免栈溢出。
- 你能写出通过 `promise_type::operator new` 接管协程帧分配的自定义 allocator（P0912 风格）。
- 你能用编译器 flag（Clang `-Rpass=coroutine-elide` 或 GCC `-fdump-tree-coro`）诊断 HALO 是否触发，并解释 HALO 的前提条件（"帧不逃逸"）。
- 你能从零实现 `shared_task`、`when_all`、`sync_wait`，并解释 shared_task 的引用计数与 final_suspend 多 resumer 链表之间的关系。

### 第三阶段验收

- 你能实现 sender-to-awaitable 桥接（P3175 风格），让任意 stdexec sender 可以被 `co_await` 消费。
- 你能使用 Asio `awaitable<T>` 写出单线程多协程的回声服务器，并用 `cancellation_slot` 优雅关停。
- 你能系统地重现协程的 8 大经典陷阱（lambda 引用悬挂、临时量提前析构、lock_guard + co_await UB 等），并解释每个陷阱的根因。
- 你能跨 MSVC/Clang/GCC 分别编译同一份协程代码，对比帧大小、HALO 触发差异、调试信息差异，并说明为什么协程 ABI 不在标准内导致"不要跨 DLL 传 coroutine_handle"。
- 你能从零实现一个 mini 协程库，包含 `task<T>`、`generator<T>`、`shared_task<T>`、`when_all`、`when_any`、`sync_wait`、`async_scope`、`stop_token`、`single_thread_executor`、sender-awaitable 桥接，且 `sync_wait` 入口可触发 HALO。

## 术语速查表

| 术语 | 你在练习里会看到什么 | 你应该问自己的问题 |
| --- | --- | --- |
| 协程 (coroutine) | 一个包含 `co_await/co_yield/co_return` 的函数 | 这个函数的执行是"调用即完成"还是"挂起-恢复"？ |
| 协程帧 (coroutine frame) | 编译器在堆上（或 HALO 优化后在栈上）分配的状态存储 | 哪些变量会进入帧？帧的大小谁决定？帧什么时候销毁？ |
| promise_type | 协程内部用于定制行为的对象，由编译器自动创建 | 它的 8 个 hook 分别在什么时间点被调用？ |
| coroutine_handle | 指向协程帧的轻量句柄，可用 `.resume()` 恢复执行 | 谁拥有这个 handle？谁负责调用 `.destroy()`？ |
| awaitable | 可被 `co_await` 消费的对象（实现了 `operator co_await` 或被 `await_transform` 识别） | 这个对象的生命周期在 co_await 期间如何保障？ |
| awaiter | awaitable 经 `co_await` 变换后得到的实际控制对象 | 它的 `await_ready/await_suspend/await_resume` 分别在什么时机被调用？ |
| co_await | 挂起当前协程，将执行权转交给 awaiter 的关键字 | co_await 表达式返回什么值？挂起后谁继续执行？ |
| co_yield | 挂起并产出一个值给调用方/消费者 | `co_yield expr` 等价于哪两步的合写？ |
| co_return | 正常完成协程，向 promise 传递最终值（或 void） | final_suspend 之后，协程帧是否还存在？ |
| symmetric transfer | `await_suspend` 返回 `coroutine_handle`，让编译器用尾调用跳转到下一个协程 | 为什么 symmetric transfer 能避免多层 resume 的栈溢出？ |
| HALO (Heap Allocation eLision Optimization) | 编译器在可证明帧不逃逸时，将协程帧从堆分配优化为栈分配 | HALO 的前提条件是什么？为什么不依赖 HALO 做正确性假设？ |
| unhandled_exception | promise_type 的异常入口，协程体内未捕获的异常会走到这里 | 为什么 unhandled_exception 中不能再 throw？ |
| sender 桥接 | 把 stdexec sender 包装为 awaitable，或把协程 task 包装为 sender（P3175/P3552） | bridge receiver 的三个 completion channel 分别对应协程的哪些路径？ |

## 一个非常重要的现实提醒

协程在 C++20 引入时，标准委员会特意将 ABI 留在实现定义域，没有强制规定协程帧的布局、HALO 的触发条件、或者 coroutine_handle 的跨库传递语义。这意味着：

- **协程 ABI 不稳定**：同一份协程代码，MSVC 和 Clang 生成的帧布局可能不同，HALO 触发条件可能不同，连调试信息的格式都不同。如果你把协程写成库的边界 API，跨编译器调用时不要传裸的 `coroutine_handle`，而应该在库内部封装好消费逻辑。
- **HALO 是优化，不是语义**：永远不要在代码逻辑里假设"协程帧一定在堆上"或"协程帧一定在栈上"。HALO 的触发条件会随编译器版本变化。正确的做法是：逻辑上就当帧在堆上，性能上期待 HALO 但不依赖它。
- **编译器支持仍在演进**：C++23 `<generator>` 的 MSVC 实现、Clang 17+ 的 symmetric transfer 优化、GCC 14+ 的 coroutine 调试符号，每个版本都在改进。如果你的代码在某个编译器版本上行为异常，先确认不是编译器 bug，再怀疑自己的理解。

## 学完后你应该达到什么水平

### 第一阶段完成后

- 解释为什么协程不等于线程，协程是一次可以暂停和恢复的函数。
- 解释 `co_await` 导致挂起时，当前协程的栈帧被"冻结"到哪里（协程帧），而执行流又去了哪里（交给 awaiter）。
- 解释为什么 `std::generator<T>` 的 `co_yield` 是惰性的——每次 `begin()` 迭代器推进时才执行到下一个 `co_yield`。
- 解释为什么 async 代码中 `co_await` 串联看起来像同步代码，但背后的执行可以是非阻塞的。
- 用附带的最小 `lazy_task<T>` 完成多步异步顺序组合，并画出一张清晰的 await-suspend-resume 链。
- 给一个回调式异步 API 写出 awaiter 适配，使之可被协程消费。

### 第二阶段完成后

- 从零实现 `promise_type` 的 8 个 hook，解释每个 hook 的调用时机。
- 实现 eager 和 lazy 两种 task 启动策略，并解释 `initial_suspend` 的选择如何影响整个协程的启动语义。
- 实现 symmetric transfer（`await_suspend` 返回 `coroutine_handle`），并解释它如何解决协程互相 resume 的栈深度问题。
- 通过编译器诊断输出观察协程帧布局和 HALO，解释 HALO 的前提条件。
- 实现自定义 allocator（P0912 风格）来接管协程帧的分配。
- 从零实现 `shared_task`、`when_all`、`sync_wait`，并解释其中的并发控制和生命周期管理。

### 第三阶段完成后

- 实现 sender-to-awaitable 桥接，让 stdexec sender 可被 `co_await` 消费。
- 使用 Asio coroutine 写出单线程多协程的回声服务器，管理 cancellation 和 async_scope。
- 系统地诊断和规避协程的 8 大陷阱（lambda 悬挂、临时量析构、锁 + co_await 死锁等）。
- 跨编译器编译同一份协程代码，分析 ABI 差异并制定跨库边界策略。
- 从零实现一个 mini 协程库（task/generator/shared_task/when_all/when_any/sync_wait/async_scope/桥接），不依赖任何第三方库。
- 实现一个带超时/取消/重试的 RPC 框架。

## 参考资料入口

做题过程中，建议反复对照下面这些资料的"概念定位"，而不是一上来通读全文：

### 核心博客系列

- Lewis Baker 协程系列（9 篇）：从 `co_await` 基础到 symmetric transfer 到 cancellation，是理解协程实现的最佳入口
- Raymond Chen 协程系列：以 Windows 开发者视角逐篇拆解协程的每个细节，包含大量编译器行为观察

### 标准提案

- P2502R2：`std::generator<T>` —— C++23 标准 generator
- P2300R10：`std::execution` —— sender-receiver 异步模型
- P3552R3：`std::execution::task<T>` —— C++26 协程与 sender 的统一 task
- P3175R0：sender 与 coroutine awaitable 的桥接设计
- P3296R1：`async_scope` —— 结构化并发的作用域对象
- P3149R4：`std::execution` 的 when_all/when_any 相关设计
- P0912R5：协程帧的自定义 allocator 支持
- P2786R0：Trivial Awaitables —— 简化 awaitable 协议

### 演讲与教程

- Gor Nishanov "C++ Coroutines: Under the covers" (CppCon 2016)
- Lewis Baker "Structured Concurrency" (CppCon 2019)
- Andreas Weis "Debugging C++ Coroutines" (CppCon 2024)

### 参考实现

- cppcoro：`task.hpp`、`generator.hpp`、`when_all.hpp`、`async_scope.hpp`
- folly：`folly/experimental/coro/Task.h`、`SafeTask.h`、`AsyncScope.h`
- stdexec：`exec/task.hpp`、`__connect_awaitable.hpp`
- Boost.Cobalt：`channel`、`race`、`gather`
- Asio：coroutines 文档与 `awaitable<T>` 示例

### 手册

- cppreference.com：`<coroutine>`、`<generator>`、`coroutine_handle`、`std::stop_token`

## 最后一句提醒

不要把这套练习当成"我要赶快学会 co_await"。

把它当成三层训练：第一层是语法使用训练——你在学习用三个关键字表达异步控制流。第二层是编译器/运行时理解训练——你在学习协程帧、promise hooks、awaiter 协议、symmetric transfer 等在编译器和运行时层面真正发生的事情。第三层是工程判断训练——你在学习"这里该不该用协程""这个协程的生命周期谁负责""这个协程的帧应该走堆还是走池"这些问题。

三层都练透，你对 C++ 协程的理解就不再停留在"能用 co_await"，而是到达"能设计协程 API、能诊断协程性能问题、能看懂协程库实现"。
