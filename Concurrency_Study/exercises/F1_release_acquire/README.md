# 练习 F-1：release-acquire 同步

> 详尽版见 `../../08-模块F-内存模型与memory_order.md` 的 练习 F-1。

## 目标

用 `std::atomic<bool>` 的 **release 写 / acquire 读** 配对，安全发布一块**非原子（non-atomic）** payload：生产方先写好数据、再 release 写标志；消费方 acquire 读到标志后再读数据。你要能画出这条 happens-before 链，并解释为什么把 acquire 换成 relaxed 会破坏可见性、构成数据竞争（data race）→ 未定义行为（UB）。

## 前置理解

- **sequenced-before（先序于）**：单线程内按求值顺序确定的先后。
- **synchronizes-with（同步于）**：当线程 B 的 acquire 读**读到了**线程 A 的 release 写所写的值，A 的那次 release 写 synchronizes-with B 的那次 acquire 读。
- **happens-before（先行于）**：sequenced-before 与 synchronizes-with 的传递闭包。一旦 A happens-before B，A 之前的写对 B 之后的读可见。
- 因此：A release 写之前（sequenced-before）的所有写 → happens-before → B acquire 读之后的所有读。这就是“用一个原子标志发布一整片非原子数据”的原理。
- relaxed 读**不**与 release 写建立 synchronizes-with，故**不**产生 happens-before；此时读非原子 payload 与生产方的写无先后关系 = data race = UB。

## 必做任务

1. `// TODO [必做 1]`：生产方写好 `g_payload` 各字段后，用 `g_ready.store(true, memory_order_release)` 发布；消费方用 `while(!g_ready.load(memory_order_acquire))` 自旋，读到后再读 payload。画出 happens-before 边。

## 进阶任务

- `// TODO [进阶 1]`：把消费方的 acquire 改成 `memory_order_relaxed`，讲清为何理论上可能读到未初始化/陈旧 payload（x86 TSO 上常“碰巧正确”，但 ARM/POWER 或编译器重排下会暴露——代码本身是 UB）。

## 验收点

- 能用 release/acquire 配对安全发布非原子数据，并画出完整 happens-before 链。
- 能解释 synchronizes-with 是“acquire 读到了 release 写的值”才建立。
- 能说清把 acquire 换成 relaxed 为何破坏可见性、为何是 data race / UB。
- 理解“x86 上看不出错 ≠ 代码正确”，应按标准 happens-before 推理。

## 对应官方参考

- cppreference [`std::memory_order`](https://en.cppreference.com/w/cpp/atomic/memory_order)（release-acquire ordering）
- 《C++ Concurrency in Action, 2nd ed.》(Williams) 第 5 章 5.3
- Herb Sutter, "atomic<> Weapons"；Mara Bos《Rust Atomics and Locks》第 3 章

## 构建运行

```bash
cmake --build build-vs2026 --target F1_release_acquire --config Release
./build-vs2026/F1_release_acquire/Release/F1_release_acquire.exe
```
