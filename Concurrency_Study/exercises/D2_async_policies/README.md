# 练习 D-2：std::async 启动策略

> 详尽版见 `../../05-模块D-future与异步任务.md` 的 练习 D-2。

## 目标

用 `std::async` 拿回 `future`，对比三种启动策略在**执行线程**与**执行时机**上的差异；正面演示并解释 future 模型最著名的陷阱：**`std::async` 返回的 future 析构会阻塞**，不保存返回值（临时 future）会让多个 async 调用退化成串行。

## 前置理解

- 三种策略：
  - `std::launch::async`：**保证**新线程、**立即**开跑（不等 `get()`）。
  - `std::launch::deferred`：**惰性**；直到 `get()`/`wait()` 才在**调用线程**上**同步**执行（无新线程）。
  - 默认（`async | deferred`）：实现自选，**不可假设是异步**。
- **析构阻塞陷阱**：与 `launch::async` 关联的 future，其析构函数会阻塞到任务结束（仿佛隐式 `wait()`）。临时 future 在语句分号处即析构 → 当场阻塞 → 多个调用串行。要并行须把每个 future 存进变量（如 `std::vector<std::future<T>>`）。
- 只有 `std::async` 返回的（async 策略）future 有此特例；普通 promise/future、packaged_task 的 future 析构**不**阻塞。

## 必做任务

1. `// TODO [必做 1]`：用三种策略各调一次，打印执行线程 id 与 `wait_for(0s)` 状态，对比线程与时机差异。
2. `// TODO [必做 2]`：反面（不保存 future → 串行，计时 ≈ N×单任务耗时）vs 正面（存进 vector → 并行，计时 ≈ 单任务耗时）。

## 进阶任务

- `// TODO [进阶 1]`：对 deferred future 多次 `wait_for(0s)` 始终为 `deferred`，`get()` 才在主线程触发；并让 async 任务抛异常，观察 `get()` 重新抛出。

## 验收点

- 能用三种策略各跑一次并解释“线程/时机”差异。
- 能用计时数据证明“不保存 future → 串行”“保存 → 并行”，并指出根因是 future 析构阻塞。
- 能说清 `deferred` 是在 `get()` 调用线程上的同步惰性求值，及默认策略的不确定性。

## 对应官方参考

- cppreference [`std::async`](https://en.cppreference.com/w/cpp/thread/async) / [`std::launch`](https://en.cppreference.com/w/cpp/thread/launch) / [`std::future::wait_for`](https://en.cppreference.com/w/cpp/thread/future/wait_for)
- 《C++ Concurrency in Action, 2nd ed.》(Williams) 第 4 章 4.2.1；Meyers《Effective Modern C++》Item 35–36

## 构建运行

```bash
cmake --build build-vs2026 --target D2_async_policies --config Release
./build-vs2026/D2_async_policies/Release/D2_async_policies.exe
```
