# 练习 J-2：硬件干扰尺寸常量

> 详尽版见 `../../13-模块J-缓存与伪共享.md` 的 练习 J-2。

## 目标

认识 C++17 在 `<new>` 引入的一对实现定义（implementation-defined）常量 `std::hardware_destructive_interference_size`（破坏性干扰尺寸）与 `std::hardware_constructive_interference_size`（建设性干扰尺寸），打印它们的值，用 destructive 常量做对齐结构体并验证 `alignof`，并讲清两者语义上的对立。

## 前置理解

- **destructive（破坏性）**：被**不同线程**频繁访问的对象，至少应相距这么多字节，才能落在不同缓存行、互不干扰 —— 用它做对齐/填充来**隔开**，避免伪共享（false sharing）。
- **constructive（建设性）**：一起被**同一线程**访问的对象，其合并大小不应超过这么多字节，才能同处一条缓存行、一次载入命中 —— 用它把相关数据**聚拢**，促进局部性/真共享。
- 标准只保证二者 `>= alignof(std::max_align_t)`；常见实现里二者通常都等于 64（一条缓存行），但具体为实现定义。
- **告警提示**：GCC 对“跨 ABI 边界使用该常量”发 `-Winterference-size`（值依赖编译目标、可能影响布局兼容）；**MSVC（含 VS2026）无此告警**。本练习只在单可执行内使用，安全。

## 必做任务

1. `// TODO [必做 1]`：打印 destructive / constructive 两个常量及 `alignof(max_align_t)`，并说清各自用途。
2. `// TODO [必做 2]`：定义两个对齐策略结构体（`DestructiveLayout` 隔开、`ConstructiveLayout` 聚拢），打印并 `static_assert` 验证 `alignof`，对照紧凑打包版的尺寸差异。

## 验收点

- 能打印出两个常量的实际值（本机均为 64）。
- `DestructiveLayout` 的 `alignof == destructive`，两个计数器各占独立缓存行；`ConstructiveLayout` 的热字段簇聚拢在一条缓存行内。
- 能用一句话区分二者：destructive=「离我远点」（隔开防伪共享），constructive=「凑过来」（聚拢提局部性）。

## 对应官方参考

- cppreference [`hardware_destructive_interference_size`](https://en.cppreference.com/w/cpp/thread/hardware_destructive_interference_size)
- 提案 P0154R1（Hardware interference size）
- 《C++ Concurrency in Action, 2nd ed.》(Williams) 第 8 章

## 构建运行

```bash
cmake --build build-vs2026 --target J2_interference_size --config Release
./build-vs2026/J2_interference_size/Release/J2_interference_size.exe
```
