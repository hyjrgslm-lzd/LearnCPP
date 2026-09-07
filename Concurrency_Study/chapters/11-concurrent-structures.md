# 11 并发数据结构：从契约推导同步与回收

并发容器的第一道题并不是“这里用哪一种 CAS”，而是“一个成功或失败的调用向使用者承诺了什么”。如果消费者拿到 false，能否据此宣告任务结束？如果两个生产者先后返回，消费者应当观察到什么顺序？如果一个线程被挂起，其他线程能否继续搬运元素？这些问题会决定我们能选择的算法，而算法名称本身不能代替答案。

本章把同一个传输问题分成连续路线。先运行已经检查过的 mutex 基线，再逐步改变存储方式、调用粒度、线程角色和内存管理。完整正文与代码如下；应按箭头顺序阅读主线，Treiber/ABA 是用于理解动态节点的独立分支。

| 正文 | 实现与 Reference | 演进性质 |
|---|---|---|
| [01 mutex 基线](../topics/queues/01-mutex-baseline.md) | Q0、queue_baseline.hpp | 固定容量、严格 FIFO 的起点 |
| [02 预分配与批量](../topics/queues/02-bounded-and-batch.md) | mutex_ring、Capstone2 | 单元素接口保持契约；批量扩展调用粒度 |
| [03 SPSC](../topics/queues/03-spsc.md) | spsc_ring、G3 | 限制为一生产者一消费者；缓存下标是分支内改进 |
| [04 MPSC](../topics/queues/04-mpsc.md) | mpsc_ring、Q1 | 多生产者预订、单消费者；允许暂时失败 |
| [05 Vyukov MPMC](../topics/queues/05-vyukov-mpmc.md) | mpmc_ring、Capstone2 | 多消费者竞争票据，保留发布空隙 |
| [06 Michael–Scott](../topics/queues/06-michael-scott.md) | ms_queue、Q2 | 无界链式 FIFO；加入真实 HP 回收 |
| [07 Treiber 与 ABA](../topics/queues/07-treiber-and-aba.md) | treiber_stack、G1、G2 | LIFO 独立分支与安全逻辑反例 |
| [08 验证与基准](../topics/queues/08-validation-and-benchmark.md) | queue_history_test、queue_bench | 历史、逐项完整性、实测边界 |

## 为什么需要这么多版本

一把锁把“观察容量、转移元素、更新状态”包在同一临界区内，很容易给出串行解释。预分配可以移走热路径的容器分配，批量可以降低每个元素分摊的加锁次数，这些改进仍然可能保留这把锁。只有线程角色受限时，我们才有理由删除某些竞争协议：SPSC 的写下标只有一个写者，所以无需 CAS；MPSC 的读票据也只有一个写者，但生产者之间仍然需要仲裁。

当我们把“我拿到了第 p 个位置”和“第 p 个元素已经可读”分成两个步骤，就必须面对持票者被暂停的情形。后来完成的生产者不能替前面的生产者发布任意 T。此时返回 false 的含义会变弱。若忽略这个变化，使用者可能把“前面还没发布”误当作“任务已经清空”，这不是一个性能小问题，而是结束协议错误。

链式队列又改变了容量与生命周期。节点从根指针上摘除，并不意味着没有读者持有它。真正的完整操作必须包括保护、退休与释放。课程的 `cs::hazard_pointer` 是 C++23 教学实现，接口形状借鉴标准 HP；其扫描和退休有 mutex 与分配，所以不能把核心指针算法的进展保证直接贴在整个类上。

## 核心协议与完整操作要分别判断

下表是本课程代码的承诺，不是对所有同名库的概括。假设调用者遵守线程角色、生命周期和计数距离前提。所谓“有界步数”还假设单个原子与元素操作本身在有界步数内完成；仅 `is_lock_free()==true` 不足以证明底层 wait-free。

| 实现 | 核心同步协议 | 包含元素、分配、回收的完整操作 |
|---|---|---|
| mutex_queue / mutex_ring | mutex 串行化 | 可能阻塞；基线插入还可能分配 |
| spsc_ring 两版 | 固定数量的 load/store，无 CAS 重试 | 预分配；仍受实际原子和 T 操作进展限制 |
| mpsc_ring | 生产者 CAS；消费者固定步骤 | 持票者暂停可堵住后续传输；不承诺严格 lock-free FIFO |
| mpmc_ring | 两端 CAS + 每槽 sequence | 相同空隙；单个线程也可能持续重试 |
| ms_queue / treiber_stack | 安全指针寿命及无锁原子前提下，经典核心为 lock-free、非 wait-free | new、HP 槽获取、退休记录、扫描及删除器可能分配或阻塞 |

每个原子版本提供 `atomics_lock_free()`，由 Reference 输出当前对象的实测值；它不测分配器、mutex、扫描器、元素复制，更不代表“在所有平台上无锁”。

## 本章完成标准

学习后应能拿起一份实现，从入口沿着每一次共享读写讲到函数返回和最终析构：谁拥有这个槽或节点，哪次操作发布了它，哪个同步关系允许读取，失败是否改变输出，哪个线程负责释放。对所有必做 Part，练习目录里的 `solution.cpp` 给出可运行答案；对多消费者顺序，另用小规模调用/返回历史检验，不能把日志打印顺序当作线性化顺序。

基础规范使用 [C++26 工作草案 N5050](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf) 的稳定条款标签 `[intro.races]`、`[atomics.order]`、`[atomics.types.operations]`、`[saferecl.hp]`。本章代码默认 C++23，不要求实现原生 C++26 HP，也不把滚动草案误当作固定的 C++26 文本。算法出处与本课程增加的保护协议在各篇分别说明。
