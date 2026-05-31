# 练习 A-2：stop_token 协作式取消

> 详尽版见 `../../02-模块A-线程生命周期与jthread.md` 的 练习 A-2。

## 目标

用 `std::stop_token`（C++20）实现**协作式取消（cooperative cancellation）**：worker 循环检查 `stop_requested()`，主线程 `request_stop()` 请求它退出；再用 `std::stop_callback` 在取消瞬间触发回调记录“取消时刻”。强调取消是协作式的——没人能强行 kill 一个线程。

## 前置理解

- 你知道 `std::stop_source` 是“开关”，`std::stop_token` 是它的只读视图，`std::stop_callback` 是开关按下瞬间触发的铃。
- 你知道 `std::jthread` 内部自带一个 `stop_source`，可用 `get_stop_token()` 取 token；若 worker 把 `std::stop_token` 作首形参，运行期会自动注入。
- 你理解 `request_stop()` 只是“请求”，真正的退出靠 worker 在检查点主动响应。

## 必做任务

1. 写一个轮询 `st.stop_requested()` 的工作循环（`main.cpp` 的 `// TODO [必做 1]`）。
2. 主线程通过 `jthread::request_stop()` 触发取消，观察 worker 干净退出（`// TODO [必做 2]`）。

## 进阶任务

- 在 worker 内用 `std::stop_callback` 注册回调，打印“取消被请求”的时刻，确认它在 `request_stop()` 后几乎立刻触发（`// TODO [进阶 1]`）。
- （已示范）用独立的 `std::stop_source`/`stop_token` 控制一个普通 `std::thread`，理解机制不绑定 jthread。

## 验收点

- 你能说清取消为何是协作式的，以及不响应 `stop_requested()` 的 worker 为何取消不掉。
- 你能从日志论证 worker 是“被取消退出”而非“跑满次数退出”。
- 你能说出 `stop_callback` 的触发线程语义（注册时已取消→在注册线程同步执行；否则→在调用 `request_stop` 的线程执行）。

## 对应官方参考

- cppreference：`std::stop_token` — https://en.cppreference.com/w/cpp/thread/stop_token
- cppreference：`std::stop_source` — https://en.cppreference.com/w/cpp/thread/stop_source
- cppreference：`std::stop_callback` — https://en.cppreference.com/w/cpp/thread/stop_callback
- Anthony Williams《C++ Concurrency in Action, 2nd ed.》第 9.2 节（中断线程 / 协作式取消）。
