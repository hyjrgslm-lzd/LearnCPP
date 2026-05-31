# 结课2 · 无锁队列

> 详尽版见 `../../12-第二阶段结课-无锁队列.md`（项目目标 / 固定题面 / 设计约束 / 推荐输入输出 / 必做任务 / 进阶任务 / 验收点 / 复盘问题）。

## 目标

综合阶段二（模块 E/F/G）核心，实现两个**有界无锁队列**并与加锁版做吞吐对比：

1. **SPSC 有界环形队列** `SpscRingBuffer<T, Capacity>`：单生产者写 `tail`、单消费者写 `head`——两个写者无竞争，**无需 CAS**，仅 `release`/`acquire` 配对传递槽位数据可见性（复习练习 G-3）。
2. **MPMC 有界队列** `MpmcBoundedQueue<T>`（Dmitry Vyukov 算法）：定长数组，**每槽一个 `atomic<size_t> sequence`** 作闸门；`enqueue`/`dequeue` 在 `tail`/`head` 票据上 **CAS 抢位** + `sequence` 协议判满/判空，**无需任何节点回收**。

**本项目刻意自包含**：有界数组复用槽位、从不释放 → 天然规避 ABA 与 use-after-free → **不依赖**模块 I 的 hazard_pointer.hpp / rcu.hpp。

## 前置理解

- 练习 E-3：CAS（`compare_exchange`）循环。
- 练习 F-1 / F-4：release/acquire 配对建立 happens-before、发布模式。
- 练习 G-2 / G-3：ABA 问题、SPSC 无锁环形缓冲。

## 必做任务（详见根目录文档）

1. 跑通 SPSC 正确性（FIFO + 不丢数据）。
2. 设计 MPMC 数据布局（Cell{sequence,data} + 两个票据计数；容量 2 的幂）。
3. 画 MPMC 单 cell 的 sequence 状态机（`i` → `pos+1` → `pos+capacity`）。
4. 实现 `enqueue`：`acquire` 读 sequence → CAS 抢 `enqueue_pos_`（`relaxed`）→ 写数据 → `release` 发布 `sequence=pos+1`。
5. 实现 `dequeue`：对称——与 `pos+1` 比 → CAS 抢 `dequeue_pos_` → 取数据 → `release` 置 `sequence=pos+capacity`。
6. 写吞吐基准：MPMC 无锁 vs `mutex`+`queue` 加锁版，打印 ops/sec。
7. 写 MPMC 正确性测试：唯一编号 + 标记表核对"恰好一次"（不丢/不重/守恒）。
8. 逐处标注每个 `memory_order` 的理由（最重要的产出物）。

## 进阶任务

- 全 `seq_cst` 对比；`atomic_wait`/`notify` 包成阻塞版；wait-free 边界讨论；
- `alignas(64)` 伪共享量化；对照链式无锁队列回收问题如何回来；支持移动入队。

## 验收点

- SPSC：FIFO 顺序**完全正确**、不丢数据，稳定可复现。
- MPMC：每个唯一编号**恰好被消费一次**（不丢/不重/守恒），稳定可复现。
- 能逐处说清每个 `memory_order` 的理由，尤其"ticket 的 CAS 为何可 `relaxed`、数据发布为何须 `release`"。
- 能解释为何"有界 + 槽位复用"天然规避 ABA / use-after-free，从而无需 hazard pointer/RCU。
- 吞吐基准能跑出无锁版与加锁版的 ops/sec 与比值。

## 骨架现状

`main.cpp` 为可编译运行的骨架：SPSC 与加锁基线已是完整实现；MPMC 的 `enqueue`/`dequeue` 用 `// TODO [必做 4/5]:` 标注，参考代码以注释给出。未填 TODO 时 MPMC 退化为"内部一把 mutex 串行化"的占位实现——输出仍正确（不丢/不重），但**不是无锁**、吞吐也不高（基准里与加锁版相近）。按 TODO 把占位段替换为 Vyukov 的 CAS 协议即可得到真正的无锁队列。

## 编译运行（VS2026, C++20）

```bash
cmake --build build-vs2026 --target Capstone2_lockfree_queue --config Release
```

## 对应官方参考

- 《C++ Concurrency in Action, 2nd ed.》(Anthony Williams) 第 7 章（无锁数据结构、无锁队列、内存序）
- Dmitry Vyukov，"Bounded MPMC queue"：<https://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue>
- moodycamel ConcurrentQueue：<https://moodycamel.com/blog/2014/a-fast-general-purpose-lock-free-queue-for-c++>
- cppreference：`std::atomic` / `compare_exchange` / `memory_order`
