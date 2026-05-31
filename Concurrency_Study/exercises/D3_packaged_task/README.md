# 练习 D-3：packaged_task 打包任务

> 详尽版见 `../../05-模块D-future与异步任务.md` 的 练习 D-3。

## 目标

用 `std::packaged_task` 把可调用对象打包成“可投递、执行后自动把结果/异常写进 future”的任务单元；用 `get_future()` 取 future；把若干 packaged_task 投进任务队列由 worker 线程执行，主线程统一收集 futures。理清 `promise`/`packaged_task`/`async` 的分工，并为 Capstone1 线程池埋下“任务 = packaged_task”的接口直觉。

## 前置理解

- `std::packaged_task<R(Args...)>`（`<future>`，C++11）包装可调用对象，**本身也可调用**：`task(args...)` 执行内部对象并把返回值/异常**自动写入**其共享状态，`get_future()` 取出的 future 随即就绪。
- 三者分工：`promise` 手动 `set_value`/`set_exception`（最底层）→ `packaged_task` 自动 set（调用即写入）→ `async` 连起线程/调度都包办（最高层，策略受限）。
- `packaged_task` 是 **move-only**；不同签名是不同类型。统一成 `packaged_task<void()>`（或 `std::function<void()>`）做类型擦除，才能放进同一个队列。
- 任务与执行解耦：`get_future()` 在投递前就拿到；任务在哪个线程、何时被调用与“谁持有 future”无关——这正是线程池核心抽象。

## 必做任务

1. `// TODO [必做 1]`：创建 `packaged_task<int()>`，**先 `get_future()` 存好**，再 `std::move` 到另一线程调用 `task()`，主线程 `get()` 拿结果（无需手动 set_value）。
2. `// TODO [必做 2]`：造 N 个 task，逐个先 `get_future()` 存 vector、再 move 进队列；worker 线程取出 `task()` 执行，主线程统一 `get()`。
3. `// TODO [必做 3]`：让一个任务抛异常，验证它经同一条共享状态通道在 `get()` 处重新抛出。

## 进阶任务

- `// TODO [进阶 1]`：把“队列 + worker”收口成最小 `submit` 接口雏形（`packaged_task` + 类型擦除 + 返回 `future<R>`），体会 Capstone1 线程池 `submit` 的形状；起多 worker 验证任务被并行分摊。

## 验收点

- 能打包对象、在另一线程调用、经 future 拿结果（无需手动 set_value）。
- 能把多个 packaged_task 投队列、worker 执行、主线程统一收集 futures。
- 能让任务抛异常并在 `get()` 处捕获，验证异常走同一套机制。
- 能讲清 promise / packaged_task / async 的递进分工。

## 对应官方参考

- cppreference [`std::packaged_task`](https://en.cppreference.com/w/cpp/thread/packaged_task) / [`get_future`](https://en.cppreference.com/w/cpp/thread/packaged_task/get_future) / [`operator()`](https://en.cppreference.com/w/cpp/thread/packaged_task/operator())
- 《C++ Concurrency in Action, 2nd ed.》(Williams) 第 4 章 4.2.2；第 9 章

## 构建运行

```bash
cmake --build build-vs2026 --target D3_packaged_task --config Release
./build-vs2026/D3_packaged_task/Release/D3_packaged_task.exe
```
