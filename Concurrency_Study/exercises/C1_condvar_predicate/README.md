# 练习 C-1：条件变量与谓词等待

> 详尽版见 `../../04-模块C-条件变量.md` 的 练习 C-1。

## 目标

用 `std::condition_variable` + `std::mutex` + 谓词（predicate）实现一次“主线程置位 `ready` 后通知 worker 开工”的交接（handoff），并亲手写出不带谓词的错误版本观察其丢失唤醒（lost wakeup），建立“CV 的 wait 永远要配谓词”的肌肉记忆。

## 前置理解

- 条件变量（condition variable，CV，`<condition_variable>`，C++11）总是与一把 mutex + 一个共享谓词状态绑定。
- `wait` 会原子地释放锁并睡去，被唤醒后重新取锁。期间锁是放开的，别的线程才能改谓词、发通知。
- 谓词版 `wait(lk, pred)` 等价于 `while (!pred()) wait(lk);`，同时防住**虚假唤醒（spurious wakeup）**与**丢失唤醒**。

## 必做任务

1. `// TODO [必做 1]`：用谓词版 wait 实现正确 handoff（worker `wait(lk, [&]{return ready;})`，主线程持锁置位后 `notify_one`）。
2. `// TODO [必做 2]`：写反面版本——主线程抢先 notify、worker 后进入裸 wait，用带超时的 `wait_for` 把“丢失唤醒”暴露成超时。

## 进阶任务

- `// TODO [进阶 1]`：3 个 worker 等同一谓词，用 `notify_all` 广播；思考误用 `notify_one` 的后果。

## 验收点

- 正确 handoff 跑通，能解释 wait 期间锁被释放。
- 反面版本观测到丢失唤醒（超时），并说清谓词版为何不会丢。
- 能区分 `notify_one` 与 `notify_all`。

## 对应官方参考

- cppreference [`std::condition_variable`](https://en.cppreference.com/w/cpp/thread/condition_variable) / [`wait`](https://en.cppreference.com/w/cpp/thread/condition_variable/wait)
- 《C++ Concurrency in Action, 2nd ed.》(Williams) 第 4 章

## 构建运行

```bash
cmake --build build-vs2026 --target C1_condvar_predicate --config Release
./build-vs2026/C1_condvar_predicate/Release/C1_condvar_predicate.exe
```
