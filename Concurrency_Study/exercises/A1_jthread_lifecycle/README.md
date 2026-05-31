# 练习 A-1：jthread 生命周期与自动 join

> 详尽版见 `../../02-模块A-线程生命周期与jthread.md` 的 练习 A-1。

## 目标

用 `std::thread`（C++11）与 `std::jthread`（C++20）各起一个 worker，亲手观察 `jthread` 离开作用域时**自动 `request_stop()` + `join()`**，并理解“未 join/detach 的 `std::thread` 析构会调用 `std::terminate()`”这条铁律（用不真的崩溃的方式演示）。

## 前置理解

- 你知道 `std::thread` 创建后处于「可结合（joinable）」状态，析构前必须 `join()` 或 `detach()`，否则 `~thread()` 调用 `std::terminate()`。
- 你知道 `std::jthread` 是带 RAII 自动 join 的线程：析构时先 `request_stop()` 再 `join()`。
- 你能用 `cs::log` / `cs::logf` 打印带线程 id 与时间戳的日志，从时间线读出事件先后。

## 必做任务

1. 用 `std::thread` 起一个 worker 并**手动 `join()`**（`main.cpp` 的 `// TODO [必做 1]`）。
2. 用 `std::jthread` 起一个 worker，**不手动 join**，从日志确认它离开作用域时自动 join（`// TODO [必做 2]`）。

## 进阶任务

- 让 jthread 的 worker 把 `std::stop_token` 作为第一个形参，循环条件改为 `!st.stop_requested()`，观察析构时自动 `request_stop()` 触发协作式退出（`// TODO [进阶 1]`）。

## 验收点

- 你能解释为什么忘记 join 的 `std::thread` 析构会 `terminate`，而 `jthread` 不会。
- 你能从日志时间戳论证：jthread worker 的“干完了”一定早于其作用域结束。
- 你能说出 jthread 析构的两步顺序：先 `request_stop()`，后 `join()`。

## 对应官方参考

- cppreference：`std::jthread` — https://en.cppreference.com/w/cpp/thread/jthread
- cppreference：`std::thread::~thread` — https://en.cppreference.com/w/cpp/thread/thread/~thread
- Anthony Williams《C++ Concurrency in Action, 2nd ed.》第 2 章（线程管理）、第 9.2 节（中断线程 / jthread）。
