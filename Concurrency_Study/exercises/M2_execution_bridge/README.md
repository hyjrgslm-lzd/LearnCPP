# 练习 M-2：std::execution 桥接（sender/receiver 取代裸线程）

> 详尽版见 `../../16-模块M-工作窃取与结构化并发桥接.md` 的 练习 M-2。

## 目标

用 **std::execution（P2300，C++26）** 的 sender/receiver 演示如何**取代裸 thread + future**：用 `schedule(scheduler) | then(...) | then(...)` 组一条异步管线、`sync_wait` 取结果，用 `when_all` 并发汇合两个 sender。讲清相对手写线程的好处（结构化、可组合、错误/取消通道）。本模块对 senders **只作桥接演示**，深入见 `Execution_Study\`。

## 前置理解（核心抽象）

- **scheduler**：工作**在哪执行**的抽象（线程池/GPU/单线程）。本题用 `exec::static_thread_pool` 提供。
- **sender**：一段**将要**产出「值/错误/停止」的异步工作**描述**——惰性、可组合（不是立刻跑的线程）。
- **receiver**：sender 完成时的回调接收端，有**三条通道**：value / error / stopped。
- **算法**：`schedule(sched)`（在 sched 上起头）、`just(v)`（已就绪值）、`then(f)`（接上游值做变换，像 `future.then`）、`when_all(a,b)`（并发汇合）、`sync_wait(s)`（在当前线程阻塞取结果，返回 `optional<tuple<...>>`，是「异步世界」与「同步 main」的桥）。

## 工具链说明（务必先读）

- std::execution 是 **C++26 标准**（头 `<execution>`，命名空间 `std::execution`），但截至 2026-05 **MSVC 未实现**。
- 本题用 NVIDIA 参考实现 **stdexec** 回退：命名空间 **`stdexec`**，核心头 `<stdexec/execution.hpp>`，线程池在 `<exec/static_thread_pool.hpp>`。
- **MSVC 必须加 `/Zc:preprocessor`**（stdexec 头文件强制要求符合标准的预处理器）——本题 `CMakeLists.txt` 已加。

## 必做任务

1. `// TODO [必做 1]`：`schedule(sched) | then | then` 组管线，`sync_wait` 取值（期望 42）。
2. `// TODO [必做 2]`：`when_all` 并发两个 sender，下游 `then(va, vb)` 汇合（期望 123）。
3. `// TODO [进阶 1]`：`just(v)` 起头链 `then`；演示 `then` 内抛异常如何走 **error 通道**被 `sync_wait` 重新抛出。

## 验收点

- 三段管线结果与期望一致（42 / 123 / 15），异常经 `sync_wait` 重抛被正常 `try/catch` 接住。
- 你能说清 scheduler / sender / receiver 三者关系，及相对裸 thread+future 的四点好处：**结构化、可组合、三通道（value/error/stopped）、调度可换**。
- 你知道 MSVC 上要 stdexec 回退 + `/Zc:preprocessor`，并知道深入内容在 `Execution_Study\`。

## 对应官方参考

- [P2300R10 std::execution](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2300r10.html)
- [NVIDIA/stdexec](https://github.com/NVIDIA/stdexec)
- cppreference [std::execution](https://en.cppreference.com/w/cpp/execution)

## 构建运行（VS2026, C++20；需 stdexec，CMake 已用 StdexecSetup 拉取）

```bash
cmake --build build-vs2026 --target M2_execution_bridge --config Release
./build-vs2026/M2_execution_bridge/Release/M2_execution_bridge.exe
```
