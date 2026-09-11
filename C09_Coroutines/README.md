# C09：协程与异步程序设计

这套课程面向能够编写现代 C++、希望系统理解协程的工程师。你会先学习执行模型与标准库结果通道，通过小实验认识惰性执行和状态保存，再逐步实现协程类型，最后把这些机制放进 I/O、RPC 与任务组合中。

课程包含 **34 道独立练习和 3 个项目，共 37 个学习单元**。34 道练习由 2 道预备练习和既有的 32 道普通练习组成；项目沿用 Capstone1、Capstone4、Capstone5 这三个稳定 ID。

逐单元去向见[覆盖表](references/coverage.md)。实现型 Starter 现在会实际进入学生操作；未完成时明确失败，完成相关 Part 后再用同一检查验收。观察、学生作业、Reference 和独立 good/bad 分别统计。

## 从这里开始

1. 阅读 [00 预备知识：执行模型与标准库](00-预备知识-执行模型与标准库.md)，分别完成 future 与 generator 的小实验。
2. 阅读 [01 心智模型](01-心智模型.md)，把普通函数、保存的状态、协程返回对象和恢复动作联系起来。
3. 进入模块 A。每个 Part 先读对应讲解，预测示例行为，再运行或补全练习，最后用观察结果解释前面的知识。

正文代码块突出正在讲的机制；练习目录中的完整文件提供头文件、入口函数和可运行示例。`main.cpp` 是供你观察、修改和补全的 Starter，`solution.cpp` 是完整 Reference。H1–H3 的学生编辑入口为 `student.hpp`，`checks/` 保持固定；项目采用各自的 `src/` 与 `reference/` 目录。你可以先运行已有日志建立直觉，再完成 TODO，并用参考实现核对自己的推理。

通用构建、链接、ABI 与工具能力的背景见 [C01 工程课程](../C01_Build_Compile_Link/README.md)，按当前练习需要选择阅读，不要求先学完整门工程课。[覆盖登记](references/coverage.md)记录37单元的入口和验证范围；Student 的 TODO 状态与 Reference 通过状态分别看待。

本机编译与运行方式见 [构建指南](exercises/BUILD_GUIDE.md)。首次使用可在 `C09_Coroutines/exercises` 执行：

```powershell
cmake --preset verify-core
cmake --build --preset verify-core
```

## 阅读路线

| 阶段 | 正文 | 这一阶段逐步解决的问题 |
| --- | --- | --- |
| 预备 | [00 执行模型与标准库](00-预备知识-执行模型与标准库.md) | 工作何时执行，结果如何传递，序列如何按需产生 |
| 基本模型 | [01 心智模型](01-心智模型.md) | 函数暂停后保存什么，谁持有状态，谁让它继续执行 |
| 使用 | [A 三关键字](02-模块A-三关键字与最小协程.md)、[B generator 与 task](03-模块B-generator与task的使用.md)、[C 取消与组合](04-模块C-取消与组合.md) | 怎样表达产出、取值、顺序关系、并发与结束条件 |
| 第一个应用 | [Capstone1 异步小爬虫](05-第一阶段结课-异步小爬虫.md) | 怎样把获取、解析、并发与取消连接成一个有清晰生命周期的程序 |
| 语言与库实现 | [D promise](06-模块D-promise_type全解.md)、[E await 协议](07-模块E-awaitable三层与co_await变换.md)、[F 帧与分配](08-模块F-协程帧与allocator.md)、[G 高级 task](09-模块G-symmetric_transfer与高级task.md) | 怎样从调用时序与所有权推导出返回类型、等待器、组合器和同步入口 |
| 工程应用 | [H sender 桥接](10-模块H-协程与sender_receiver桥接.md)、[I 真实 I/O](11-模块I-真实异步IO与并发框架.md)、[J 诊断](12-模块J-陷阱诊断与跨编译器.md) | 执行框架如何管理完成、调度、取消和资源；怎样定位实际问题 |
| 综合实现 | [Capstone4 RPC](13-第三阶段结课-RPC框架.md)、[Capstone5 mini 协程库](14-第三阶段结课-mini协程库实现.md) | 怎样把协议、控制流、所有权与错误处理组合成完整实现 |
| 源码阅读 | [15 源码阅读路线](15-源码阅读路线.md) | 带着每阶段的问题，在真实实现中找到机制对应的位置 |

源码阅读可以贯穿学习过程。完成 G 后，可以先进入 Capstone5 的核心部分；学习 H 后再接它的 sender 桥接扩展。J 中的系统诊断也适合在遇到具体问题时按主题回看。

## 知识与练习对照

下面的链接进入各练习 README，其中给出对应知识位置、Part 操作和观察结果。依赖环境由构建指南统一说明。

| 练习 | 知识与实验 |
| --- | --- |
| [P1](exercises/P1_future_basics/README.md) | future 共享状态、等待与取值、async 策略、异常、shared_future |
| [P2](exercises/P2_generator_basics/README.md) | generator 启动、迭代推进、提前结束与保存结果 |
| [A1](exercises/A1_first_generator/README.md) | Fibonacci、消费前缀与逐行文本生成 |
| [A2](exercises/A2_co_return_lazy_task/README.md) | lazy task 的创建、启动和 `co_return` 值流 |
| [A3](exercises/A3_co_await_future/README.md) | future 适配、awaiter 三方法与跨线程恢复 |
| [B1](exercises/B1_recursive_generator/README.md) | 递归序列、嵌套生产与 `elements_of` |
| [B2](exercises/B2_task_sequential/README.md) | 父子 task 顺序链与异常传播 |
| [B3](exercises/B3_callback_to_awaiter/README.md) | 回调适配、完成时机与状态生命周期 |
| [C1](exercises/C1_stop_token_cancel/README.md) | 停止状态、token 与协作式检查 |
| [C2](exercises/C2_when_all_when_any/README.md) | 组合的结果、错误与取消收束 |
| [C3](exercises/C3_async_scope/README.md) | scope 的子任务所有权与结束边界 |
| [D1](exercises/D1_promise_8_hooks/README.md) | 从协程生命周期理解 promise 定制点 |
| [D2](exercises/D2_eager_vs_lazy/README.md) | initial suspend 与 eager/lazy 启动 |
| [D3](exercises/D3_final_suspend_symmetric/README.md) | final suspend、continuation 与控制转交 |
| [E1](exercises/E1_co_await_lookup/README.md) | awaitable 变换与 `operator co_await` 查找 |
| [E2](exercises/E2_await_suspend_three/README.md) | `await_suspend` 的三种返回形式 |
| [E3](exercises/E3_trivial_awaitable/README.md) | 就绪短路与 trivial awaitables |
| [F1](exercises/F1_frame_layout/README.md) | 协程状态、参数与局部对象的生命周期 |
| [F2](exercises/F2_promise_allocator/README.md) | promise 分配函数与释放配对 |
| [F3](exercises/F3_halo_diagnose/README.md) | 分配消除与具体编译器输出观察 |
| [G1](exercises/G1_shared_task/README.md) | 多个等待者与共享结果 |
| [G2](exercises/G2_when_all_impl/README.md) | 组合器的完成状态与分支收束 |
| [G3](exercises/G3_sync_wait_impl/README.md) | 单次启动、完成通知与同步消费 |
| [H1](exercises/H1_as_awaitable/README.md) | sender completion 与协程等待适配 |
| [H2](exercises/H2_std_execution_task/README.md) | 固定 stdexec 版本的 task 与执行模型 |
| [H3](exercises/H3_bidirectional_bridge/README.md) | 两种异步抽象的双向连接 |
| [I1](exercises/I1_asio_echo/README.md) | Asio awaitable、事件循环与 echo |
| [I2](exercises/I2_io_uring_iocp/README.md) | I/O 提交与完成通知的关系 |
| [I3](exercises/I3_folly_safe_task/README.md) | Folly task、异步作用域与安全参数传递 |
| [I4](exercises/I4_cobalt_channel/README.md) | Cobalt channel、生产消费与结束通知 |
| [I5](exercises/I5_cppcoro_patterns/README.md) | cppcoro 的 task、generator 与组合模式 |
| [J1](exercises/J1_eight_pitfalls/README.md) | 生命周期与恢复错误的诊断 |
| [J2](exercises/J2_cross_compiler_abi/README.md) | 编译产物、ABI 与模块边界 |
| [J3](exercises/J3_coroutine_tracing/README.md) | 协程标识、事件与执行链追踪 |
| [Capstone1](exercises/Capstone1_async_crawler/README.md) | 获取、解析、并发与取消的综合应用 |
| [Capstone4](exercises/Capstone4_rpc_framework/README.md) | RPC 请求生命周期、错误、超时与关停 |
| [Capstone5](exercises/Capstone5_mini_corolib/README.md) | mini 协程库核心与桥接扩展 |

## 怎样利用观察结果学习

读一个新机制时，先用一句话描述正在解决的问题。例如，“生产者每次只交付一个值”或“父任务要等两个子任务都结束”。接着给示例中的对象标出所有者，给控制流标出启动、挂起、完成位置。

运行实验时，记录足以解释机制的现象：一个计数、几行日志、一个结果或一条异常路径。多线程输出可能存在多种合法顺序；根据代码保证的先后关系解释日志，可以理解多次运行中出现的不同排列。

最后，把观察放回知识模型中解释。如果 task 创建后尚未执行，找出启动动作；如果等待者得到了结果，找出存储、通知和消费的位置。这样的记录在后面阅读更复杂的实现时可以直接复用。

## 技术背景与参考资料

课程的协程语言基础来自 C++20，标准 generator 来自 C++23。工程练习使用仓库固定的第三方版本；实际构建以本机编译器、标准库和已启用依赖为准。

[标准条款与版本状态](references/标准条款与版本状态.md) 汇集规范入口，并区分语言规则、库类型约定与编译器实现观察。各章在需要时引用相关条款和源码，便于你由示例继续追到正式定义。

学习期间可反复使用的资料包括 [WG21 协程定义](https://eel.is/c++draft/dcl.fct.def.coroutine)、[await 表达式](https://eel.is/c++draft/expr.await)、[generator](https://eel.is/c++draft/coro.generator)，以及 [Lewis Baker 的协程文章](https://lewissbaker.github.io/)。具体框架的文档与源码入口见 H、I 和源码阅读路线。

## C02 生命周期与所有权先修

阅读协程捕获、帧和 awaiter 之前，可通过 [C02 生命周期与借用](../C02_Objects_Lifetime_Ownership/chapters/03-lifetime-and-borrowing.md)及[RAII 与所有权](../C02_Objects_Lifetime_Ownership/chapters/07-raii-and-ownership.md)补齐对象存活与责任转交。闭包先析构而帧仍借用其成员，是两种生命期不一致的问题；挂起、恢复、取消与帧销毁协议继续由本课展开。

## C03 状态、错误与回调衔接

[C03 状态集合](../C03_Type_Modeling_Interface_Design/chapters/04-variant-and-state-space.md)、[错误通道](../C03_Type_Modeling_Interface_Design/chapters/05-expected-and-error-channels.md)和[可调用包装](../C03_Type_Modeling_Interface_Design/chapters/12-callable-objects-and-type-erasure.md)解释promise中的值/异常载荷与回调拥有、借用关系。optional空值不自动表示取消，exception_ptr也不定义恢复时机；这些含义仍由本课的协程协议明确。

## C04 泛型与编译期桥接

[进入C04课程](../C04_Generic_CompileTime_Reflection/README.md)。promise选择、operator co_await查找和awaitable包装需要C04的模板选择、ADL、重载和转发；本课继续主讲帧生命周期、恢复及异常协议，不能用一个通用CPO模型替换协程专属变换。

## C05 数据表达先修

RPC长度头、数字完整消费、UTF与基础schema的先修见[C05 数据表达](../C05_Data_Representation_Standard_Facilities/README.md)。C05的同步字节解码不替代这里的连接、协程、取消和缓冲存活责任。

## C07 系统 I/O 先修

[C07 readiness](../C07_OS_Memory_System_IO/chapters/11-readiness.md)、[IOCP](../C07_OS_Memory_System_IO/chapters/12-windows-iocp.md)、[io_uring](../C07_OS_Memory_System_IO/chapters/13-linux-io-uring.md)和[取消收束](../C07_OS_Memory_System_IO/chapters/14-cancellation-shutdown.md)解释提交、等待和最终完成，以及 buffer/请求身份必须存活到何时。再回本课 I2，将系统完成接到 awaiter 的结果写入与恢复；帧的销毁/取消协议仍由本课单独验证。

## C10 执行协议桥接

[C10 task 桥接](../C10_Execution/chapters/10-task-and-scope.md)在本课 promise/await/final_suspend 基础上实现 sender await、环境传播与 task-as-sender；[G1 adaptor](../C10_Execution/chapters/08-adaptor.md)则从非协程完成协议推导组合。先修按具体单元进入，不要求先读完两课。
