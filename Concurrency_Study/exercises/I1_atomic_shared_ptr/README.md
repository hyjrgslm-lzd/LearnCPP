# 练习 I-1：atomic&lt;shared_ptr&gt;

> 详尽版见 `../../11-模块I-安全内存回收.md` 的 练习 I-1。

## 目标

用 C++20 的 `std::atomic<std::shared_ptr<T>>`（`<memory>`）实现一个可并发安全读写的共享配置单元，把无锁结构的回收难题（reclamation problem）交给**引用计数（reference counting）**解决：读者 `load` 出一份 `shared_ptr` 副本，旧对象就被这份副本撑着，写者哪怕 `store` 了新值也不会让旧对象被提前释放，从而根除 use-after-free。最后测吞吐并讨论代价。

## 前置理解

- **回收难题**：无锁结构里，线程 A 读到指向节点 N 的指针、尚未解引用时，线程 B 把 N 摘下并 `delete` → A 解引用悬垂指针（use-after-free）。这正是模块 G Treiber 栈「pop 故意泄漏」的病根。
- **引用计数解法**：`load` 返回的 `shared_ptr` 副本让计数 +1，旧对象的释放被推迟到“最后一个持有它的 `shared_ptr`（包括还没读完的读者）析构”那一刻。
- **代价**：`std::atomic<std::shared_ptr<T>>` 在**多数标准库实现里并非 lock-free**（`is_lock_free()` 通常为 `false`，内部往往加锁来原子地操作“控制块指针 + 引用计数”）。安全好写，但有同步开销，热路径吞吐不如 hazard pointer / RCU。

## 必做任务

1. `// TODO [必做 1]`：用 `atomic<shared_ptr>::load`（acquire）实现 `read()`、`store`（release）实现 `write()`。
2. 多线程并发读写压测：读者反复 `read()` 出副本并解引用、校验不变量 `payload == version*2`；写者反复用新版本替换。验收：**全程不崩、无 use-after-free**，一致读取次数 == 总读取次数。
3. 打印 `is_lock_free()`，确认多数实现为 `false`，并能解释这一代价的来源。

## 验收点

- 并发读写不崩、不撕裂（一致读取次数严格等于总读取次数）。
- 能说清引用计数如何撑住读者手里的旧对象、根除 use-after-free。
- 能解释为什么 `atomic<shared_ptr>` 多数实现非 lock-free，以及它相对 hazard pointer / RCU 的取舍。

## 对应官方参考

- cppreference [`std::atomic<std::shared_ptr>`](https://en.cppreference.com/w/cpp/memory/shared_ptr/atomic2)
- cppreference [`std::shared_ptr`](https://en.cppreference.com/w/cpp/memory/shared_ptr)
- 《C++ Concurrency in Action, 2nd ed.》(Williams) 第 7 章

## 构建运行

```bash
cmake --build build-vs2026 --target I1_atomic_shared_ptr --config Release
./build-vs2026/I1_atomic_shared_ptr/Release/I1_atomic_shared_ptr.exe
```
