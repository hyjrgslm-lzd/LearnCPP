# 第 16 章：调度与任务组合

把一段循环拆给四个线程，只回答了“同时有几个人干活”。如果前四分之一的数据特别难，静态分配仍可能让一个线程忙到最后；如果内存由另一个节点初始化，空闲线程接手任务后还可能付出更多远端访问；如果父任务占着 worker 等待尚未执行的子任务，线程数再多也可能全部停住。这三种情况分别涉及工作分配、数据放置和依赖完成，不能用同一个“增加并行度”解释。

本章从可运行的三种调度方式出发，推导工作窃取的队列、等待和关闭协议，再把显式 future 等待过渡到 sender 的结果组合。NUMA 系列作为调度的数据位置分支：它要求先取得 CPU 和页面证据，再谈本地性。普通代码默认 C++23；标准 execution 的说明固定对照 C++26 工作草案 N5050，实际桥接程序使用固定版本的 NVIDIA stdexec。

## 阅读与实验顺序

1. [调度 01：静态、动态与负载偏斜](../topics/scheduling/01-static-dynamic.md)。同一个确定性负载怎样得到不同的分工；任务粒度、计时边界和完成量如何保持可比。
2. [调度 02：带锁工作窃取池的完整协议](../topics/scheduling/02-work-stealing.md)。本地 LIFO、异端窃取、活跃计数、条件变量、shutdown future、构造失败和合作式等待。
3. [NUMA 01：拓扑和可用 CPU](../topics/numa/01-topology.md)，随后阅读 [线程亲和](../topics/numa/02-affinity.md)、[页面放置](../topics/numa/03-placement.md)、[按节点分片与跨节点共享](../topics/numa/04-sharding.md)。这是场景分支，不是宣称 NUMA 特化池能普遍替换普通池。
4. [调度 03：执行管线与三种完成](../topics/scheduling/03-execution-bridge.md)。理解 schedule、then、when_all、sync_wait，区分汇合任务和销毁执行资源。

对应练习为 [M1](../exercises/M1_work_stealing_pool/README.md)、[J3](../exercises/J3_numa_concept/README.md)、[N1](../exercises/N1_numa_placement/README.md)、[M2](../exercises/M2_execution_bridge/README.md)。四题 main 明确标为 OBSERVATION：M1/M2 运行独立调度 baseline/值管线，J3/N1 运行平台诊断探针，0 只覆盖实际显示的检查范围，不表示全部 Part 完成。没有 main 直接 include solution.cpp。各题 solution 独立提供完整检查。先预测、观察，再按 README 的实现任务重写相应函数并用 Reference 验证；不把观察驱动伪称未填学生代码。成功输出和退出码仅是该次运行的证据，不替代对协议的论证。

## 本章的共同边界

M1 是无界、带锁、无全局 FIFO 保证的线程池。它不保证无锁进展、不提供任务中止，也不保证任意依赖图无死锁。基准中的 static、dynamic、stealing 只对同一组独立任务做完成量比较；错误程序不参与排名。M2 的标准接口名和具体库资源不是同一层抽象；NUMA API 的分配请求与页面查询的实际结果也不是同一件事。

结课时，应能从一个具体任务追踪：谁拥有 callable，哪个队列接纳它，谁把它取走，执行失败去哪条通道，父任务如何等待，何时不再接收新任务，何时可以销毁捕获对象；若任务访问 NUMA 内存，还要回答实际在哪个 CPU 和哪些页面节点上运行。下一章的诊断路线用于检查这些假设，不能把 profiler 或 sanitizer 的无报错当作数学证明。
