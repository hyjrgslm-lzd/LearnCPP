# 练习 J-1：伪共享实测

> 详尽版见 `../../13-模块J-缓存与伪共享.md` 的 练习 J-1。

## 目标

亲手测出伪共享（false sharing）的性能代价：两个线程各写各自的计数器，逻辑上互不相干，但若两个计数器落在同一条缓存行（cache line，典型 64 字节），每次写都会让对方核心持有的该行失效，缓存行在核心间反复弹跳（cache line bouncing），吞吐暴跌。再用 `alignas(std::hardware_destructive_interference_size)`（或 64）把两个计数器分到不同缓存行，重测并对比加速比（speedup）。

## 前置理解

- **缓存行**是缓存一致性协议（cache coherence，如 MESI）的最小搬运单位：CPU 永远以整条缓存行为粒度在核心间传输，而非单变量。
- **伪共享**：不同线程频繁写各自的变量，但这些变量恰好同处一条缓存行，硬件被迫把整条行在核心间来回搬，造成本不该有的竞争。
- **对策**：用 `alignas(缓存行大小)` 给每个被不同线程写的对象做对齐 + 填充（padding），把它们各自推到独立缓存行。

## 必做任务

1. `// TODO [必做 1]`：构造相邻计数器的 `CountersShared`（同一缓存行），开两个线程各锤一个计数器，计时记 `ms_shared`。
2. `// TODO [必做 2]`：构造 `alignas` 分行的 `CountersPadded`，同样两线程各锤一个，计时记 `ms_padded`，打印两者耗时与比值。

## 验收点

- 伪共享版明显慢于对齐版，`speedup = ms_shared / ms_padded` 常见 2~8 倍（随机器而异；本机实测约 7.9 倍）。
- 你能解释为什么“逻辑无冲突”的两个写却互相拖慢：缓存行弹跳。
- 你能说清 `alignas` 如何通过对齐 + 填充消除伪共享，并从 `sizeof` 看出布局差异。

## 对应官方参考

- cppreference [`hardware_destructive_interference_size`](https://en.cppreference.com/w/cpp/thread/hardware_destructive_interference_size)
- 《C++ Concurrency in Action, 2nd ed.》(Williams) 第 8 章（伪共享 / 数据布局）
- Ulrich Drepper, *What Every Programmer Should Know About Memory*

## 构建运行

```bash
cmake --build build-vs2026 --target J1_false_sharing --config Release
./build-vs2026/J1_false_sharing/Release/J1_false_sharing.exe
```

> 请务必用 **Release** 构建：Debug 下原子操作开销会掩盖伪共享差距。
