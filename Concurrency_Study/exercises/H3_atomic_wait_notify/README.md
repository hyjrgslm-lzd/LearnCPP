# 练习 H-3：atomic wait / notify

> 详尽版见 `../../10-模块H-高级同步原语.md` 的 练习 H-3。

## 目标

掌握 C++20 的 `std::atomic<T>::wait(old)` / `notify_one()` / `notify_all()`：用它替代忙等自旋（busy-wait / spin），实现一个高效的“一次性事件 / flag”，并讲清相对自旋的优势。

## 前置理解

- **`wait(old, mo)`**：若当前原子值**等于** `old`，则**阻塞**本线程（睡眠）；被 `notify` 唤醒且值发生改变后返回。若调用时已不等于 `old`，立即返回（不阻塞）。
- **`notify_one()` / `notify_all()`**：唤醒一个 / 全部正在 `wait` 的线程。
- **相对 spin 的优势**：自旋反复 `load` 复查，持续占满 CPU、浪费功耗、与别人抢核；`wait` 让线程**真正睡眠**，平台上常借 **futex / `WaitOnAddress`** 之类“按地址等待”原语实现，几乎不烧 CPU。
- **使用范式（务必 while 而非 if）**：
  - 等待方：`while (flag.load(mo) == old) flag.wait(old, mo);`
  - 通知方：`flag.store(new, mo); flag.notify_one();`
  - 用 `while` 是因为存在伪唤醒（spurious wakeup）/“值变了又变回”——醒来后必须复查谓词。顺序铁律：通知方**先改值、再 notify**。

## 必做任务

1. `// TODO [必做 1]`：用 atomic flag + `wait/notify` 实现高效等待——waiter 在 `while (flag==0) flag.wait(0, acquire)` 里睡眠等待。
2. `// TODO [必做 2]`：signaler 先 `store(1, release)`（顺带发布 payload）再 `notify_one()` 唤醒。
3. 对比第二部分的朴素自旋版本：观察事件来临前 spinner 白白自旋的圈数，体会 CPU 浪费。

## 验收点

- wait/notify 版本中 waiter 被正确唤醒并读到 signaler 发布的 payload（release/acquire 配对建立可见性）。
- 能说清 `atomic::wait` 相对自旋的优势（不烧 CPU、省功耗、平台多用 futex 实现），以及为何要在 `while` 谓词里复查（防伪唤醒）、为何通知方要“先改值再 notify”。

## 对应官方参考

- cppreference [`std::atomic<T>::wait`](https://en.cppreference.com/w/cpp/atomic/atomic/wait) / [`notify_one`](https://en.cppreference.com/w/cpp/atomic/atomic/notify_one) / [`notify_all`](https://en.cppreference.com/w/cpp/atomic/atomic/notify_all)
- 《C++ Concurrency in Action, 2nd ed.》(Williams) 第 4 / 5 章
- 提案 P1135R6（The C++20 Synchronization Library）

## 构建运行

```bash
cmake --build build-vs2026 --target H3_atomic_wait_notify --config Release
./build-vs2026/H3_atomic_wait_notify/Release/H3_atomic_wait_notify.exe
```
