# 练习 G-3：SPSC 无锁环形缓冲

> 详尽版见 `../../09-模块G-无锁数据结构.md` 的 练习 G-3。

## 目标

实现单生产者单消费者（single-producer single-consumer，SPSC）的无锁环形缓冲：定长数组 + 两个原子下标 `head` / `tail`，生产者只写 `tail`、消费者只写 `head`，用 release/acquire 配对传递数据可见性。验证 FIFO（先进先出）顺序与不丢数据。

## 前置理解

- **SPSC 的灵魂假设**：`tail` 只被生产者写、`head` 只被消费者写。两个写者**各自独占**一个原子下标 → 没有写-写竞争 → 无需 CAS，只要 `load`/`store`。
- **release/acquire 配对**：生产者先写槽位数据、再 `release` 提交 `tail`；消费者 `acquire` 读 `tail`，从而“看到新 tail 时槽位数据也已可见”（happens-before）。`pop` 同理用 `release` 提交 `head` 通知生产者空出位置。
- 满/空判定（留一格法）：空 = `head == tail`；满 = `(tail+1)%N == head`，实际可用容量 `N-1`。

## 必做任务

1. `// TODO [必做 1]`：实现 `push`/`pop`，含满/空判定与正确的 release/acquire 内存序（读自己写的下标用 relaxed，读对方写的下标用 acquire，提交用 release）。
2. `// TODO [必做 2]`：单生产者单消费者各跑大量元素，验证消费总数 == 生产总数（不丢），且消费序列严格递增（FIFO 正确）。

## 验收点

- 单生产者单消费者跑通，不丢数据、FIFO 顺序正确。
- 能说清为何 SPSC 不需要 CAS（每个下标只有一个写者）。
- 能指出哪一对 release/acquire 在传递槽位数据的可见性。

## 对应官方参考

- cppreference [`std::atomic`](https://en.cppreference.com/w/cpp/atomic/atomic) / [`memory_order`](https://en.cppreference.com/w/cpp/atomic/memory_order)
- 《C++ Concurrency in Action, 2nd ed.》(Williams) 第 7 章
- moodycamel 博客 "A Fast Lock-Free Queue for C++"

## 构建运行

```bash
cmake --build build-vs2026 --target G3_spsc_ringbuffer --config Release
./build-vs2026/G3_spsc_ringbuffer/Release/G3_spsc_ringbuffer.exe
```
