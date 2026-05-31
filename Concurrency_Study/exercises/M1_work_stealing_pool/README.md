# 练习 M-1：工作窃取线程池

> 详尽版见 `../../16-模块M-工作窃取与结构化并发桥接.md` 的 练习 M-1。

## 目标

实现一个**工作窃取（work stealing）线程池**：每个 worker 持有**自己的本地双端队列（deque）**，本地任务**LIFO**地从**自己这一端（own end）**push/pop；本地空了就去**偷（steal）**别的 worker 队列**另一端（other end）**的任务。用递归分治任务（并行 fib、二分求和）验证，并对比 Capstone1 的单队列+全局锁线程池。

## 前置理解

- **本地 LIFO + 异端窃取**：owner 走 back（push_back/pop_back，取最新），小偷走 front（pop_front，取最老）。两端分离 + 每队列独立锁 → 常态**零跨线程争用**；偷取从另一端取“最老/最大”的子树，偷一次划算、频率低。
- **为什么 LIFO**：递归分治刚切出的子任务压本地栈顶，本线程下一步就取它，**缓存局部性**好、栈深可控。
- **任务粒度（granularity）**：cutoff 以下退化串行递归，避免粒度过细被调度开销吃掉（呼应模块 L「并行不是免费午餐」）。
- **合作式等待（cooperative blocking）**：fork-join 等子结果时**绝不能裸 `future.get()` 死等**——等待者本身是 worker，死睡则它队列里的子任务无人执行 → 死锁。正确做法是 `pool.wait_for(fut)`：等的同时继续 `try_run_one()` 帮忙跑任务（TBB/Cilk 的关键设计）。
- **对比单队列池（Capstone1）**：单队列池所有 worker 抢**同一把锁/同一条队列** → 高争用瓶颈；工作窃取把争用从“每次取任务”降到“偶尔偷一次”，并自动**负载均衡**。

## 必做任务

1. `// TODO [必做 1]`：`WorkStealingDeque::pop()` owner 端从 **back** 弹出（LIFO）。
2. `// TODO [必做 2]`：`steal()` 从**另一端 front** 取最老任务。
3. `// TODO [必做 3+4]`：worker 主循环 = 本地优先 + 窃取回退（victim 随机起点，避免热点）。
4. `// TODO [必做 5]`：`wait_for()` 合作式等待（等结果时继续帮忙跑任务，不死等）。
5. 跑并行 fib / 二分求和验证结果与串行一致；看析构日志的「窃取次数 vs 本地命中」比例。

## 进阶任务

- `// TODO [进阶 1]`：`parallel_sum` 二分 fork-join。
- 把加锁 deque 换成无锁 **Chase-Lev deque**（论文见下）；调 cutoff 观察粒度对窃取频率/加速比的影响。

## 验收点

- 并行 fib / 求和结果与串行**完全一致**，程序**正常退出不死锁**。
- 析构日志里**本地命中 ≫ 窃取次数**（常态走本地，窃取只在不均衡时发生）。
- 你能口述本地 LIFO / 异端窃取协议、合作式等待为何能防死锁、与单队列池的差异。

## 对应官方参考

- 《C++ Concurrency in Action, 2nd ed.》(Williams) 第 9 章（高级线程池 / 工作窃取）
- Chase & Lev, *Dynamic Circular Work-Stealing Deque* (SPAA 2005)
- cppreference [`std::deque`](https://en.cppreference.com/w/cpp/container/deque)

## 构建运行（VS2026, C++20；纯标准库，无第三方依赖）

```bash
cmake --build build-vs2026 --target M1_work_stealing_pool --config Release
./build-vs2026/M1_work_stealing_pool/Release/M1_work_stealing_pool.exe
```
