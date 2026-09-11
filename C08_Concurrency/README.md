# C08：并发、同步与内存模型

这套课程从普通函数、对象生命期和线程执行开始，逐步进入共享状态、同步协议、原子与内存模型、并发数据结构，再把这些机制用于任务调度和数据并行计算。

复杂主题采用问题驱动的演进方式：先得到一个容易理解、行为明确的基线，预测并观察它的问题，推导改进，再检查新的不变量和代价。正文可以按需要展开为系列文章；每个版本都有对应的源码、实验和答案解析。阅读时同时关注“它为什么正确”与“它在什么场景值得使用”。

## 从这里开始

先阅读 [00 执行与对象背景](chapters/00-execution-and-objects.md) 和 [01 并发心智模型](chapters/01-concurrency-model.md)。课程假定你能阅读基本函数、类和模板；移动、捕获、RAII、结果状态等后续会用到的概念会通过小实验补足。

在 `C08_Concurrency/exercises` 运行：

```powershell
cmake --preset verify-core
cmake --build --preset verify-core --parallel 4
ctest --preset verify-core
```

核心路径不下载第三方依赖。完整环境、单题构建、C++26 探测、基准运行和平台限制见[构建指南](exercises/BUILD_GUIDE.md)。

通用构建、编译链接、符号、ABI 和工具能力探测的连续讲解见 [C01 工程课程](../C01_Build_Compile_Link/README.md)。本课在全局版图中主讲 C08：共享状态、同步、发布和安全回收；现有 CPU 性能、SIMD 与 NUMA 实验同时是 C13 的可复用输入，不代表完整 C13 已交付。各专项先修和未验证边界见本课[覆盖表](references/coverage.md)和[标准与实现状态](references/standards-and-implementations.md)。

## 学习路线

| 顺序 | 阅读入口 | 逐步解决的问题 |
|---|---|---|
| 00 | [执行与对象背景](chapters/00-execution-and-objects.md) | 谁在执行，数据放在哪里，谁保证对象活到最后一次使用 |
| 01 | [并发心智模型](chapters/01-concurrency-model.md) | 同步/异步、并发/并行怎样区分，怎样描述共享状态和不变量 |
| 02 | [结果通道](chapters/02-result-channels.md) | 结果入口、就绪、等待、消费、错误和共享分别是什么 |
| 03 | [线程与执行方式](chapters/03-threads-and-execution.md) | 怎样创建、传参、等待线程，怎样让结果和异常回到调用方 |
| 04 | [共享状态与锁](chapters/04-shared-state-and-locks.md) | 哪些操作必须作为整体，锁保护的究竟是什么 |
| 05 | [等待与有界队列](chapters/05-waiting-and-channels.md) | 如何等待状态变化，怎样处理满、空与背压 |
| 06 | [取消与关闭](chapters/06-cancellation-and-shutdown.md) | 停止请求、拒绝新工作、排空和生命周期怎样协作 |
| 07 | [计数与阶段同步](chapters/07-coordination.md) | 如何表达一次汇合、重复阶段和资源使用上限 |
| 项目 1 | [有界线程池](exercises/Capstone1_thread_pool/README.md) | 把任务投递、结果、异常、背压和关停组成一个系统 |
| 08 | [原子操作](chapters/08-atomics.md) | 原子操作与复合操作有什么区别，CAS 和等待各解决什么 |
| 09 | [内存模型](chapters/09-memory-model.md) | 怎样从标准同步关系推导跨线程可见性和允许的交错 |
| 10 | [发布与生命周期](chapters/10-publication-and-lifetime.md) | 发布新版本后，旧读者如何继续安全使用旧对象 |
| 11 | [并发数据结构](chapters/11-concurrent-structures.md) | 栈和不同队列怎样交接所有权、维持顺序并保证进展 |
| 项目 2 | [并发队列与进展保证](exercises/Capstone2_lockfree_queue/README.md) | 容量、预订、发布、暂时失败和回绕怎样组合 |
| 12 | [性能方法学](chapters/12-measurement.md) | 怎样设计公平比较并判断测量是否支持结论 |
| 13 | [缓存与布局](chapters/13-cache-and-layout.md) | 真/伪共享、局部累积和分块为何影响访问成本 |
| 14 | [并行算法](chapters/14-parallel-algorithms.md) | 执行策略允许什么，依赖、异常与浮点误差怎样限制算法 |
| 15 | [SIMD](chapters/15-simd.md) | 从标量与自动向量化走到显式向量、掩码、尾部和归约 |
| 16 | [调度与任务组合](chapters/16-scheduling.md) | 怎样分配不均匀工作，怎样避免嵌套等待与生命周期失控 |
| 项目 3 | [并行计算优化](exercises/Capstone3_parallel_compute/README.md) | 用矩阵乘、归约和排序逐步验证布局、分块、并行的效果 |
| 17 | [诊断与源码路线](chapters/17-diagnostics-and-sources.md) | 如何从报告、等待关系和真实实现继续追到原因 |

## 专题怎样读

并发数据结构从[一把锁的队列基线](topics/queues/01-mutex-baseline.md)进入，再沿第 11 章的路线研究 SPSC、MPSC、MPMC、Treiber 与动态节点。每种结构都说明线程角色、容量、顺序、失败语义、元素限制和进展保证；不同契约的实现分组比较。

[安全回收专题](topics/reclamation/README.md)覆盖对象借用、引用计数、标签与生命期的区别、hazard pointer、EBR、QSBR 和 RCU。它们有各自的应用义务；读多写少的版本发布与动态节点摘除也有不同的接口契约。

SIMD 由第 15 章连接完整演进系列，NUMA 从[拓扑与亲和性练习](exercises/J3_numa_concept/README.md)进入。缺少真实多节点硬件时仍可学习协议和运行探测；没有放置证据的远端性能实验明确报告 SKIP。

## 一道练习的学习闭环

先根据正文预测一个具体行为，再运行 Starter 观察，随后完成对应 Part。Reference 给出完整实现与有效检查；答案解析把结果连接回状态、所有权和同步关系。多个线程的日志可能有不同合法顺序，解释日志时应依据程序保证的先后关系。

`main.cpp` 按题目分为待实现的 Starter 或可运行的观察/实验入口；`solution.cpp` 是 Reference。较大的参考实现放在本题的 `reference.hpp`、`reference/` 或公共教学头中，不应把修改参考答案作为学生作业路径。Reference 目标名在练习 ID 后加 `_reference`。学生实现测试、观察检查与 Reference 分别标记，含义见[构建指南](exercises/BUILD_GUIDE.md#验证学生程序与验证参考答案)。短正确性测试与长性能实验分别运行。

## 本轮补全入口

已有路线之外，下面三条专题按各自先修进入，不要求先完成全部高级无锁或性能内容：

- [异步日志队列与关闭](topics/logging/01-async-spdlog.md)：先修 C05 同步日志前端、本课有界队列及取消关闭；通过 [U01](exercises/U01_async_logging/README.md) 研究溢出、消息所有权、flush 和排空，服务观测仍由 C11 承接。
- [C++29 线程属性](topics/frontier/01-thread-attributes.md)：先修线程创建、对象借用、jthread 取消；[F01](exercises/F01_thread_attributes/README.md) 分别记录标准主体与教学观察，不把亲和性当作标准属性验证。
- [C++29 HP batches](topics/frontier/02-hazard-pointer-batches.md)：先修内存模型及 hazard pointer；[F02](exercises/F02_hazard_pointer_batches/README.md) 讨论资源的批量取得和释放，不把教学域的清场接口当成标准保证。

[原生设施单元](exercises/F03_native_facilities/README.md)连接标准 HP、RCU、sender 的独立主体。标准版本、是否请求、工具链能力和实际运行状态分别看待；模型通过不能补成原生 PASS。

队列的新增定位入口见[队列验证与基准](topics/queues/08-validation-and-benchmark.md)。它记录诊断口径、可复现实验命令和结论边界；旧报告和样本只保留其历史版本含义，不作为当前源码的提交内容。

## 版本、证据与覆盖

课程默认构建 C++23；常用并发设施分别来自 C++11/17/20。C++26 的 SIMD、sender/receiver、回收设施与新原子操作按实际头文件、特性宏、实例化和链接探测。标准、TS、第三方 API 的差异见[标准与实现状态](references/standards-and-implementations.md)。

[知识覆盖与迁移表](references/coverage.md)对应知识点、正文、练习和验证口径。[标准与实现状态](references/standards-and-implementations.md)记录规范、工具链能力和第三方版本。历史文件名、项目 ID 仅用于稳定导航，不决定算法实际具备什么保证。

## C02 对象生命期先修

[共享所有权](../C02_Objects_Lifetime_Ownership/chapters/09-shared-ownership.md)、[存储与对象创建](../C02_Objects_Lifetime_Ownership/chapters/12-storage-and-object-creation.md)及[别名与指针来源](../C02_Objects_Lifetime_Ownership/chapters/13-aliasing-and-provenance.md)提供语言层前提。对象尚存活不等于并发访问已同步，地址复用也不能单独证明或排除 ABA；同步、发布和安全回收仍由本课协议与实验负责。

## C07 系统与内存基础桥接

[C07 系统模型](../C07_OS_Memory_System_IO/chapters/01-system-model.md)、[虚拟内存](../C07_OS_Memory_System_IO/chapters/04-virtual-memory.md)、[pmr 与池](../C07_OS_Memory_System_IO/chapters/07-pmr.md)补齐线程所处地址空间、页与分配责任；[IPC 与文件锁](../C07_OS_Memory_System_IO/chapters/10-ipc.md)说明跨进程资源边界。共享地址不等于同步，跨进程信号也不能直接替换本课的 C++ 内存模型与回收协议。

## C10 执行协议桥接

[C10 run_loop 与环境](../C10_Execution/chapters/09-runtime.md)把 mutex/CV、发布和关闭协议用于异步执行运行时；[task/scope](../C10_Execution/chapters/10-task-and-scope.md)进一步说明完成、停止与对象收束。sender 图不替代本课的同步证明。
