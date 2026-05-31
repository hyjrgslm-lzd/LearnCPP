# 练习 E5：Multi-Process Service 与并发客户端

## 目标

`[E5-T01]` (main.cu:1) E5_mps_and_concurrent_clients/main.cu。
`[E5-T02]` (main.cu:2) 练习目标：理解 Multi-Process Service (MPS) 的工作原理，演示单 GPU 多 process 场景，测量 kernel 延迟方差。

理解 Multi-Process Service（MPS）的基本概念、启动方式、以及在什么场景下开启它。通过观察单 GPU 多 process 并发时的 context switch overhead，体会 MPS 如何降低这种开销。

## 前置理解

- 你知道 CUDA context 是 device 上的虚拟执行环境，每个 process 默认有自己的 context。
- 你理解"context switch"会带来延迟（需要 flush GPU 管道）。
- 你有过在一个 GPU 上跑多个进程的经历，或者知道这种场景会变慢。

## 必做任务

`[E5-T03]` (main.cu:6) 用法：`--role=producer` / `--role=consumer` / 默认单进程。
`[E5-T04]` (main.cu:11) 模块 E 练习 E5 的必做任务清单。
`[E5-T13]` (main.cu:60) TODO [必做-1] 实现 `kernel_worker`：每线程对其负责的元素做 INNER_LOOP 次浮点运算。
`[E5-T14]` (main.cu:62) TODO [必做-1] kernel 函数体内实现：`acc = acc*1.0001f+0.001f` 累乘累加。
`[E5-T27]` (main.cu:140) TODO [必做-2] 持续循环让 GPU 保持忙碌（producer 后台运行，consumer 测量延迟方差）。

1. 写一个轻量的 CUDA 程序 `worker.cu`，启动一个简单 kernel（例如 1-10 ms 执行时间），输出当前 process ID 和执行时间。
2. 不用 MPS：用命令行或脚本同时启动 4 个 `worker` 进程（在同一块 GPU 上）。观察它们的完成时间；由于 context switch 开销，总耗时应该明显大于单个 worker 的时间。
3. 启动 MPS daemon：`nvidia-cuda-mps-control -d`（需要 root 或特定权限）。或者在文档中说明如何启动（通常涉及设置环境变量）。
4. 再次同时启动 4 个 `worker` 进程。观察完成时间；预期会快于无 MPS 的情况（因为多个 process 共享同一个 GPU context）。
5. 记录有无 MPS 时的对比数据（总耗时、吞吐量）。
6. 用 `nvidia-smi -i <gpu_id>` 或 Nsight Systems 观察 GPU 的利用率变化。

## 进阶任务

`[E5-T05]` (main.cu:18) 进阶任务清单（TODO [进阶]）。

- 尝试在 MPS daemon 启动后，设置 active thread percentage（`nvidia-cuda-mps-control` 的 `set_active_thread_percentage_default_nnodes` 命令），限制某些 process 的资源使用；观察吞吐变化。
- 对比多个小 kernel 的场景与单个大 kernel 的场景，观察 MPS 的收益是否随之变化。
- 如果有两块 GPU，在其中一块上启动 MPS，另一块不启用，对比两者在多 process 场景下的性能（概念理解即可）。

## 验收点

`[E5-T06]` (main.cu:23) 重要说明：MPS 是系统级配置，不是 CUDA API 调用。
`[E5-T07]` (main.cu:48) 常量定义。
`[E5-T08]` (main.cu:51) `N_ELEM = 1 << 24`，16M 元素。
`[E5-T09]` (main.cu:54) `MEASURE_ITERS = 50`，测量延迟方差的次数。
`[E5-T10]` (main.cu:55) `INNER_LOOP = 256`，控制 kernel 执行时间 ~1-10 ms。
`[E5-T11]` (main.cu:58) Kernel（TODO 区域）。
`[E5-T12]` (main.cu:61) `kernel_worker`：模拟实际工作负载（中等计算密度，1-10 ms）。
`[E5-T15]` (main.cu:71) 测量单进程 kernel 延迟（均值 + 方差）。
`[E5-T16]` (main.cu:84) 预热。
`[E5-T17]` (main.cu:91) 测量。
`[E5-T18]` (main.cu:102) 统计。
`[E5-T25]` (main.cu:127) Producer 角色：持续发起 kernel 作为背景负载。
`[E5-T29]` (main.cu:153) Consumer 角色：在背景负载下测量 kernel 延迟方差。
`[E5-T33]` (main.cu:166) `main` 入口。
`[E5-T34]` (main.cu:174) 解析 `--role` 参数。
`[E5-T39]` (main.cu:197) standalone：单进程基准，用于对比。

- 你能给出有无 MPS 时的定量对比（例如总耗时、平均 latency）。
- 4 个 process 在有 MPS 的情况下完成得更快（如果硬件和驱动支持的话）。
- 你理解了为什么 MPS 对多 process 场景有帮助。

## 观察点

- MPS 的核心是"多个 host process 共享一个 GPU context"，避免频繁的 context switch。
- 没有 MPS 时，context switch 可能导致 GPU 管道清空和重填，成本不菲。
- MPS 的资源隔离是软件级的（基于 thread block scheduling），而不是硬件级的；因此多个 process 仍然会竞争 SM。
- MPS 对 many small kernels 的收益比对 few large kernels 的收益更大（因为 context switch 开销的占比不同）。

## 常见坑

1. 启动 MPS daemon 需要 root 权限或特定用户，某些集群环境下不允许（文档应该说明）。
2. MPS daemon 启动后，忘记设置 `CUDA_MPS_PIPE_DIRECTORY` 环境变量，导致 client process 无法连接。
3. 在有 MPS 的情况下，多个 process 竞争资源时，某个 process 的 kernel 可能被其它 process 打断，导致执行顺序变化（需要理解这不是 bug）。
4. 假设 MPS 在所有场景下都快；实际上如果只有一个 process，MPS 可能略慢（因为多了一层中介）。
5. MPS 不支持 CUDA Graphs 的某些特性（这取决于 CUDA 版本和驱动版本）；混用时可能出错。
6. 关闭 MPS daemon 时没有 graceful shutdown，导致还在运行的 client process 出错。

## 提示

- `nvidia-cuda-mps-control` 通常在 CUDA Toolkit 的 `bin` 目录中。
- 设置 `CUDA_DEVICE_ORDER=PCI_BUS_ID` 和 `CUDA_VISIBLE_DEVICES=<id>` 以精确指定 GPU。
- 如果不能启动 MPS daemon（权限原因），可以在文档中详细说明启动流程和预期结果，然后用概念理解通过验收。

## MPS 启动步骤（系统级操作，非 CUDA API）

**重要说明：MPS 是管理员级别的系统配置，不是 CUDA API 调用。**
以下操作需要 root 权限或 sudo，且仅在 Linux 系统上支持（Windows 不支持 MPS）。

```bash
# 1. 确保 GPU 处于独占进程模式（Exclusive Process Mode）
sudo nvidia-smi -i 0 -c EXCLUSIVE_PROCESS

# 2. 启动 MPS daemon（后台运行）
export CUDA_VISIBLE_DEVICES=0
nvidia-cuda-mps-control -d

# 3. 验证 MPS daemon 已启动
echo "get_server_list" | nvidia-cuda-mps-control

# 4. 运行多个并发 client 进程
./E5_mps_and_concurrent_clients --role=producer &
./E5_mps_and_concurrent_clients --role=consumer
wait

# 5. 关闭 MPS daemon（graceful shutdown）
echo quit | nvidia-cuda-mps-control

# 6. 恢复 GPU 到默认模式
sudo nvidia-smi -i 0 -c DEFAULT
```

如果没有 root 权限，建议：
1. 在虚拟机或容器（Docker with --gpus）中以 root 身份测试。
2. 或直接阅读 NVIDIA MPS 文档，用概念理解通过验收。

## 复盘问题

1. MPS 中的"multi-process"指的是什么？它如何不同于 CUDA Graph replay 的并发？
2. 为什么 MPS 的资源隔离是"软件级"而不是"硬件级"？这有什么含义？
3. 在 MPS 启用的情况下，如果两个 process 的 kernel 同时启动，它们会怎样执行？
4. MPS 对 stream 并发有什么影响？

## 对应官方参考

- NVIDIA CUDA Multi-Process Service Documentation：https://docs.nvidia.com/deploy/cuda-mps/
- CUDA Best Practices：多 process 场景

## 输出对照（printf / std::puts 原文）

- `[E5-T19]` (main.cu:113) 原文：`[PID %d][%s] 延迟统计（%d 次）:` -> 现：`[PID %d][%s] latency stats (%d samples):`
- `[E5-T20]` (main.cu:115) 原文：`均值   = %.3f ms` -> 现：`mean   = %.3f ms`
- `[E5-T21]` (main.cu:116) 原文：`标准差 = %.3f ms` -> 现：`stddev = %.3f ms`
- `[E5-T22]` (main.cu:117) 原文：`最小值 = %.3f ms` -> 现：`min    = %.3f ms`
- `[E5-T23]` (main.cu:118) 原文：`最大值 = %.3f ms` -> 现：`max    = %.3f ms`
- `[E5-T24]` (main.cu:120) 原文：`变异系数 (CV) = %.1f%%` -> 现：`CV     = %.1f%%`
- `[E5-T26]` (main.cu:130) 原文：`[PID %d] Producer 启动，持续发起 kernel 作为背景负载...` -> 现：`[PID %d] Producer started, continuously issuing kernels as background load...`
- `[E5-T28]` (main.cu:144) 原文：`[Producer] 已完成 %d 轮 kernel` -> 现：`[Producer] completed %d kernel rounds`
- `[E5-T30]` (main.cu:156) 原文：`[PID %d] Consumer 启动，测量 kernel 延迟方差...` -> 现：`[PID %d] Consumer started, measuring kernel latency variance...`
- `[E5-T31]` (main.cu:159) 原文：`说明：与 Producer 进程并发运行时，若无 MPS，` -> 现：`note: when running concurrently with Producer, without MPS`
- `[E5-T32]` (main.cu:160) 原文：`context switch 会造成延迟变大且 CV 增高。` -> 现：`context switch will inflate latency and CV.`
- `[E5-T35]` (main.cu:180) 原文：`=== E5: MPS 与并发客户端 ===` -> 现：`=== E5: MPS and concurrent clients ===`
- `[E5-T36]` (main.cu:181) 原文：`PID = %d, role = %s` -> 现：保持英文不变
- `[E5-T37]` (main.cu:182) 原文：`grid = %d, block = %d, inner_loop = %d` -> 现：保持英文不变
- `[E5-T38]` (main.cu:184) 原文：`预估 kernel 执行时间 ~%.0f ms（视 GPU 而定）` -> 现：`estimated kernel time ~%.0f ms (depends on GPU)`
- `[E5-T40]` (main.cu:199) 原文：`--- 多进程测试操作说明 ---` -> 现：`--- multi-process test instructions ---`
- `[E5-T41]` (main.cu:200) 原文：`1. 无 MPS 测试：` -> 现：`1. without MPS:`
- `[E5-T42]` (main.cu:201) 原文：`终端 1: ./E5_mps_and_concurrent_clients --role=producer &` -> 现：`terminal 1: ./E5_mps_and_concurrent_clients --role=producer &`
- `[E5-T43]` (main.cu:202) 原文：`终端 2: ./E5_mps_and_concurrent_clients --role=consumer` -> 现：`terminal 2: ./E5_mps_and_concurrent_clients --role=consumer`
- `[E5-T44]` (main.cu:203) 原文：`观察 consumer 的延迟方差（应比基准高）` -> 现：`observe consumer latency variance (should be higher than baseline)`
- `[E5-T45]` (main.cu:204) 原文：`2. 启用 MPS 后重复测试：` -> 现：`2. enable MPS and re-run:`
- `[E5-T46]` (main.cu:205) 原文：`见 README.md 的 MPS 启动步骤` -> 现：`see MPS startup steps in README.md`
- `[E5-T47]` (main.cu:207) 原文：`预期：有 MPS 时，consumer 延迟方差应低于无 MPS 时` -> 现：`expected: with MPS, consumer variance should drop below the without-MPS case`
- `[E5-T48]` (main.cu:212) 原文：`提示：用 'nsys profile ...' 采集` -> 现：`hint: capture with 'nsys profile ...'`
- `[E5-T49]` (main.cu:214) 原文：`观察 GPU 利用率和 context switch 行为` -> 现：`observe GPU utilization and context switch behavior`
