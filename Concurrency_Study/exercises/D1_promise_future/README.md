# 练习 D-1：promise / future 基础

> 详尽版见 `../../05-模块D-future与异步任务.md` 的 练习 D-1。

## 目标

用 `std::promise`（写端）/ `std::future`（读端）实现一条跨线程的一次性通道（one-shot channel）：worker 用 `set_value` 回传结果、用 `set_exception` 回传异常，主线程 `future.get()` 取出（异常被重新抛出）。再用 `std::shared_future` 让多个消费者等同一个结果。

## 前置理解

- future 模型（`<future>`，C++11）把“一个将来才就绪的值/异常”抽象成对象，替代你手写的 mutex + condition_variable + ready 标志样板。
- `promise.get_future()` 取出配对的 future。`promise`/`future` 都是 **move-only**；`shared_future` 可拷贝。
- 一次性：同一 promise 设值两次抛 `promise_already_satisfied`；`future.get()` 取一次即失效，再取是错误（需多次/多消费者用 `shared_future`）。
- 异常被**存入**通道、推迟到消费方 `get()` 才抛出。promise 不设值就析构 → future 收到 `broken_promise`。

## 必做任务

1. `// TODO [必做 1]`：worker move 进 `promise<int>`，`set_value` 回传结果；主线程 `future.get()` 取出（观察 get 后 future 失效）。
2. `// TODO [必做 2]`：worker `set_exception(std::current_exception())` 回传异常；主线程在 `try{ fut.get(); }catch` 处接住。
3. `// TODO [必做 3]`：`fut.share()` 转 `shared_future`，按值拷贝给多个等待线程，生产方置值后各自 `get()` 拿到同一结果。

## 进阶任务

- `// TODO [进阶 1]`：让 promise 不设值就出作用域，观察 `future.get()` 抛 `std::future_error`（`broken_promise`）。

## 验收点

- 能用 promise/future 跨线程传值，解释 `get()` 的阻塞与“取一次即失效”。
- 能用 `set_exception` 跨线程传异常并在 `get()` 处捕获。
- 能用 `shared_future` 让多消费者拿同一结果，说清它与 `future` 的所有权差异。
- 能说出 `broken_promise` 何时发生、保护了什么。

## 对应官方参考

- cppreference [`std::promise`](https://en.cppreference.com/w/cpp/thread/promise) / [`std::future`](https://en.cppreference.com/w/cpp/thread/future) / [`std::shared_future`](https://en.cppreference.com/w/cpp/thread/shared_future)
- cppreference [`std::future_error`](https://en.cppreference.com/w/cpp/thread/future_error)
- 《C++ Concurrency in Action, 2nd ed.》(Williams) 第 4 章 4.2

## 构建运行

```bash
cmake --build build-vs2026 --target D1_promise_future --config Release
./build-vs2026/D1_promise_future/Release/D1_promise_future.exe
```
