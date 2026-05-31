# 练习 H-1：latch 与 barrier

> 详尽版见 `../../10-模块H-高级同步原语.md` 的 练习 H-1。

## 目标

对比 C++20 两个新栅栏原语：`std::latch`（闩，`<latch>`，**一次性**向下计数栅栏）与 `std::barrier`（屏障，`<barrier>`，**可重用**的多阶段汇合点）。用 latch 实现“全部就绪后同时开跑”，用 barrier 实现多轮迭代的相位同步 + completion function（完成函数）做每轮收尾。

## 前置理解

- **`std::latch`**：内部一个计数。`count_down(n=1)` 减计数，`wait()` 阻塞到计数归零，`arrive_and_wait()` = `count_down()+wait()`，`try_wait()` 非阻塞查询。**计数到 0 后不可重用**（无法重置回初值）。
- **`std::barrier`**：维护“代 / 阶段（generation / phase）”。`arrive_and_wait()` 到达并等待；每阶段所有参与者到齐后，构造时给的 **completion function** 由实现选定的**某一个线程执行恰好一次**，再唤醒全部，barrier 自动重置进入下一代——**可重用**。还有 `arrive()`（返回 token）、`arrive_and_drop()`（本线程退出后续参与，到齐人数 -1）。completion function 必须 `noexcept`。
- **核心区别**：latch 是“等 N 件事都完成”的一次性闸门；barrier 是“反复汇合 N 个参与者”的循环节拍器。

## 必做任务

1. `// TODO [必做 1]`：用 latch 实现“全部就绪后同时开跑”——每个 worker 准备好后 `count_down()` 报到，主线程 `wait()` 到全部就绪后用发令枪 latch `count_down(1)` 放行，worker 在 `go.wait()` 处几乎同时被唤醒。
2. `// TODO [必做 2]`：用 barrier 做多轮相位同步——worker 每轮算一块后 `arrive_and_wait()`，completion function 在每轮末打印本轮汇总并清零，barrier 自动进入下一轮。

## 验收点

- latch 演示中所有 worker 在发令枪后几乎同时起跑，且 latch 归零后不可重用这一点能说清。
- barrier 演示中 completion function 每轮**恰好执行一次**（可观察到执行它的线程 id 在不同轮可能不同），且 barrier 跨多轮**重用**成功。
- 能说清 latch（一次性）与 barrier（可重用 + completion）的本质区别与各自适用场景。

## 对应官方参考

- cppreference [`std::latch`](https://en.cppreference.com/w/cpp/thread/latch) / [`std::barrier`](https://en.cppreference.com/w/cpp/thread/barrier)
- 《C++ Concurrency in Action, 2nd ed.》(Williams) 第 4 章（latch 与 barrier）
- 提案 P1135R6（The C++20 Synchronization Library）

## 构建运行

```bash
cmake --build build-vs2026 --target H1_latch_barrier --config Release
./build-vs2026/H1_latch_barrier/Release/H1_latch_barrier.exe
```
