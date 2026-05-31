# 练习 A-3：线程中的异常传播

> 详尽版见 `../../02-模块A-线程生命周期与jthread.md` 的 练习 A-3。

## 目标

理解“线程函数让异常逃逸顶层 → `std::terminate()`”这条铁律，并掌握安全跨线程传播异常的两条路：`std::exception_ptr` + `std::current_exception` / `std::rethrow_exception` 手动通道，以及 `std::promise::set_exception` + `future.get()` 的正式通道。

## 前置理解

- 你知道异常**不会自动跨线程传播**：主线程的 `try/catch` 接不住子线程里抛出的异常（不在同一调用栈）。
- 你知道 `std::current_exception()` 在 catch 块内返回指向当前异常的 `std::exception_ptr`，可安全跨线程传递；`std::rethrow_exception(ptr)` 在任意线程把它重新抛出。
- 你知道 `std::promise::set_exception` 让 `future.get()` 在取值处自动重抛异常。

## 必做任务

1. 在 worker 的 catch 块里用 `std::current_exception()` 把异常打包进共享的 `exception_ptr`（`main.cpp` 的 `// TODO [必做 1]`）。
2. 主线程 join 后检查该 `exception_ptr`，用 `std::rethrow_exception` 重抛并 catch 处理（`// TODO [必做 2]`）。

## 进阶任务

- 改走 promise/future 通道：worker 在 catch 里 `set_exception(std::current_exception())`，主线程 `fut.get()` 用 try/catch 接住重抛的异常（`// TODO [进阶 1]` 与 `// TODO [进阶 2]`）。

## 验收点

- 你能解释为什么子线程未捕获的异常会导致 `std::terminate()`，且主线程 try/catch 无效。
- 你能用 `exception_ptr` 把异常从 worker 安全搬运到主线程并重抛处理。
- 你能说出 promise/future 通道相对手动 `exception_ptr` 的优势（取值与错误走同一接口、`get()` 自动重抛）。

## 对应官方参考

- cppreference：`std::exception_ptr` — https://en.cppreference.com/w/cpp/error/exception_ptr
- cppreference：`std::current_exception` — https://en.cppreference.com/w/cpp/error/current_exception
- cppreference：`std::rethrow_exception` — https://en.cppreference.com/w/cpp/error/rethrow_exception
- cppreference：`std::promise::set_exception` — https://en.cppreference.com/w/cpp/thread/promise/set_exception
- Anthony Williams《C++ Concurrency in Action, 2nd ed.》第 8.4.1 节（异常与并发）、第 4.2 节（future 传播异常）。
