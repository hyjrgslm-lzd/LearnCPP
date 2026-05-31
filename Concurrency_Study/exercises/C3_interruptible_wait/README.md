# 练习 C-3：可中断的条件等待

> 详尽版见 `../../04-模块C-条件变量.md` 的 练习 C-3。

## 目标

用 `std::condition_variable_any` + `std::stop_token` 实现可中断等待（interruptible wait，C++20）：worker 用 `cv.wait(lk, stoken, pred)` 阻塞，主线程 `request_stop()` 即可把它唤醒并优雅退出。再把“stop_token 取消”与 C-2 的“关闭队列标志”两种唤醒退出方式正面对照。

## 前置理解

- **关键事实**：`std::condition_variable`（配 `std::mutex`，C++11）没有 `stop_token` 重载；带 `stop_token` 的可中断 `wait` 是 `std::condition_variable_any` 的能力，且该重载是 **C++20 新增**。
- `cv.wait(lk, stoken, pred)` 等价于 `while (!stoken.stop_requested() && !pred()) wait(lk);`；返回 `true`=谓词满足、`false`=被取消。
- `request_stop()` 通过内部 `stop_callback` **自动 notify** 唤醒等待者，无需手动 notify。

## 必做任务

1. `// TODO [必做 1]`：用 `std::jthread`（自动传 `stop_token`、析构自动取消 + join）起 worker，用 `condition_variable_any` 的可中断 wait 等信箱非空；主线程先正常投递、再 `request_stop()` 触发取消退出。

## 进阶任务

- `// TODO [进阶 1]`：用普通 `condition_variable` + `closed_` 标志 + `notify_all` 复刻 C-2 风格的可关闭队列，与 stop_token 版对照，写清两者取舍（业务标志 vs 正交取消通道）。

## 验收点

- `request_stop()` 后 worker 被自动唤醒并优雅退出，无需手动 notify。
- 能解释 `wait(lk, stoken, pred)` 返回值含义，以及为何可中断 wait 用 `condition_variable_any`（C++20）。
- 能对比“close 标志”与“stop_token 取消”的适用场景。

## 对应官方参考

- cppreference [`std::condition_variable_any`](https://en.cppreference.com/w/cpp/thread/condition_variable_any) / [`wait`（stop_token 重载）](https://en.cppreference.com/w/cpp/thread/condition_variable_any/wait)
- cppreference [`std::stop_token`](https://en.cppreference.com/w/cpp/thread/stop_token)
- 《C++ Concurrency in Action, 2nd ed.》(Williams) 第 4 章

## 构建运行

```bash
cmake --build build-vs2026 --target C3_interruptible_wait --config Release
./build-vs2026/C3_interruptible_wait/Release/C3_interruptible_wait.exe
```
