# 练习 H-2：counting / binary semaphore

> 详尽版见 `../../10-模块H-高级同步原语.md` 的 练习 H-2。

## 目标

掌握 C++20 信号量（`<semaphore>`）：`std::counting_semaphore<N>`（计数信号量）做**限流**——任意时刻最多 K 个线程持有资源；`std::binary_semaphore`（= `counting_semaphore<1>`）做线程间**一次性信号 / 唤醒**。

## 前置理解

- **`std::counting_semaphore<LeastMaxValue>`**：内部一个非负计数。`acquire()` 计数 -1（为 0 则阻塞），`release(n=1)` 计数 +1（唤醒等待者）；`try_acquire()` 非阻塞尝试，`try_acquire_for(d)` / `try_acquire_until(t)` 限时尝试。模板参数是计数上限，构造参数是初始计数。
- **`std::binary_semaphore`** = `counting_semaphore<1>`：计数只在 0/1 间，做轻量信号。
- **semaphore vs mutex**：mutex 有**所有权**（谁锁谁解、天生 1 份）；semaphore **无所有权**——A 线程 `acquire`、B 线程 `release` 完全合法，计数可 > 1。所以信号量适合“资源计数 / 跨线程发信号”，而非“互斥保护一段代码”。

## 必做任务

1. `// TODO [必做 1]`：用 `counting_semaphore<K>` 实现资源池信号量——进资源区前 `acquire`、出区后 `release`，验证临界区内并发数从不超过 K（程序打印峰值占用与限额比对）。
2. `// TODO [必做 2]`：用 `binary_semaphore` 做一次性 ping-pong 通知——ping 准备好数据后 `release` 通知 pong，pong `acquire` 等信号；再用第二个二值信号量让 pong 回告 ping，演示双向交棒。

## 验收点

- counting_semaphore 演示中峰值同时占用 **≤ K**（无越界）。
- binary_semaphore 演示中 ping/pong 一次性握手成功，且能说清 `release`/`acquire` 可由不同线程执行（无所有权）。
- 能区分 semaphore（无所有权、可计数）与 mutex（有所有权、互斥）的适用场景。

## 对应官方参考

- cppreference [`std::counting_semaphore`](https://en.cppreference.com/w/cpp/thread/counting_semaphore)
- 《C++ Concurrency in Action, 2nd ed.》(Williams) 第 4 章（信号量）
- 提案 P1135R6（The C++20 Synchronization Library）

## 构建运行

```bash
cmake --build build-vs2026 --target H2_semaphore --config Release
./build-vs2026/H2_semaphore/Release/H2_semaphore.exe
```
