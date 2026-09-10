# 18 Measurement：把数字变成可复查的事实

系统性能测量最容易出错的地方，不是计时 API，而是把一个数字讲成它没有证明的结论。本章只建立 B01 的实验协议：每个数字来自哪个 executable、哪份源码、哪些输入、哪个进程、哪个 stdout JSON。

## 先锁住正确性

B01 的 C++ 入口先跑 `--check` 和 `--check-completion`。allocation 路径复用 L05 的真实 resource，检查 payload checksum、上游 allocate/deallocate 平衡和 peak 计数。I/O 路径复用 P1 的真实 `read_file_chunks()`、Reference `assemble()` 和 `write_new_file()`，每轮有轻量检查，计时外再做整文件对照。

这一步只说明“测的是正确程序”。它不说明哪个后端快。

## 一个进程才是一个样本

采样器先用固定 seed 洗牌 case，跑完整 warmup round；全部 warmup 有效后，再跑 5 个正式 round，每个 round 重新洗牌 case。这样不会把“某个 case 永远连续跑完”绑定到温度、页缓存或后台负载的时间漂移上。warmup 用来让动态链接、首次路径创建、库初始化等成本先暴露，但不进入统计。C++ 参数 `ROUNDS=2` 是进程内循环，用来让被测工作量更稳定；它仍然只是一个正式样本。

只有正式进程退出 0、stderr 为空、stdout 有且只有一行 JSON、JSON 是 object、JSON 标记 `valid: true`，并且 case 字段和数值字段通过协议校验，才算有效样本。Python JSON 接受 `NaN`，`bool` 也是 `int` 的子类，所以采样器显式拒绝 NaN/Infinity、负时间、bool 时间、错 case、错 byte count、错 checksum 和 upstream 不平衡。`SKIP`、timeout、cleanup 失败、stderr 报错、JSON 缺字段，全部保留原始记录，但不参与 median/min/max。每个 case 不足 5 个 valid，采样器不给统计值。

## Hash 比口头环境说明更可靠

采样器在 phase 开始和结束 hash executable 与关键源码：B01 benchmark、B01 CMake、采样器、C07 公共 headers、P1 Reference、L05 Reference 和 StudySetup。hash 变化说明测量期间程序定义变了，phase 无效。compare 还必须引用 baseline 的 `result.json`，并验证 baseline 和当前 fingerprint 一致。

这避免“先测 buffered，改了代码，再测 mapped，然后把两组数字放一起”的错误。

## I/O 数字的边界

B01 I/O 每个进程新建输入文件并 flush，然后在计时内执行 open/setup、read 或 map、owned chunk 物化、assemble、新目标 write/flush/close。最后整文件对照在计时外。

因此它测的是这条课程管线的端到端成本。它不是纯设备读带宽，不是 syscall 总数，也不是断电持久性测试。本实验不清全局页缓存，所以不能把一次结果解释成冷盘，也不能假设所有数据都在缓存里。

## Allocation 数字的边界

allocation case 每次分配 64 byte、构造 `uint64_t`、volatile 观察、防止死代码消除，然后销毁并释放。计数 resource 记录的是 pmr 上游调用、字节、峰值和平衡，不等于 OS 系统调用数。

arena、pool、standard pmr resource 的差异要结合 stage 时间和 counters 看。重复地址只说明同一进程内地址复用，不证明 allocator 没被调用。

## 输出之后再分析

正式输出是 `result.json` 和 `samples/*.json`。`result.json` 只有在所有 case 都有 5 个正式 valid 样本、fingerprint 前后一致时才标记 `phase_valid: true`。`samples` 保留每个子进程的 stdout/stderr/exit/timeout/cleanup/parsed 数据。

正式结果已经冻结在 [`references/measurements/cost-analysis.md`](../references/measurements/cost-analysis.md)。这份分析只按 Windows 与 WSL/Linux 分别解释，不跨系统排名；同一系统内也先看输入规模、stage、counter 和离散度，再给结论。

本轮最有教学价值的结果不是“哪个 API 更快”，而是几个反直觉边界：mapped 能把课程层面的 `read_calls` 降到 0，但仍要物化 owned chunks；completion 能证明 IOCP/io_uring 路径真的产生 completion 与 depth，但端到端仍受 sink 阶段支配；allocation 里减少 upstream 调用也可能换来 setup、cleanup 或 peak bytes。数字能支持机制判断，不能替代实验边界说明。
