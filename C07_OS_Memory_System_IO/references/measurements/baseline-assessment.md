# B01 基线定位与对照假设

此记录在两平台 baseline 结束、compare 尚未启动时写入。基线均为 5 个 case，各 1 个预热进程、5 个正式进程，共 30 个原始记录；两份 `phase_valid=true`。所有后端/资源先作为独立机制教学实现，没有根据未取得的性能结果替换默认策略。

原始基线：[Windows](b01-windows-baseline/result.json)、[WSL/Linux](b01-linux-baseline/result.json)。使用本次最终 Release B01 二进制；phase 开始/结束的 executable 与 13 个源码输入指纹相同。Windows/Linux 分别运行，没有同时采样，也没有运行本任务构建。

## 已定位到的阶段

表中总时间及 write 阶段均为各自 5 个值的中位数（毫秒）；中位数相加不保证等于总时间中位数。I/O 每进程执行两轮完整管线。

| 平台 | 每轮文件 | 总时间 median | write/flush/close median | read_calls / 进程 |
|---|---:|---:|---:|---:|
| Windows | 4 KiB | 5.716 | 5.528 | 2 |
| Windows | 1 MiB | 21.436 | 20.343 | 32 |
| Windows | 8 MiB | 136.302 | 128.006 | 256 |
| WSL/Linux | 4 KiB | 9.218 | 9.196 | 2 |
| WSL/Linux | 1 MiB | 14.505 | 13.764 | 32 |
| WSL/Linux | 8 MiB | 48.982 | 43.232 | 256 |

在这些基线输入上，主成本落在 sink 的新文件创建、write、flush 和 checked close 整个阶段。这里没有把 flush 单独计时，所以不能进一步认定某个 syscall、设备或驱动是根因。读取阶段已经独立测量：8 MiB 时 Windows 约 5.893 ms，Linux 约 2.322 ms，远小于各自 sink 阶段。只换 reader 预期无法消除主要端到端成本。

heap 每次 64 byte allocate/construct/observe/destroy/deallocate。4096 / 32768 次请求对应相同数目的上游 allocation，最大同时 outstanding 只有 64 byte。Windows 总时间中位数 86.9 / 677.3 us，Linux 39.111 / 300.818 us；主要在 loop 阶段。Windows 大规模样本 min 665.2 us、max 1145.0 us，波动必须保留，不能只引用最佳一次。

## 允许进入 compare 的问题

1. **mapped**：普通 read API 调用计数应减少，但 owned chunk 物化和 P1 assemble 仍有应用复制。检查 read/setup/copy 计数，不能从 read_calls=0 宣称零复制或无系统调用。是否降低端到端时间待数据判断。
2. **completion depth 1/4**：大文件应观察到实际多个在途请求和对应完成数，小文件只有一个 chunk 时不能达到 depth 4。比较 reader/setup 和总时间；sink 占主导时不能只凭总时间小变化宣称 IOCP/io_uring 优化成功。
3. **arena/pool/标准 pmr**：同一循环内一次只活一个对象，定长池可复用少量块；arena 不逐次回收，可能以更大 backing 和 setup 成本交换较少上游调用。先检查 allocation/deallocation、peak bytes 和地址复用，再解释 setup/loop/cleanup 及离散度。减少上游调用不自动等于更快。

判定口径：每个 compare case 正好 5 个 VALID 正式样本才统计；保留 min/max/stdev。计数变化可以验证机制预测；有限 5 次数据只支撑当前协议的观察，不用于跨平台排名、冷盘带宽、生产服务延迟或普遍优劣结论。此时没有采纳或排除任何性能方案。
