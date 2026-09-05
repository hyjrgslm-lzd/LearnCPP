# C++ 协程练习包

## 这套文档要解决什么问题

通过编码练习，依次学习协程的使用、语言协议与工程应用。

协程是 C++20 引入的无栈、可挂起/恢复函数机制。语言本身不提供 task、scheduler、事件循环或异步 I/O；C++23/C++26 的库设施与第三方框架在这套语言协议上补齐这些能力。

标准边界先看这份索引：[`references/标准条款与版本状态.md`](references/标准条款与版本状态.md)。课程正文会讲实践模型，但所有"标准已规定 / 工作草案 / 历史 paper / 实现细节 / 教学简化"的边界以该索引为准。

## 课程地图

| 阶段 | 知道是什么 | 知道为什么 | 知道怎么实现 | 知道怎么用 |
| --- | --- | --- | --- | --- |
| A-C + Capstone1 | 三关键字、generator、task、awaiter、stop_token、when_all/when_any、async_scope | 为什么协程适合表达异步控制流，为什么取消必须协作，为什么并发需要收束边界 | 写最小 awaiter、最小 lazy task、最小取消/组合/scope 练习 | 在应用层把回调、延迟、并发抓取和解析流水线组织成可读协程代码 |
| D-G | promise 生命周期、`co_await` 变换、协程状态、分配、HALO、shared_task、sync_wait | 为什么 hook 顺序决定语义，为什么 final suspend 要保留消费窗口，为什么返回 handle 能避免嵌套 resume 的栈增长风险 | 从零实现 task、shared_task、when_all、sync_wait、promise allocator，并用编译器输出验证帧/状态 | 在库层设计协程 return type、组合子、同步入口和帧分配策略 |
| H-J + Capstone4/5 | sender/receiver 桥接、真实 I/O、跨编译器 ABI、陷阱诊断、RPC 与 mini 协程库 | 为什么现代 C++ 项目把协程用于 I/O、RPC、任务图、结构化并发和异步资源管理 | 实现 sender-to-awaitable、Asio/folly/cobalt 风格模式、RPC 超时/取消/重试、mini corolib | 在生产边界决定哪里用协程、哪里只暴露普通 API、如何测试/诊断/隔离 ABI 风险 |

规模：当前是 **32 道普通练习题 + 3 个结课项目**。Capstone1/4/5 是稳定项目 ID，不代表项目数量；I-5 已完成并计入第三阶段 11 道普通练习题。

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

- 语言基线：C++20；涉及 `std::generator` 的题目需要 C++23 标准库支持；涉及 `std::execution` / `task` / `async_scope` 的题目按 C++26 工作草案学习设计，通常需要 stdexec 等参考实现。
- 编译器与标准库：以构建期 feature probe 为准；编译器版本号不能单独证明 `<generator>`、`<print>` 或 execution 支持。
- 第一阶段：禁止第三方依赖，仅使用 C++23 标准库 `<generator>` 与练习包共享的 minimal `lazy_task<T>` 教学实现。
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

- 你能从零写出本课程 task 所需的 promise hooks，并解释标准 replacement body 中每个调用的时机和目的。
- 你能实现 eager 版和 lazy 版的 task，并解释 `initial_suspend` 的 `suspend_always` vs `suspend_never` 对协程启动语义的影响。
- 你能实现 `await_suspend` 返回 `coroutine_handle` 的控制转交，解释它如何避免库代码显式递归 `.resume()`，并区分标准语义与编译器的栈/尾调用实现。
- 你能写出通过 `promise_type::operator new` 接管协程帧分配的自定义 allocator（P0912 风格）。
- 你能结合编译器诊断、IR/汇编和 allocation 计数观察动态分配是否被消除，并说明任何单一 flag 都不是跨编译器 HALO 保证。
- 你能从零实现 `shared_task`、`when_all`、`sync_wait`，并解释 shared_task 的引用计数与 final_suspend 多 resumer 链表之间的关系。

### 第三阶段验收

- 你能基于本课程固定的 stdexec 版本实现 sender-to-awaitable 桥接，并说明它与 current working draft `execution::as_awaitable` 的差异。
- 你能使用 Asio `awaitable<T>` 写出单线程多协程的回声服务器，并用 `cancellation_slot` 优雅关停。
- 你能系统地重现并分类协程陷阱：lambda/引用悬挂、异步 API 保存短命指针、持锁挂起、重复 resume/destroy、detached 生命周期等，并解释哪些是 UB、哪些是死锁或工程风险。
- 你能跨 MSVC/Clang/GCC 编译同一份协程代码，记录实现差异，并为跨模块边界设计同工具链 ABI 契约或 opaque C API，而不把“跨 DLL 必崩”当作标准结论。
- 你能从零实现一个 mini 协程库，包含 `task<T>`、`generator<T>`、`shared_task<T>`、`when_all`、`when_any`、`sync_wait`、scope、`stop_token`、`single_thread_executor` 和可选 sender-awaitable 桥接；HALO 单独作为实现观察项。

## 术语速查表

| 术语 | 你在练习里会看到什么 | 你应该问自己的问题 |
| --- | --- | --- |
| 协程 (coroutine) | 一个包含 `co_await/co_yield/co_return` 的函数 | 这个函数的执行是"调用即完成"还是"挂起-恢复"？ |
| 协程帧 / 协程状态 (coroutine state) | 标准意义上的 coroutine state；实现通常以帧对象保存 promise、参数副本和跨挂起点仍需存活的对象 | 哪些变量会进入帧？帧的大小谁决定？帧什么时候销毁？ |
| promise_type | 协程内部用于定制行为的对象，由编译器自动创建 | 必需与可选定制点分别在什么时间调用，哪些只适用于 generator 或分配失败？ |
| coroutine_handle | 指向协程帧的轻量句柄，可用 `.resume()` 恢复执行 | 谁拥有这个 handle？谁负责调用 `.destroy()`？ |
| awaitable | 可被 `co_await` 消费的对象（实现了 `operator co_await` 或被 `await_transform` 识别） | 这个对象的生命周期在 co_await 期间如何保障？ |
| awaiter | awaitable 经 `co_await` 变换后得到的实际控制对象 | 它的 `await_ready/await_suspend/await_resume` 分别在什么时机被调用？ |
| co_await | 挂起当前协程，将执行权转交给 awaiter 的关键字 | co_await 表达式返回什么值？挂起后谁继续执行？ |
| co_yield | 挂起并产出一个值给调用方/消费者 | `co_yield expr` 等价于哪两步的合写？ |
| co_return | 正常完成协程，向 promise 传递最终值（或 void） | final_suspend 之后，协程帧是否还存在？ |
| symmetric transfer | `await_suspend` 返回 `coroutine_handle`，标准规定会恢复该 handle 指向的协程 | 为什么它能避免库代码直接递归 `.resume()` 带来的栈增长风险？哪些栈/尾调用细节不能假设？ |
| HALO (Heap Allocation eLision Optimization) | 编译器在可证明协程状态生命周期被调用方严格嵌套时，可能省略动态分配 | HALO 的前提条件是什么？为什么不依赖 HALO 做正确性假设？ |
| unhandled_exception | promise_type 的异常入口，协程体内未捕获的异常会走到这里 | 为什么教学 task 通常在这里存 `exception_ptr`，而不是直接传播？标准允许什么异常路径？ |
| sender 桥接 | 按 `[exec.as.awaitable]` 把 sender 适配为 awaitable，或按 P3552 把协程 task 设计为 sender | bridge receiver 的三个 completion channel 分别对应协程的哪些路径？ |

## 一个非常重要的现实提醒

协程在 C++20 引入时，标准规定了语言变换和库接口，但没有规定协程帧的具体布局、HALO 的触发条件、或者不同编译器/ABI 之间裸 `coroutine_handle` 的兼容性。这意味着：

- **协程 ABI 不稳定**：同一份协程代码，MSVC 和 Clang 生成的帧布局可能不同，HALO 触发条件可能不同，连调试信息的格式都不同。如果你把协程写成库的边界 API，跨编译器/跨运行时调用时不要传裸的 `coroutine_handle`，而应该在库内部封装好消费逻辑和销毁所有权。
- **HALO 是优化，不是语义**：永远不要在代码逻辑里假设"协程帧一定在堆上"或"协程帧一定在栈上"。HALO 的触发条件会随编译器版本变化。正确的做法是：逻辑上只依赖返回对象和 handle 的生命周期契约，性能上期待 HALO 但不依赖它。
- **编译器和标准库支持仍在演进**：同一个 Clang/GCC/MSVC 前端搭配不同标准库时，`<generator>`、execution 和诊断能力可能不同。先记录完整工具链，再区分课程代码、库实现与编译器问题。

## 参考资料入口

做题过程中，建议反复对照下面这些资料的"概念定位"，而不是一上来通读全文：

### 核心博客系列

- Lewis Baker 协程系列（9 篇）：从 `co_await` 基础到 symmetric transfer 到 cancellation，是理解协程实现的最佳入口
- Raymond Chen 协程系列：以 Windows 开发者视角逐篇拆解协程的每个细节，包含大量编译器行为观察

### 标准提案

- P2502R2：`std::generator: Synchronous Coroutine Generator for Ranges` —— C++23 标准 generator
- P2300R10：`std::execution` —— sender-receiver 异步模型
- P3552R3：`Add a Coroutine Task Type`；当前规范性学习入口是 working draft `[exec.task]`
- working draft [`[exec.as.awaitable]`](https://eel.is/c++draft/exec.as.awaitable)：sender 到 coroutine awaitable 的适配规则
- P3296R1：`let_async_scope` —— 创建并保证 join 的 scope adaptor；与 P3149 配套阅读
- P3149R4：`async_scope — Creating scopes for non-sequential concurrency`、`spawn`、`spawn_future` 与 counting scope
- P0912R5：`Merge Coroutines TS into C++20 working draft`；其中包含 coroutine state 分配规则
- C++20 [`[coroutine.trivial.awaitables]`](https://eel.is/c++draft/coroutine.trivial.awaitables)：`suspend_always` / `suspend_never`

### 演讲与教程

- Gor Nishanov "C++ Coroutines: Under the covers" (CppCon 2016)
- Lewis Baker "Structured Concurrency" (CppCon 2019)
- Andreas Weis "Debugging C++ Coroutines" (CppCon 2024)

### 参考实现

- cppcoro：`task.hpp`、`generator.hpp`、`when_all.hpp`、`async_scope.hpp`
- folly：`folly/coro/Task.h`、`folly/coro/safe/SafeTask.h`、`folly/coro/AsyncScope.h`
- stdexec：`exec/task.hpp`、`__connect_awaitable.hpp`
- Boost.Cobalt：`channel`、`race`、`gather`
- Asio：coroutines 文档与 `awaitable<T>` 示例

### 手册

- cppreference.com：`<coroutine>`、`<generator>`、`coroutine_handle`、`std::stop_token`
