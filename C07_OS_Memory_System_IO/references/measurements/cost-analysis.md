# B01 成本实验分析

本文只解释本次冻结的 B01 数据，不做跨平台排名。Windows 样本来自 `C:\Users\zhidan.li\AppData\Local\Temp\c07-test-*`，实际文件系统记录为 NTFS；WSL/Linux 样本来自 `/tmp/c07-test-*`，`findmnt -T /tmp` 记录为 ext4。两边都在同一台 24 逻辑 CPU 主机上运行，但没有做全局页缓存清理、CPU 绑定、电源策略固定或后台负载隔离，所以 5 个正式样本只能支持当前协议下的观察。

原始证据入口：

- [Windows baseline](b01-windows-baseline/result.json) / [Windows compare](b01-windows-compare/result.json)
- [WSL/Linux baseline](b01-linux-baseline/result.json) / [WSL/Linux compare](b01-linux-compare/result.json)
- [baseline 前置判断](baseline-assessment.md)
- [独立算术复核](arithmetic-audit.json)
- [Windows 环境](environment-windows.json) / [WSL/Linux 环境](environment-linux.json)
- [Windows temp 文件系统](filesystem-windows-temp.json) / [Linux /tmp 文件系统](filesystem-linux-temp.json)
- replay 命令记录：[Windows baseline](b01-windows-baseline-run.json)、[Windows compare](b01-windows-compare-run.json)、[Linux baseline](b01-linux-baseline-run.json)、[Linux compare](b01-linux-compare-run.json)

四个 phase 均为 `phase_valid=true`。baseline 为 5 个 case，每 case 1 个 warmup 和 5 个正式进程；compare 为 17 个 case，每 case 1 个 warmup 和 5 个正式进程。合计 264 条原始记录，其中 44 条 warmup、220 条正式记录；这些计数也由 `arithmetic-audit.json` 独立复核为 `PASS`。统计值只来自 `verdict=VALID` 且 `warmup=false` 的样本。Windows C++ 计时分辨率为 100 ns，环境中的 Python `perf_counter` 也记录为 `QueryPerformanceCounter()` 100 ns；Linux C++ 与 Python `perf_counter` 均为 `clock_gettime(CLOCK_MONOTONIC)` 1 ns。Python supervisor 的 `process_seconds` 只证明子进程运行与清理，不用于性能结论。

Windows baseline/compare 使用同一个 executable SHA-256：`c557ef164161acf5161ffd32217bf0669d931a951a68e2454d9be1346fe2a992`。Linux baseline/compare 使用同一个 executable SHA-256：`639cf402833c15c42081e27e91e6ee0d0f08fbffcbc1282f38e79070a2ab368d`。每个 phase 的开始/结束 fingerprint 相同；同一平台内 compare 还引用了对应 baseline 的 `result.json`。

## I/O 结果

I/O 每个正式样本是一个独立进程，进程内执行两轮完整管线。总时间包含 open/setup、read 或 map、owned chunk 物化、assemble、新目标文件 create/write/flush/close。整文件对照在计时外。`write_seconds` 是 sink 阶段整体，不能只解释成 flush 成本；`read_calls`、`completions` 是课程计数，不能直接等同 OS syscall 数。`app_copy` 只记录读后端为了产生 owned chunks 而做的显式物化复制，不包含 P1 assemble、通用容器复制、输入初始化或内核内部复制；buffered 的 0 不代表全链路零复制。

### Windows I/O

单位：总时间列为 `median/min/max/stdev`，阶段列和计数列均为 5 个正式样本的 median，时间单位为 ms。阶段 median 相加不保证等于总时间 median。

| case | total | setup | read | assemble | write | read_calls | completions | peak | app_copy |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| buffered 4 KiB d4 | 5.716/5.326/6.215/0.348 | 0.156 | 0.021 | 0.013 | 5.528 | 2 | 0 | 0 | 0 |
| mapped 4 KiB d4 | 5.742/5.622/5.837/0.081 | 0.252 | 0.011 | 0.010 | 5.432 | 0 | 0 | 0 | 8192 |
| completion 4 KiB d1 | 5.616/5.461/6.560/0.447 | 0.246 | 0.027 | 0.009 | 5.272 | 2 | 2 | 1 | 8192 |
| completion 4 KiB d4 | 5.737/5.654/6.575/0.380 | 0.234 | 0.025 | 0.008 | 5.494 | 2 | 2 | 1 | 8192 |
| buffered 1 MiB d4 | 21.436/21.380/22.522/0.590 | 0.230 | 0.574 | 0.259 | 20.343 | 32 | 0 | 0 | 0 |
| mapped 1 MiB d4 | 22.571/22.143/24.525/0.989 | 0.369 | 0.987 | 0.297 | 21.007 | 0 | 0 | 0 | 2097152 |
| completion 1 MiB d1 | 21.631/21.500/22.974/0.638 | 0.208 | 0.613 | 0.293 | 20.459 | 32 | 32 | 1 | 2097152 |
| completion 1 MiB d4 | 22.149/21.374/24.436/1.221 | 0.395 | 0.767 | 0.279 | 20.605 | 32 | 32 | 4 | 2097152 |
| buffered 8 MiB d4 | 136.302/133.176/137.832/1.751 | 0.273 | 5.893 | 2.148 | 128.006 | 256 | 0 | 0 | 0 |
| mapped 8 MiB d4 | 139.265/138.268/148.136/4.365 | 0.424 | 7.624 | 2.143 | 127.542 | 0 | 0 | 0 | 16777216 |
| completion 8 MiB d1 | 137.175/134.194/137.895/1.547 | 0.319 | 6.431 | 2.188 | 128.283 | 256 | 256 | 1 | 16777216 |
| completion 8 MiB d4 | 136.040/135.222/142.022/2.888 | 0.386 | 6.214 | 2.183 | 127.357 | 256 | 256 | 4 | 16777216 |

Windows 的主成本稳定落在 sink 阶段。4 KiB、1 MiB、8 MiB 的 write median 分别约 5.3 ms、20.5 ms、128 ms，明显大于 read/assemble。mapped 把 `read_calls` 变成 0，但它仍要把 mapping 内容物化为 owned chunks；在 1 MiB 与 8 MiB 上总时间 median 反而高于 buffered。completion 能证明 IOCP 路径真的产生 completion；depth 4 在 1 MiB/8 MiB 的 `peak=4`，4 KiB 每轮只有 1 个 chunk，两轮累计 2 次请求但不会并发，所以 peak 只能到 1。端到端上，8 MiB completion d4 与 buffered 很接近，median 低 0.262 ms，但 stdev 2.888 ms，大于差值，不能宣称稳定收益。

### WSL/Linux I/O

单位同上：总时间列为 `median/min/max/stdev`，阶段列和计数列均为 5 个正式样本的 median，时间单位为 ms。阶段 median 相加不保证等于总时间 median。

| case | total | setup | read | assemble | write | read_calls | completions | peak | app_copy |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| buffered 4 KiB d4 | 9.218/8.039/9.943/0.781 | 0.009 | 0.010 | 0.003 | 9.196 | 2 | 0 | 0 | 0 |
| mapped 4 KiB d4 | 10.123/6.667/11.707/2.045 | 0.031 | 0.005 | 0.004 | 10.057 | 0 | 0 | 0 | 8192 |
| completion 4 KiB d1 | 9.297/7.667/9.413/0.731 | 0.098 | 0.018 | 0.003 | 9.081 | 2 | 2 | 1 | 8192 |
| completion 4 KiB d4 | 8.871/6.914/10.141/1.259 | 0.115 | 0.023 | 0.004 | 8.667 | 2 | 2 | 1 | 8192 |
| buffered 1 MiB d4 | 14.505/14.099/14.854/0.358 | 0.013 | 0.338 | 0.474 | 13.764 | 32 | 0 | 0 | 0 |
| mapped 1 MiB d4 | 13.476/11.764/14.364/0.977 | 0.033 | 0.265 | 0.366 | 12.379 | 0 | 0 | 0 | 2097152 |
| completion 1 MiB d1 | 14.566/12.979/30.115/7.263 | 0.126 | 0.404 | 0.416 | 13.528 | 32 | 32 | 1 | 2097152 |
| completion 1 MiB d4 | 13.057/12.030/16.449/1.725 | 0.151 | 0.328 | 0.382 | 12.048 | 32 | 32 | 4 | 2097152 |
| buffered 8 MiB d4 | 48.982/48.470/57.762/3.887 | 0.034 | 2.322 | 3.417 | 43.232 | 256 | 0 | 0 | 0 |
| mapped 8 MiB d4 | 50.497/49.325/51.377/0.922 | 0.064 | 2.168 | 3.614 | 44.159 | 0 | 0 | 0 | 16777216 |
| completion 8 MiB d1 | 50.025/48.928/93.882/19.223 | 0.176 | 2.746 | 3.347 | 43.674 | 256 | 256 | 1 | 16777216 |
| completion 8 MiB d4 | 51.571/50.837/65.706/6.448 | 0.199 | 2.672 | 3.620 | 44.560 | 256 | 256 | 4 | 16777216 |

Linux 的 sink 阶段同样主导端到端时间，但样本离散度比单看 median 更关键。1 MiB mapped 与 completion d4 的 median 低于 buffered，主要也表现为 write 阶段 median 降低；这不是 reader 自身“更快”的单独证明，因为 baseline 与 compare 是分 phase 运行，不是同一个 case 交错运行。8 MiB 上 buffered median 仍最低，completion d1 出现 93.882 ms 的 max，completion d4 也有 65.706 ms 的 max。这里更适合教学结论：io_uring 路径确实执行并产生 completion/depth，但当前端到端管线仍由 sink 主导，调度和后台干扰也没有控制；这些尖峰原因尚未定位，不能从 5 次样本推出稳定生产优势。

## Allocation 结果

allocation 每轮只有一个 live 对象：分配 64 byte、构造 `uint64_t`、volatile 观察、销毁、释放。计数 resource 的 upstream allocation/deallocation 是 pmr 上游请求，不等于 OS 系统调用。地址复用只说明同一进程内返回地址重复，不能单独证明 allocator 没工作。

### Windows allocation

单位：总时间列为 `median/min/max/stdev`，阶段列和计数列均为 5 个正式样本的 median，时间单位为 us。阶段 median 相加不保证等于总时间 median。

| case | total | setup | loop | cleanup | upstream alloc/dealloc | upstream bytes | peak bytes | address reuse |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| heap 4096 | 86.900/85.600/93.900/3.871 | 0.900 | 84.600 | 0.900 | 4096/4096 | 262144 | 64 | 289 |
| arena 4096 | 51.700/48.700/52.000/1.356 | 38.400 | 11.600 | 1.500 | 1/1 | 262208 | 262208 | 0 |
| pool 4096 | 17.300/17.000/26.000/3.881 | 5.300 | 10.800 | 1.200 | 1/1 | 192 | 192 | 4095 |
| std-monotonic 4096 | 106.800/105.000/130.800/11.173 | 1.100 | 77.300 | 28.700 | 18/18 | 278672 | 278672 | 0 |
| std-pool 4096 | 31.900/30.900/38.000/2.930 | 1.200 | 28.400 | 2.300 | 2/2 | 616 | 616 | 4095 |
| heap 32768 | 677.300/665.200/1145.000/208.854 | 1.100 | 675.300 | 1.000 | 32768/32768 | 2097152 | 64 | 3206 |
| arena 32768 | 352.600/338.700/371.600/12.188 | 209.200 | 92.000 | 53.100 | 1/1 | 2097216 | 2097216 | 0 |
| pool 32768 | 90.800/90.100/91.100/0.396 | 4.600 | 85.300 | 1.100 | 1/1 | 192 | 192 | 32767 |
| std-monotonic 32768 | 519.700/493.500/541.800/19.696 | 1.100 | 410.600 | 108.100 | 23/23 | 2117896 | 2117896 | 0 |
| std-pool 32768 | 192.500/184.500/620.700/192.967 | 1.200 | 189.300 | 2.000 | 2/2 | 616 | 616 | 32767 |

Windows 上定长 pool 最符合这个微基准：只有一个 64 byte live 对象，pool 只需 192 byte upstream，几乎每次都复用同一地址。arena 把上游调用压到 1 次，但预留了接近总请求量的 backing store，setup 与 cleanup 明显变大。标准 `monotonic_buffer_resource` 和 `unsynchronized_pool_resource` 也减少上游调用，但并不自动赢过手写 pool；尤其 `std-pool 32768` 的 max 620.700 us 说明单次慢样本会改变直觉。

### WSL/Linux allocation

单位同上：总时间列为 `median/min/max/stdev`，阶段列和计数列均为 5 个正式样本的 median，时间单位为 us。阶段 median 相加不保证等于总时间 median。

| case | total | setup | loop | cleanup | upstream alloc/dealloc | upstream bytes | peak bytes | address reuse |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| heap 4096 | 39.111/38.291/39.661/0.536 | 0.160 | 38.771 | 0.130 | 4096/4096 | 262144 | 64 | 4095 |
| arena 4096 | 58.884/56.444/63.173/2.493 | 45.265 | 4.080 | 9.689 | 1/1 | 262208 | 262208 | 0 |
| pool 4096 | 5.469/4.879/5.729/0.354 | 1.410 | 3.029 | 1.020 | 1/1 | 192 | 192 | 4095 |
| std-monotonic 4096 | 59.293/51.085/87.220/14.350 | 0.090 | 38.296 | 20.208 | 12/12 | 264320 | 264320 | 0 |
| std-pool 4096 | 43.035/41.525/44.204/1.089 | 3.100 | 38.396 | 1.600 | 3/3 | 1688 | 1688 | 4095 |
| heap 32768 | 300.818/293.468/336.019/17.035 | 0.160 | 300.528 | 0.150 | 32768/32768 | 2097152 | 64 | 32767 |
| arena 32768 | 407.026/401.723/424.583/9.793 | 332.004 | 36.866 | 38.315 | 1/1 | 2097216 | 2097216 | 0 |
| pool 32768 | 25.727/25.526/27.477/0.817 | 0.989 | 23.908 | 1.060 | 1/1 | 192 | 192 | 32767 |
| std-monotonic 32768 | 349.482/316.010/404.043/34.564 | 0.150 | 301.517 | 46.814 | 18/18 | 3025664 | 3025664 | 0 |
| std-pool 32768 | 292.828/290.993/342.623/22.087 | 3.439 | 287.496 | 1.620 | 3/3 | 1688 | 1688 | 32767 |

Linux allocation 的机制信号更清楚：heap 每次都上游分配；pool 只有 1 次 upstream 且每次复用地址，median 从 heap 300.818 us 降到 25.727 us。arena 的 loop 很小，但 setup/cleanup 和 backing bytes 主导总成本；在“一次只活一个对象”的协议里，这不是好交换。标准 pmr 的 counters 验证了资源策略差异，但本次总时间未优于手写定长 pool。

## 结论边界

本实验验证的是课程机制，不是生产调优报告。可成立的结论有三条：

1. 当前 P1 风格端到端 I/O 管线里，sink 阶段是主要成本；只替换 reader 不足以解释端到端变化。
2. mapped 能减少课程层面的 `read_calls`，completion 能证明真实 completion 与 depth，但两者都没有消除 owned chunk 物化、assemble 和 sink 成本。
3. allocation 的 counters 必须和阶段时间一起读：减少 upstream 调用可能换来 setup、cleanup 或 peak bytes；在单 live 64 byte 场景里，定长 pool 最贴合协议。

不可成立的结论也要明示：不能跨 Windows 与 WSL/Linux 排名；不能把计数当作 syscall 数；不能把 5 个样本解释成统计显著；不能把 baseline 与 compare 的分 phase 结果当作同一 case 完全交错实验；不能宣称这些 variants 已经是生产优化方案。
