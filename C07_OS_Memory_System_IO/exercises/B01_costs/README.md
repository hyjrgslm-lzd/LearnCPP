# B01 costs：有证据的采样，不先写结论

本题不是排行工具。`B01_costs_benchmark` 只产出单次进程的 JSON 事实；`tools/sample_benchmarks.py` 负责用相同协议采样、保存原始证据、拒绝把失败当性能数据。

## Benchmark CLI

父级 C++ benchmark 提供这些入口：

```powershell
B01_costs_benchmark --check
B01_costs_benchmark --check-completion
B01_costs_benchmark alloc VARIANT ITEMS
B01_costs_benchmark io VARIANT BYTES ROUNDS DEPTH
```

`alloc` 的 `VARIANT` 是 `heap`、`arena`、`pool`、`std-monotonic`、`std-pool`，`ITEMS <= 65536`。计时包含 resource setup、64 byte allocate/construct/volatile observe/destroy/deallocate、最终 release 和 backing 释放。`upstream_allocations` 是 `memory_resource` 上游调用，不等于 OS 系统调用；重复地址也不等于省掉 allocator 调用。

`io` 的 `VARIANT` 是 `buffered`、`mapped`、`completion`，`BYTES <= 8MiB`，`ROUNDS <= 16`，`DEPTH <= 16`。计时包含 source 打开/setup、读取及 owned chunk 物化、P1 Reference assemble、新目标写入/flush/close。source 生成和最后整文件对照在计时外。这里不清全局缓存，不承诺冷盘，也不承诺全 cache hit。

成功时 stdout 只有一行 JSON，含 `kind`、`variant`、`seconds`、stage 字段、counter 字段、`valid`、`clock_resolution_seconds`。真实能力不可用返回 77 并输出 `SKIP:`。

## Sampling Protocol

采样器默认 seed 是 `20260910`。它先用固定 seed 洗牌所有 case，逐个跑 1 个 warmup 进程；全部 warmup 有效后，再跑 5 个正式 round，每个 round 用 `seed + round` 重新洗牌 case 顺序。warmup 记录进 raw，但不进入统计。C++ 参数里的 `ROUNDS=2` 是一次进程内工作量，不是两个独立样本。

baseline phase：

- `io buffered 4096 2 4`
- `io buffered 1048576 2 4`
- `io buffered 8388608 2 4`
- `alloc heap 4096`
- `alloc heap 32768`

compare phase：

- `io mapped` 的 4 KiB、1 MiB、8 MiB，`ROUNDS=2 DEPTH=4`
- `io completion` 的 4 KiB、1 MiB、8 MiB，`DEPTH=1` 和 `DEPTH=4`
- `alloc arena/pool/std-monotonic/std-pool`，每个跑 4096 和 32768

每个样本保存 stdout、stderr、exit code、timeout、cleanup 状态和 JSON parse 结果。`VALID` 要求 exit code 为 0、stderr 为空、stdout JSON 是 object，`seconds` 和各阶段时间是有限、非 bool、非负实数，且总 `seconds > 0`；kind、variant、size/items/rounds/depth 必须匹配调度 case；I/O `bytes == size * rounds`；allocation checksum 和 upstream balance 必须满足协议。`FAIL`、`UNKNOWN`、`SKIP` 都不能进入 median/min/max。每个 case 必须正好 5 个正式 `VALID` 样本才统计；warmup 失败会阻断整个 phase，并保留已产生证据。

采样器在开始和结束分别 hash executable 以及这些源码依赖：B01 C++/CMake/采样器、C07 公共 headers、P1 Reference assemble、L05 Reference pmr、StudySetup。phase 期间 hash 变化则结果无效。compare 必须传 baseline result，并要求 baseline 与当前 executable/source fingerprint 完全一致。

## Commands

以下 Windows 命令从仓库根执行。先按[构建指南](../BUILD_GUIDE.md)构建 `verify-core`，再使用同一个 Release 二进制；输出目录必须全新。先读 baseline 的阶段/计数并记录待验证假设，再执行 compare。

```powershell
python C07_OS_Memory_System_IO/exercises/B01_costs/tools/sample_benchmarks.py --phase baseline --exe C07_OS_Memory_System_IO/exercises/build/verify-core/B01_costs/Release/B01_costs_benchmark.exe --output C07_OS_Memory_System_IO/references/measurements/my-windows-baseline
```

Windows compare：

```powershell
python C07_OS_Memory_System_IO/exercises/B01_costs/tools/sample_benchmarks.py --phase compare --exe C07_OS_Memory_System_IO/exercises/build/verify-core/B01_costs/Release/B01_costs_benchmark.exe --baseline-result C07_OS_Memory_System_IO/references/measurements/my-windows-baseline/result.json --output C07_OS_Memory_System_IO/references/measurements/my-windows-compare
```

Linux/WSL 从已构建 `linux-uring` 的原生 ext4 **快照仓库根**执行，不用 `/mnt/f` 工作树。未启用 io_uring 时不能把完整 compare 当已完成：

```bash
python3 C07_OS_Memory_System_IO/exercises/B01_costs/tools/sample_benchmarks.py --phase baseline --exe C07_OS_Memory_System_IO/exercises/build/linux-uring/B01_costs/B01_costs_benchmark --output C07_OS_Memory_System_IO/references/measurements/my-linux-baseline
python3 C07_OS_Memory_System_IO/exercises/B01_costs/tools/sample_benchmarks.py --phase compare --exe C07_OS_Memory_System_IO/exercises/build/linux-uring/B01_costs/B01_costs_benchmark --baseline-result C07_OS_Memory_System_IO/references/measurements/my-linux-baseline/result.json --output C07_OS_Memory_System_IO/references/measurements/my-linux-compare
```

自检只验证采样器规则，不跑真实 benchmark：

```powershell
python C07_OS_Memory_System_IO/exercises/B01_costs/tools/sample_benchmarks.py --self-check
```

正式采样结果冻结后，再写 `references/measurements/cost-analysis.md`。不要在只有协议和控制测试时提前写性能结论。
