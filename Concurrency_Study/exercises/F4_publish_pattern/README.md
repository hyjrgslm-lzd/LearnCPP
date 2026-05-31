# 练习 F-4：发布-订阅内存序模式

> 详尽版见 `../../08-模块F-内存模型与memory_order.md` 的 练习 F-4。

## 目标

把 F-1 的“一个 bool 标志发布一个 payload”推广为通用模式：单生产者反复发布**新版本**的一整块结构体/缓冲，用一个原子**版本号（version / sequence number）**作发布点；消费者 acquire 读版本号、读到新版本后再读数据。这正是无锁（lock-free）数据结构的发布骨架。

## 前置理解

- 发布的数据 `g_data` 是**非原子**结构体；它的可见性完全由版本号 `g_version` 的 release/acquire 配对担保。
- 生产者不变式：先写好新一版 `g_data`（这些写 sequenced-before 后面的 release 写），再 `g_version.store(v, release)` 发布。
- 消费者不变式：`g_version.load(acquire)` 读到版本 V ⇒ 与生产者写 V 的那次 release 写 synchronizes-with ⇒ 生产者在那之前对 `g_data` 的全部写 happens-before 我随后对 `g_data` 的读 ⇒ 我读到的必是与 V 配套的完整数据。
- 单写者下，读者可能跳过中间版本直接看到最新版——这正常；要的是“看到的那一版数据自洽完整”，而非每版都见。
- 无锁结构的共同骨架：先在“别人看不到”时把新状态准备好，再用一次 release 原子写（版本号/指针/标志）一举发布；读者 acquire 读到发布点即 happens-before 全部新数据。

## 必做任务

1. `// TODO [必做 1]`：生产者每轮写好新版 `g_data` 后用 `g_version.store(v, release)` 发布；多个消费者 `g_version.load(acquire)` 追版本，读到新版本后读 `g_data` 并校验自洽（`value == id*1000+id`、`label == "snapshot-v"+id`）。

## 验收点

- 能用一个原子版本号 release/acquire 发布整片非原子数据，读者读到的版本与数据自洽。
- 能复述消费者那条 happens-before 推理链。
- 能说出这就是无锁/单写多读结构的发布骨架，并指出“发布指针 + RCU/引用计数”是其自然延伸。

## 对应官方参考

- cppreference [`std::memory_order`](https://en.cppreference.com/w/cpp/atomic/memory_order)（release-acquire；release sequence）
- 《C++ Concurrency in Action, 2nd ed.》(Williams) 第 5 章 5.3 / 第 7 章
- Mara Bos《Rust Atomics and Locks》第 3 章（release/acquire）

## 构建运行

```bash
cmake --build build-vs2026 --target F4_publish_pattern --config Release
./build-vs2026/F4_publish_pattern/Release/F4_publish_pattern.exe
```
