# 练习 F-3：seq_cst 与内存栅栏

> 详尽版见 `../../08-模块F-内存模型与memory_order.md` 的 练习 F-3。

## 目标

复现经典的 **store-load 重排**现象（Dekker / store-buffer 模式）：两个线程各自“先 store 自己、再 load 对方”，在 relaxed 下**可能都读到对方的旧值（0）**。然后用 `memory_order_seq_cst` 修复，并用 `std::atomic_thread_fence(memory_order_seq_cst)` 替代“每操作 seq_cst”达到同样效果。

## 前置理解

- **store-load（StoreLoad）重排**：同一线程内“写 x 再读 y”可能被观测成“先读 y（旧值）再写 x”。x86（TSO）唯一允许的硬件重排就是它，所以**本现象在 x86 上能真实复现**。
- **release/acquire 修不了它**：release/acquire 只管 message-passing 方向的可见性（一个变量的写被另一线程读到），不禁止 StoreLoad 重排。
- **seq_cst 的额外保证**：在 acquire/release 之上，所有 `seq_cst` 操作还服从**单一全序（single total order）**。这条全序保证“至少一个线程的 store 排在另一线程的 load 之前”，于是“两边都读到 0”被禁止。
- **`atomic_thread_fence(seq_cst)`**：一道参与全局 seq_cst 全序的全栅栏。在“写自己之后、读对方之前”插一道，可把 relaxed store/load 在全序上隔开，等效修复，且把屏障集中到关键点。

## 必做任务

1. `// TODO [必做 1]`：实现 store-load 模式；relaxed 版统计 `both==0`（应 >0，复现重排），seq_cst 版统计 `both==0`（应恒为 0，修复）。

## 进阶任务

- `// TODO [进阶 1]`：store/load 改回 relaxed，在两者之间插 `std::atomic_thread_fence(memory_order_seq_cst)`，验证 `both==0` 同样恒为 0。

## 验收点

- 能复现 store-load 重排（relaxed 版出现 both==0），并解释它是哪种重排。
- 能用 seq_cst 修复，并讲清“单一全序”为何禁止 both==0。
- 能说清 release/acquire 为何修不了 store-load 重排。
- 能用 seq_cst fence 替代每操作 seq_cst，理解“屏障集中到关键点”的取舍。

## 对应官方参考

- cppreference [`std::memory_order`](https://en.cppreference.com/w/cpp/atomic/memory_order)（seq_cst ordering）
- cppreference [`std::atomic_thread_fence`](https://en.cppreference.com/w/cpp/atomic/atomic_thread_fence)
- 《C++ Concurrency in Action, 2nd ed.》(Williams) 第 5 章 5.3.3
- Herb Sutter, "atomic<> Weapons"（SC 全序与 StoreLoad 重排）

## 构建运行

```bash
cmake --build build-vs2026 --target F3_seqcst_fence --config Release
./build-vs2026/F3_seqcst_fence/Release/F3_seqcst_fence.exe
```
