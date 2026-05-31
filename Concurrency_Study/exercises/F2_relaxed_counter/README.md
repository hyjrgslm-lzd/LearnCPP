# 练习 F-2：relaxed 计数器

> 详尽版见 `../../08-模块F-内存模型与memory_order.md` 的 练习 F-2。

## 目标

论证为什么纯计数器用 `memory_order_relaxed` 的 `fetch_add` 是**正确**的——计数只依赖**原子性（atomicity）**与**单变量的修改顺序（modification order）**，不需要任何跨变量可见性顺序。再反向理解：用 relaxed 做“标志位发布数据”为何**错误**——relaxed 不建立 synchronizes-with / happens-before，两个独立变量的写在别的线程看来可被重排观测到。

## 前置理解

- **修改顺序（modification order）**：每个原子变量都有一条全线程一致的写序列；任何内存序下，所有对该变量的 RMW（如 `fetch_add`）都串在这条序列上，不会丢更新。
- relaxed 的能力边界：**只**保证“这一个原子变量自己”的原子性与修改顺序；**不**保证不同变量之间的先后可见性。
- 计数器不关心“别的变量是否已可见”，因此 relaxed 足够，且最省（无多余内存屏障）。
- “发布数据”需要跨变量的可见性顺序（flag 可见 ⇒ data 可见），那是 release/acquire 的活，relaxed 做不到。

## 必做任务

1. `// TODO [必做 1]`：多线程对 `std::atomic<long long>` 用 `fetch_add(1, memory_order_relaxed)` 自增，验证最终求和精确等于 `线程数 × 每线程次数`。

## 进阶任务

- `// TODO [进阶 1]`：构造 message-passing 反例——生产线程 relaxed 写 `data` 再 relaxed 写 `flag`，消费线程读到 `flag==1` 后读 `data`。统计“看到 flag=1 却读到 data=0”的次数；并用 release/acquire 版本作对照（理论恒为 0）。x86 上 relaxed 版可能统计为 0（TSO 难复现 store-store 重排），但这不代表 relaxed 用作标志位是对的。

## 验收点

- 能说清 relaxed 计数为何正确：原子性 + 修改顺序，无需跨变量顺序。
- 能说清 relaxed 用作标志位为何错误：不建立 happens-before。
- 能用一句话给出 relaxed 的适用边界：纯计数/统计可以，发布数据不行。

## 对应官方参考

- cppreference [`std::memory_order`](https://en.cppreference.com/w/cpp/atomic/memory_order)（relaxed ordering / modification order）
- 《C++ Concurrency in Action, 2nd ed.》(Williams) 第 5 章 5.3.3
- Mara Bos《Rust Atomics and Locks》第 3 章（Relaxed）

## 构建运行

```bash
cmake --build build-vs2026 --target F2_relaxed_counter --config Release
./build-vs2026/F2_relaxed_counter/Release/F2_relaxed_counter.exe
```
