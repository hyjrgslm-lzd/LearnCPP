# 练习 G-1：Treiber 无锁栈

> 详尽版见 `../../09-模块G-无锁数据结构.md` 的 练习 G-1。

## 目标

用单个原子指针 `head` + CAS 循环（compare-and-swap loop）实现一个无锁（lock-free）后进先出（LIFO）栈：`push`（new node，CAS 接到 head）与 `pop`（CAS 摘 head）。多线程压测验证不丢不重，并讲清每一步的内存序（memory order）。

## 前置理解

- CAS 循环范式：读旧值 → 基于旧值算新值 → `compare_exchange_weak` 提交；失败说明别人抢先改了 `head`，旧值被自动刷新，重试即可。`compare_exchange_weak` 的第一个参数是**引用**，失败时被写回最新值。
- 内存序：`push` 成功用 `release`（发布新节点内容），`pop` 成功用 `acquire`（与 push 的 release 配对，看到完整节点）；失败分支取回最新 `head` 即可。
- lock-free 进展保证：没有任何线程持锁，CAS 失败的线程重试，但总有线程能前进（区别于 wait-free 的“每线程有界步数”）。

## 必做任务

1. `// TODO [必做 1]`：实现 `push` 与 `pop` 的 CAS 循环（用 `compare_exchange_weak` 操作 `head`），并按注释标注每步内存序。
2. `// TODO [必做 1/必做 2]`：多线程压测（N 线程并发 push，再并发 pop），统计弹出总数 == 压入总数。
3. **注明 pop 的节点释放**：本题刻意**泄漏**未 `delete`——无锁下直接释放会 use-after-free，安全回收（hazard pointer / RCU）留模块 I。

## 验收点

- 多线程压测下弹出总数严格等于压入总数（不丢不重）。
- 能说清 push 用 release、pop 用 acquire 的理由，以及失败分支为何可用 relaxed。
- 能解释为什么本题必须泄漏节点，以及它和模块 I 的关系。

## 对应官方参考

- cppreference [`std::atomic`](https://en.cppreference.com/w/cpp/atomic/atomic) / [`compare_exchange`](https://en.cppreference.com/w/cpp/atomic/atomic/compare_exchange)
- 《C++ Concurrency in Action, 2nd ed.》(Williams) 第 7 章 7.2.1
- Fedor Pikus, "Lock-free Programming" (CppCon)

## 构建运行

```bash
cmake --build build-vs2026 --target G1_treiber_stack --config Release
./build-vs2026/G1_treiber_stack/Release/G1_treiber_stack.exe
```
