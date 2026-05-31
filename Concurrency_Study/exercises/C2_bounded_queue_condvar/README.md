# 练习 C-2：用条件变量实现有界队列

> 详尽版见 `../../04-模块C-条件变量.md` 的 练习 C-2。

## 目标

用一把 `std::mutex` + 两个 `std::condition_variable`（`not_full` / `not_empty`）实现有界阻塞队列（bounded blocking queue）：`push` 满则等、`pop` 空则等。多生产者多消费者（MPMC）跑通，正确选择 `notify_one`；进阶加 `close()` 让消费者优雅退出。

## 前置理解

- 这是 C-1 的延伸：生产者等“队列非满”、消费者等“队列非空”，是两个不同谓词 → 用两个 CV 精确分流。
- 一次 `push` 只新增 1 个可消费元素、一次 `pop` 只空出 1 个位置 → 各自 `notify_one` 即可，`notify_all` 会引发惊群（thundering herd）。

## 必做任务

1. `// TODO [必做 1]`：实现 `push`（`not_full_.wait` → 入队 → `not_empty_.notify_one`）与 `pop`（`not_empty_.wait` → 出队 → `not_full_.notify_one`）。
2. `// TODO [必做 2]`：3 生产者 × 2 消费者跑通，消费总数应 == 生产总数（容量设小以逼出阻塞路径）。

## 进阶任务

- `// TODO [进阶 1]`：`close()` 置 `closed_` 并对两个 CV `notify_all`；`pop` 在“已关闭且排空”时返回 `std::nullopt`，消费者据此优雅退出（allow drain）。

## 验收点

- 小容量下 MPMC 正确跑通，无死锁、无漏取。
- 能解释为何用两个 CV、为何 push/pop 各选 `notify_one`、为何 `close()` 必须 `notify_all`。
- 谓词里 `|| closed_` 是“关闭能穿透阻塞”的关键。

## 对应官方参考

- cppreference [`std::condition_variable`](https://en.cppreference.com/w/cpp/thread/condition_variable) / [`notify_one`](https://en.cppreference.com/w/cpp/thread/condition_variable/notify_one)
- 《C++ Concurrency in Action, 2nd ed.》(Williams) 第 4 章 4.1.1

## 构建运行

```bash
cmake --build build-vs2026 --target C2_bounded_queue_condvar --config Release
./build-vs2026/C2_bounded_queue_condvar/Release/C2_bounded_queue_condvar.exe
```
