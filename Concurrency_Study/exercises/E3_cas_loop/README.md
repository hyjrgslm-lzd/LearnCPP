# 练习 E-3：CAS 循环

> 详尽版见 `../../07-模块E-原子操作基础.md` 的 练习 E-3。

## 目标

掌握“比较并交换”（compare-and-swap，CAS）这一无锁编程基石：`compare_exchange_weak/strong(expected, desired)` 原子地“若当前值等于 `expected` 则改成 `desired`”，并把当前值回写进 `expected`。学会 CAS 循环（CAS loop）范式，用它实现标准库没有内置的任意读-改-写——本题用 CAS 循环实现 atomic `fetch_max`（原子取最大值）。说清 `weak` 与 `strong` 的取舍。

## 前置理解

- `compare_exchange_*(expected, desired)`：原子地比较“当前值 == `expected`？”
  - 是 → 写入 `desired`，返回 `true`；
  - 否 → 不写，把 `expected` **刷新为真实当前值**，返回 `false`。
- **CAS 循环范式**：`load` 当前值作 `expected` → 算出 `desired` → `compare_exchange_weak`；失败时 `expected` 已被刷新，拿着最新值重算重试。这是模块 G 无锁数据结构反复出现的核心套路。
- **weak vs strong**：
  - `weak` 允许**伪失败**（spurious failure，即便值确实等于 `expected` 也可能返回 `false`），但放进循环无所谓（多转一圈），且在 LL/SC 架构上更高效——**循环里优先用 weak**。
  - `strong` 不会伪失败，适合**不便重试的单次调用**。
- 默认内存序仍是 `seq_cst`；“成功/失败用不同内存序”的细控制留模块 F。

## 必做任务

1. `// TODO [必做 1]`：实现 `atomic_fetch_max(atomic<int>&, int)`——CAS 循环把目标更新为 `max(目标, 候选)`，`candidate <= cur` 时直接返回。8 线程竞争抬高，验证最终等于全局最大候选。
2. `// TODO [必做 2]`：同构地用 CAS 循环实现 `atomic_fetch_multiply`，巩固“load→算→试，失败重来”的骨架。

## 进阶任务

- `// TODO [进阶 1]`：用 `compare_exchange_strong` 演示成功与失败两条路径，重点观察“失败时 `expected` 被刷新为真实当前值”这一行为。

## 验收点

- 能讲清 CAS 的语义：比较 `expected`、相等则写 `desired`、不等则刷新 `expected`。
- 能写出正确的 CAS 循环并用它实现 `fetch_max` / 任意 RMW。
- 能说出 weak 与 strong 的差异与各自适用场景（循环用 weak、单次用 strong）。

## 对应官方参考

- cppreference [`std::atomic::compare_exchange`](https://en.cppreference.com/w/cpp/atomic/atomic/compare_exchange)
- cppreference [`std::atomic::fetch_add`](https://en.cppreference.com/w/cpp/atomic/atomic/fetch_add)
- 《C++ Concurrency in Action, 2nd ed.》(Williams) 第 5 章 5.2.4

## 构建运行

```bash
cmake --build build-vs2026 --target E3_cas_loop --config Release
./build-vs2026/E3_cas_loop/Release/E3_cas_loop.exe
```
