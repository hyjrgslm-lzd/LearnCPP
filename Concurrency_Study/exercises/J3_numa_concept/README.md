# 练习 J-3：NUMA 概念与线程亲和性

> 详尽版见 `../../13-模块J-缓存与伪共享.md` 的 练习 J-3。

## 目标

以概念 + 小实验为主（不强求真 NUMA 硬件）：说清 NUMA（Non-Uniform Memory Access，非一致内存访问）的访问代价与 first-touch（首次接触）策略，并用 Windows 的 `SetThreadAffinityMask`（`#ifdef _WIN32` 包裹）演示把线程绑定到指定核（线程亲和性，thread affinity）。

## 前置理解

- **NUMA**：内存按节点（node）划分，每节点贴着一组 CPU 核。核心访问**本地节点**内存快，访问**远端节点**要走处理器互联，延迟更高、带宽更低 —— 访问代价随“数据在哪个节点”而**不一致**。
- **first-touch**：物理页在被某线程**首次写入**时，被分配到**该线程当时所在 CPU 的本地节点**。推论：让“将来读写某数据的线程”去**亲手初始化**那块数据，数据才落在它的本地节点。
- **线程亲和性**：把线程**绑定**到指定核，减少跨核迁移导致的缓存损失，也是“让线程稳定待在某 NUMA 节点、配合 first-touch 拿本地内存”的前提。Windows 用 `SetThreadAffinityMask`（亲和性掩码每一位对应一个逻辑核）；Linux 用 `pthread_setaffinity_np`；macOS 仅有建议性策略。

## 必做任务

1. `// TODO [必做 1]`：开 N 个线程，各调 `pin_current_thread_to_cpu(i)` 绑到第 i 号核，跑一段热负载并由各线程**亲手初始化**本地数据（first-touch 微缩演示），打印绑核成功与否。非 Windows 平台会打印“跳过”。

## 验收点

- 在 Windows 上能成功把多个线程分别绑到不同核（本机实测 4/4 成功）。
- 能讲清 NUMA 的本地/远端访问代价，以及 first-touch“谁先写、分给谁的本地节点”。
- 能说清亲和性为何是 NUMA 局部性优化的前提；理解单 NUMA 节点机器上 first-touch 无可测差异（无远端节点）。

## 对应官方参考

- Windows [`SetThreadAffinityMask`](https://learn.microsoft.com/windows/win32/api/processthreadsapi/nf-processthreadsapi-setthreadaffinitymask)
- Windows [NUMA Support](https://learn.microsoft.com/windows/win32/procthread/numa-support)
- Ulrich Drepper, *What Every Programmer Should Know About Memory*（NUMA / first-touch）
- 《C++ Concurrency in Action, 2nd ed.》(Williams) 第 8 章（数据局部性）

## 构建运行

```bash
cmake --build build-vs2026 --target J3_numa_concept --config Release
./build-vs2026/J3_numa_concept/Release/J3_numa_concept.exe
```

> 运行时可打开任务管理器/资源监视器，观察各线程是否分别压在不同核上。
