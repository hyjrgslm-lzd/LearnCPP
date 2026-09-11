# C10：Execution 与运行时设计

本课面向有现代C++基础、尚未系统掌握sender/receiver的工程师。从对象、执行与完成协议出发，逐步理解组合、调度、环境、取消和收束，再亲手实现adaptor与运行时，接入真实I/O和异构执行。采用记录处理流水线贯穿，局部机制用小实验展开；正文主讲知识，练习验证知识。

先读[课程地图](chapters/00-course-map.md)和[构建指南](exercises/BUILD_GUIDE.md)。目录、题号或旧测试通过都不单独代表完成；实际验证记录留在本机 build/validation 目录，不作为课程源码提交。

## 阅读路线

| 阶段 | 正文 | 实践入口 |
|---|---|---|
| 建立执行模型 | [01 五对象与生命周期](chapters/01-execution-model.md)、[02 惰性与组合](chapters/02-composition.md) | A1–A3；D11/D12回看底层协议 |
| 工作在哪里执行 | [03 调度器与上下文](chapters/03-schedulers.md) | B4–B6，starts_on/continues_on/on及真实执行位置 |
| 完成与上下文 | [04 error/stopped](chapters/04-channels.md)、[05 environment/scope](chapters/05-environments-and-scopes.md) | C1/C2、D11–D13 |
| 从使用进入实现 | [06 定制点](chapters/06-customization.md)、[07 完成签名](chapters/07-completion-signatures.md)、[08 adaptor](chapters/08-adaptor.md) | E1–F3、G1–G3；G1为已审复杂样章 |
| 构建运行时 | [09 run_loop](chapters/09-runtime.md)、[10 task/scope](chapters/10-task-and-scope.md) | H1–H3、D14、R1；语言协程先修来自C09 |
| 真实平台桥接 | [11 Native I/O](chapters/11-native-io.md)、[12 异构执行](chapters/12-heterogeneous.md) | I1、B02、V1；分别接C07/C14 |
| 综合与源码 | [13 记录流水线](chapters/13-pipeline.md)、[14 两级源码阅读](chapters/14-source-reading.md)、[15 mini execution](chapters/15-mini-execution.md) | P1、S1/S2、P2 |
| 成本与前沿 | [16 可归因测量](chapters/16-measurement.md)、[规范和实现索引](references/standards-and-implementations.md) | B01、F01原生标准设施探针与主体 |

原26题保留稳定题号，允许改写题面与实现；原四个结课项目分别由P1、S1、S2、P2承接。逐知识去向和下游反查见[覆盖表](references/coverage.md)。

## 本课的技术边界

核心优先最新可用标准：MSVC采用/std:c++latest，现有GCC13主路径C++23，Clang18按设施能力使用C++26。接受某个模式不代表完整支持该标准库；nvc++同样分别检查语言、宿主库和GPU能力。

规范固定N5050/N5054；代码使用固定stdexec `nvhpc-26.05`，SHA `6d7ad689f4d4831c5136e4abe1c601f9a3b64e43`。stdexec::是参考实现，exec::是扩展，nvexec::是NVIDIA异构路径；F01才检查工具链原生std::execution。具体差异、来源和日期见标准索引。

Windows使用真实IOCP，WSL/Linux使用已有liburing的io_uring路径。配置默认不联网、不安装依赖。Linux完整I/O验证需显式启用C10_STUDY_ENABLE_IO_URING并提供固定前缀；关闭时标NOT_ENABLED，不能当作能力通过。当前无nvc++时V1单/多GPU各自SKIP，仍保留完整正文、源码、构建与实验规格。

## 怎样做题

实现型题目在src/student/solution.hpp完成自己的操作，main.cpp/检查器不随学生实现改条件。Reference和独立good是不同依据，bad证明检查能拒绝指定错误；不要靠改变完成标志、预填结果或调用Reference获得通过。

运行型Student初态以UNFINISHED/exit2拒绝；纯类型题可按题面登记的精确类型检查以exit1拒绝。崩溃、超时和清理失败不是预期拒绝。观察型R1/S1/S2/B01/B02及能力单元则保留完整可运行程序，程序通过不代替读者的预测、对象图、源码分析和扩展任务。

每个单元都要能回答：谁拥有值和op-state，何时connect/start，实际在哪里执行，三条通道怎样传播，停止与关闭之后谁负责收束。对性能结论还要给出原始数据与归因，不能只给一个总耗时或“看起来更快”。

## 先修与跨课责任

[C02](../C02_Objects_Lifetime_Ownership/README.md)讲对象、借用与移动；[C03](../C03_Type_Modeling_Interface_Design/README.md)讲错误载荷和接口；[C04](../C04_Generic_CompileTime_Reflection/README.md)讲泛型/CPO/类型计算。它们不替代本课的具体完成协议。

[C07](../C07_OS_Memory_System_IO/README.md)提供系统完成源，[C08](../C08_Concurrency/README.md)提供同步和发布基础，[C09](../C09_Coroutines/README.md)提供promise/await语言协议，[C14](../C14_GPU/README.md)提供设备/stream/内存模型。基础sender组合不要求先学完整协程库；跨课桥接只在两侧必要基础之后进入。
