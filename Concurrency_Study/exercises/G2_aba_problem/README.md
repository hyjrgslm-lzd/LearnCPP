# 练习 G-2：ABA 问题

> 详尽版见 `../../09-模块G-无锁数据结构.md` 的 练习 G-2。

## 目标

亲手构造 ABA 场景并演示它如何击穿朴素指针 CAS，然后用带版本号的标签指针（tagged pointer / version counter）破解它。

## 前置理解

- **ABA**：线程读到 `head == A` 后挂起；其间别的线程 pop A、pop B、又把 A（被释放后复用的同址节点）push 回来，`head` 再次 == A，但 `A->next` 已变。原线程恢复后 `compare_exchange` 发现“还是 A”便误判“没人动过”而成功提交，把基于陈旧假设算出的新值写入 → 链表损坏 / 丢节点。
- 根因：CAS 只比较**值（指针）**，不比较**值的历史**。
- 解法一（本题演示）：把 `(指针, version)` 打包成一个原子整体做 CAS，每次成功改动 `version += 1`；“变回 A”也骗不过版本号，旧 CAS 必然失败、强制重试。
- 解法二：hazard pointer / RCU（从“别让被复用的同址节点出现”角度，管住何时能安全回收），详见模块 I。

## 必做任务

1. `// TODO`（第一部分）：构造 `C->B->A` 栈，用两个原子信号严格编排线程交错，**确定性复现** ABA——受害者朴素 CAS 误判成功，head 被改成已弹出的 B。
2. `// TODO`（第二部分）：用 64 位整数打包 `(下标, 版本号)`，演示同样的交错下带版本号的 CAS 失败（正确），并说明 hazard pointer / RCU 是另一类解法。

## 验收点

- 能复现并解释 ABA 为何让朴素 CAS“错误地成功”。
- 能说清版本号为何能根除 ABA：CAS 比较的是 `(指针, 版本)` 整体。
- 能区分“标签指针”与“安全回收（hazard pointer/RCU）”两类思路的着力点。

## 对应官方参考

- cppreference [`compare_exchange`](https://en.cppreference.com/w/cpp/atomic/atomic/compare_exchange)
- 《C++ Concurrency in Action, 2nd ed.》(Williams) 第 7 章 7.2.2
- Fedor Pikus, "Lock-free Programming" — ABA 一节

## 构建运行

```bash
cmake --build build-vs2026 --target G2_aba_problem --config Release
./build-vs2026/G2_aba_problem/Release/G2_aba_problem.exe
```
