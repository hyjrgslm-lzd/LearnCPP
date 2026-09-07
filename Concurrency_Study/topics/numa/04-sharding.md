# NUMA 04：按节点分片以后，调度必须尊重数据位置

上一节把页面放置变成了可查询的状态。现在假设两个 reader 分别处理一半数据。若这两半由一个节点上的主线程初始化，另一节点的 reader 会读取远端页面。即使计算量均分，内存访问路径仍然不均匀。改变初始化责任，使每个 reader 首次写入自己未来使用的页面，才有机会同时获得计算分工和数据局部性。

本篇真实对照是 [`placement_experiment`](../../exercises/include/concurrency_study/numa.hpp) 的 firsttouch 与 parallel-init。跨节点共享的检查在 [N1 solution.cpp](../../exercises/N1_numa_placement/solution.cpp) 的 `shared_read`。所有 NUMA 名称都要求实际页节点证据；机器不具备条件时，程序只报告已运行的本地部分。

## 1. 分片同时涉及数据、初始化者和消费者

设 N 页分给 P 个 reader。每个 reader 得到完整的连续页区间，尾部余数分给前几个 reader。parallel-init 中，第 i 个 reader 在自己绑定的 CPU 上初始化这个区间；正式读取仍由同一个目标 CPU 负责同一范围。因而我们能为每一页写出一个预期 node 向量，并与查询结果逐项比较。

firsttouch 作为对照保留相同的 reader、相同区间、相同读取次数，只由第一个 CPU 初始化所有页。两版计时都排除初始化本身，所以它们研究的是“初始化决策留下的页面位置怎样影响后续读取”，而不是比较并行 memset 与串行 memset 的速度。

这一安排属于静态按节点分片，不能概括为通用负载均衡。如果每一页需要的计算时间差异很大，某个节点的 worker 仍可能最晚完成。此时需要同时考虑调度收益和移动工作的代价，而不能只看哪条队列最长。

## 2. 为什么全局工作窃取不自动等于 NUMA 友好

M1 的 [`work_stealing_pool`](../../exercises/include/concurrency_study/work_stealing_pool.hpp) 不读取 CPU 或页面拓扑，也不绑定 worker。它平衡的是可执行任务，无法识别任务引用的页面在哪个 node。一个空闲 worker 偷到任务以后，可能从原来的本地访问变成大量远端访问。

考虑任务预计还有 C 时间的计算、需要扫描 B 字节远端数据。窃取后若减少的排队等待大于新增访问代价，迁移执行可能值得；反之可能只是让更多互联流量换来相似的完成时间。这里没有给 C、B 或带宽填一个固定常数，因为机器、访问模式和缓存状态不同。这是决策模型，不是当前硬件已测出的公式。

一种后续设计是先在 node 内窃取，再在持续空闲时跨 node，且任务描述带有数据归属。另一种是保持数据分片，向拥有该数据的 worker 发送小请求，由它局部处理并返回小结果。两者都改变了任务路由和等待契约，需要独立实现与验证。本课程已经运行的是按页静态分片和通用窃取两条分支，没有把它们拼成一个未经验证的“NUMA aware pool”。

同样，给 M1 worker 一次性绑核，不会自动迁移已存在的任务数据。就算 lambda 很小，它按引用访问的数组也可能来自另一个节点；callable 对象在哪个队列里，与业务数据在哪个节点，是两张不同的映射。

## 3. 共享只读数据与共享可写状态

并不是所有数据都能按 reader 完全分开。一个只读索引可能被所有节点反复查阅。N1 的 `shared_read` 明确在 home node 上请求并初始化同一组页，查询其驻留位置，然后选至多两个不同 node 的 CPU，让每个 reader 完整读取相同页。每个 reader 的总和都要正确，且读取前后页节点不变。

这个检查验证“多个 reader 安全共享同一份已初始化的不可变数据”，并观察其执行位置。没有写入者，因此没有数据竞争；仍可能存在远端读取和带宽压力。缓存可能让部分读取命中本地 cache，所以“页面 home 是 remote”也不意味着每条 load 都访问远端 DRAM。程序没有硬件事件计数，不能把这个检查输出叫作远端总线事务数。

若把只读索引改成全局共享计数器，问题又变了。多个节点更新同一缓存行需要同步和一致性协调；memory page 的 home 节点并不能完整描述缓存行所有权的移动。把每个 worker 的计数先存在独立槽位，join 后再归并，常可减少共享更新，但必须接受“运行中没有即时全局精确计数”的契约变化。该变化应与缓存章节的布局实验一起分析，不能只贴一个 NUMA 标签。

只读复制是另一种选择：每 node 一份索引，减少跨节点读取，代价是空间、初始化和更新协议。如果索引偶尔更新，需要定义如何发布新版本、旧版本何时无人使用。这又回到发布与回收章节，而不是操作系统替程序解决对象寿命。

## 4. 把实验控制在一个主因素内

首先跑 firsttouch 和 parallel-init，只改变初始化者，保存相同的大小、reader CPU 和范围。接着可以固定页面安排，只调整每页计算量，研究从带宽限制转向计算限制的过程；这需要扩展当前扫描 kernel，并给新增逻辑留下检查。不要在同一轮同时改 reader 数、线程绑定、数组大小、访问顺序和初始化策略。

shared_read 是场景切换：每个 reader 都读全量数据，总读取量随 reader 数增加，与分片总计只读一遍的实验不同。不能把它的耗时直接放进分片版本的排名。它在 Reference 中作正确性与放置检查，不输出同工作量的性能排名。

单节点下 firsttouch 与 parallel-init 仍应覆盖同样的页、得到相同总和、在页查询中都属于 node 0。这时差异可能来自线程启动、缓存和噪声，而不是远端内存。`shared_read` 只运行一个 home reader 并说明缺少跨节点 reader；remote/interleaved 仍返回 77。这样的结果完整地回答了当前机器能验证什么。

memory node 数与可用 reader 所在 node 数还可能不同：即使有两个内存节点，允许 CPU 也可能全在 node 0。N1 的 shared_read 这时完成本地检查并把自己的 Part 标为 SKIP；其余放置 Part 继续执行，最终汇总保持 77。solution 用模拟拓扑检查这个状态合并逻辑，但模拟数据不调用平台 API，也不输出跨节点性能数字。

## 自测与答案

**parallel-init 一定比 firsttouch 快吗？** 不一定。没有远端节点、数据留在缓存、扫描太短或初始化分工与实际消费不同，都可能消除收益；只要求数据正确和放置证据成立。

**把任务移到数据所在 node，就不用同步了吗？** 仍需同步。node 不是互斥域，同一个 node 上有多个执行者；对象的数据竞争和发布规则不变。

**共享只读页应复制还是交错？** 取决于读取量、更新频率、空间和各节点需求。复制增加空间并需要版本管理；交错分散 home 节点但单个 reader 仍访问部分远端页。应先固定契约，再分别测量。

**为什么 N1 的 shared_read 不与 parallel-init 比毫秒？** 前者每个 reader 读全量数据，后者 reader 合计读一遍。完成量和工作内容不同，直接排名会混淆问题。

## 依据

- [Linux 内核内存策略](https://www.kernel.org/doc/html/latest/admin-guide/mm/numa_memory_policy.html) 与 [libnuma 官方手册](https://github.com/numactl/numactl/blob/master/numa.3) 说明策略与节点限制；本篇分片及调度选择由课程代码和工作量推导。
- [Microsoft NUMA Support](https://learn.microsoft.com/en-us/windows/win32/procthread/numa-support) 给出 Windows 的节点、分配和页面查询入口。
- [C++26 N5050](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf) 的 `[intro.races]` 和对象生命周期条款适用于所有节点上的共享访问；NUMA 不修改这些规则。
