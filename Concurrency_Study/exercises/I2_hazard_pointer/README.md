# 练习 I-2：hazard pointer 安全回收

> 详尽版见 `../../11-模块I-安全内存回收.md` 的 练习 I-2。

## 目标

给模块 G 的 Treiber 无锁栈（lock-free stack）的 `pop` 加上**安全的节点回收**，用风险指针（hazard pointer）彻底消除“pop 后 `delete` 节点”导致的 use-after-free——做到既无锁、又能真正释放内存（不再像 G-1 那样故意泄漏）。多线程压测无崩溃、收支平衡。

本题使用本仓库自带的**教学版**风险指针 `cs::hazard_pointer`（`concurrency_study/hazard_pointer.hpp`）。

## 前置理解

- **保护-回收协议（protect / retire / reclaim）**：
  - 读者（`pop`）在解引用 `head` 前，先用 `hazard_pointer::protect(head_)` **登记保护**并重读校验（登记期间没被换走才成立）；被保护的指针在本 `hazard_pointer` 释放前不会被回收。
  - 安全访问（读 `next`、CAS 摘下）后，旧节点不直接 `delete`，而是 `retire()` 交给回收系统。
  - 回收系统只 `delete` 那些**没有任何 hazard 槽正在保护**的退休节点。
- **与 G-1 对比**：G-1 的 `pop` 故意泄漏以回避 use-after-free；这里用 hazard pointer 让“安全回收”与“无锁”同时成立。
- **教学版说明**：C++26 在 `std` 命名空间 `<hazard_pointer>` 标准化了等价设施（提案 `P2530R3`），但 MSVC VS2026 尚未实现，故用 `cs::` 教学版。`cs::` ↔ `std::` API 差异映射见模块文档对照表。

## 必做任务

1. `// TODO [必做 1]`：在 `pop` 中 `make_hazard_pointer()` 申请句柄，`protect(head_)` 保护读出的 `head`；安全访问（读 `next`/`data`、CAS 摘下）后调用旧节点的 `retire()` 延迟回收。
2. 多线程 push/pop 混合压测：验证**全程不崩**（无 use-after-free）、**收支平衡**（总 push == 总 pop，不丢不重）。

## 验收点

- 多线程压测无崩溃、收支平衡。
- 能逐步说出 `protect head -> 安全读 next/data -> CAS 摘下 -> retire 旧节点` 的协议。
- 能解释 `retire` 为何不立即 `delete`（可能仍有读者保护着它），以及它相对 G-1“泄漏”的根本改进。

## 对应官方参考

- 提案 [`P2530R3` Hazard Pointers for C++26](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2530r3.pdf)（早期：`P0566`）
- cppreference（C++26）[`<hazard_pointer>`](https://en.cppreference.com/w/cpp/header/hazard_pointer)
- [folly Hazptr](https://github.com/facebook/folly/blob/main/folly/synchronization/Hazptr.h)（工业级对照）
- 《C++ Concurrency in Action, 2nd ed.》(Williams) 第 7 章 7.2.2

## 构建运行

```bash
cmake --build build-vs2026 --target I2_hazard_pointer --config Release
./build-vs2026/I2_hazard_pointer/Release/I2_hazard_pointer.exe
```
