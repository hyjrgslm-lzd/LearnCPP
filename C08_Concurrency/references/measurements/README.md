# 最终验证证据与实验附录

日期：2026-09-08。这里保存随仓库发布的原始证据；旧专题记录中的 `build/` 和 `.build` 路径仅是本机开发归档，不是远端可下载文件。

## 验证口径

- [离线核心 CTest](ctest-verify-core.xml)：60 项，58 PASS、2 SKIP（M2 未启用 stdexec；N1 无多节点条件），0 失败。
- [完整 Windows CTest](ctest-full-windows.xml)：61 项，60 PASS、1 SKIP（N1），0 失败；包含真实 xsimd/stdexec 以及严格判分/fast 内核独立目标。
- [练习入口记录](entry-roles.json)：16 个未完成实现型 Starter 均按预期返回 1；34 个观察型入口返回 0。前者不是参考实现失败，后者不代表全部 Part 已完成。
- [原生能力原始诊断](native-probes.json)：明确开启 C++26 后，六项指定 API 的实际编译/链接均未通过。不要把这个结果泛化为编译器完全没有相应领域能力，也不要把 OFF 当作失败探测。

CTest 两种配置的正确性运行可并行执行，持续时间不用于性能分析。其后停止了本任务的其他编译、测试和 agent 运行程序，再由主线程串行执行下面的正式采样；未锁定频率或控制整个操作系统的后台负载。

## 17 组正式数据

每个具体 variant 预热一次、五次独立进程正式采样；每次重建状态，种子 42 安排组内版本顺序，进程外上限 30 秒。共 36 次前置正确性程序检查，438 次基准进程（73 次预热、365 次正式）。正式进程中 345 PASS、20 SKIP，无 FAIL；不删去异常样本。

每组 `run.json` 保存原始 stdout/stderr、完整命令、环境、源码快照与二进制 hash、各轮状态及统计量；同目录 `samples.csv` 是正式有效行，不含预热或 SKIP。15 组为 PASS，两组为 PARTIAL_SKIP，后者原生退出码均为 77，未使用 `--allow-partial` 冒充完全通过。

| 数据组 | 关键参数 | 状态 | 比较边界 |
|---|---|---|---|
| [cap3](final-20260908/cap3/run.json) | `--size 48 --block 16 --threads 2 --items 65537` | PASS | GEMM、归约、排序按 suite 分开 |
| [cap3-empty](final-20260908/cap3-empty/run.json) | `--size 0 --items 0 --threads 2` | PASS | 元数据/零工作检查，不作性能排名 |
| [layout](final-20260908/layout/run.json) | `--size 100000 --threads 2 --batch 256` | PASS | 区分伪共享、真共享与批量交接 |
| [numa](final-20260908/numa/run.json) | `--size 1048576` | PARTIAL_SKIP | 单 reader 与双 reader 分组；无远端结论 |
| [policy-heavy-1024](final-20260908/policy-heavy-1024/run.json) | `--workload heavy --size 1024` | PASS | 同一负载、同一规模比较五种策略 |
| [policy-heavy-1048577](final-20260908/policy-heavy-1048577/run.json) | `--workload heavy --size 1048577` | PASS | 同一负载、同一规模比较五种策略 |
| [policy-heavy-65537](final-20260908/policy-heavy-65537/run.json) | `--workload heavy --size 65537` | PASS | 同一负载、同一规模比较五种策略 |
| [policy-light-1024](final-20260908/policy-light-1024/run.json) | `--workload light --size 1024` | PASS | 同一负载、同一规模比较五种策略 |
| [policy-light-1048577](final-20260908/policy-light-1048577/run.json) | `--workload light --size 1048577` | PASS | 同一负载、同一规模比较五种策略 |
| [policy-light-65537](final-20260908/policy-light-65537/run.json) | `--workload light --size 65537` | PASS | 同一负载、同一规模比较五种策略 |
| [queue-batch](final-20260908/queue-batch/run.json) | `--size 100003 --producers 3 --consumers 4 --capacity 64 --batch 8` | PASS | 批量独立组，不直接推算单项加速比 |
| [queue-mpsc](final-20260908/queue-mpsc/run.json) | `--size 100003 --producers 3 --consumers 1 --capacity 64 --batch 1` | PASS | 单消费者场景中的 MPSC/MPMC |
| [queue-ms](final-20260908/queue-ms/run.json) | `--size 100003 --producers 3 --consumers 4 --capacity 0 --batch 1` | PASS | 无界节点/HP 独立组 |
| [queue-spsc](final-20260908/queue-spsc/run.json) | `--size 100003 --producers 1 --consumers 1 --capacity 64 --batch 1` | PASS | SPSC 两版本；不与 MPMC 混排 |
| [queue-storage](final-20260908/queue-storage/run.json) | `--size 100003 --producers 3 --consumers 4 --capacity 64 --batch 1` | PASS | 同角色/容量的锁内存储对照 |
| [scheduling](final-20260908/scheduling/run.json) | `--size 1024 --threads 4` | PASS | 同一偏斜任务集的三种调度 |
| [simd](final-20260908/simd/run.json) | `--size 262147` | PARTIAL_SKIP | 加法、点积、布局按 suite 分开 |

SIMD 两个原生分支各五次正式 SKIP；NUMA remote/interleaved 各五次正式 SKIP。可选条件不具备不是成功测量，原始 stderr 保留原因。N1 Reference 的三个必需分支缺条件使整题返回 77；NUMA 采样前另用 J3 Reference 与 N1 的本地观察探针检查已具备的条件，并没有把 N1 Reference 改成 PASS。

## 怎样读这些数值

单位均为毫秒。这里选取有助于解释推导的中位数和范围；完整五个值在各组 JSON/CSV，不能只用本表代替原始样本。

| 同组对照 | 中位数及范围 | 可以作出的有限观察 |
|---|---|---|
| mutex / ring | 13.3535 [11.3654, 22.0667] / 14.3542 [13.8961, 18.6785] | 预分配版本未取得更低中位数，范围重叠；不支持“预分配必然加速” |
| spsc / spsc-cached | 0.8393 [0.8166, 0.9259] / 0.9324 [0.7839, 1.8106] | 本组缓存版未显示稳定优势，保留其较大波动 |
| packed / padded | 1.2563 [1.1767, 1.4828] / 0.4796 [0.4732, 0.5217] | 当前两个线程计数模型下布局分离耗时较低；未测硬件一致性事件 |
| dot_scalar / dot_sse2 / dot_xsimd | 0.1466 [0.1451, 0.1532] / 0.0486 [0.0443, 0.0499] / 0.7964 [0.7858, 0.8216] | xsimd 路径明显更慢也原样保留；没有 profiler 证据时不归因于某条指令 |
| GEMM naive / tiled / threaded | 0.0396 [0.0394, 0.0523] / 0.0432 [0.0422, 0.0440] / 0.4085 [0.3935, 0.4741] | n=48 时分块和线程版没有击败 naive，不能忽略启动和清零边界 |
| static / dynamic / stealing | 2.7077 [2.6558, 3.0715] / 1.0726 [1.0339, 1.1720] / 1.3166 [1.2851, 1.9922] | 这个偏斜任务集的 dynamic 较低；不证明窃取对任何应用都无价值 |

轻/重负载分别比较 plain 与 par，不能将不同函数的耗时相除称为算法加速：

| 负载 / 规模 | plain 中位数 | par 中位数 |
|---|---:|---:|
| light / 1024 | 0.0002 | 0.0263 |
| light / 65537 | 0.0037 | 0.0358 |
| light / 1048577 | 0.1328 | 0.2415 |
| heavy / 1024 | 0.0408 | 0.0848 |
| heavy / 65537 | 2.6010 | 0.6426 |
| heavy / 1048577 | 41.5228 | 3.6221 |

light 三档均未显示 par 的中位数优势；heavy 随规模增大出现不同结果，符合“固定开销更可能被较大计算量摊薄”的解释，但这不是对实际 worker 数、频率或瓶颈的测量。最短 light 区间很小，计时量化与扰动占比高，不应放大成精确微观延迟定律。空任务组仅检查已知零 worker 的表示，不进入性能排名。

NUMA 仅有 node 0。firsttouch 与 parallel-init 的双 reader 条件相互可比；local 的单 reader 是另一组。本次没有任何远端或交错放置的耗时证据，也没有连续观测页迁移或 page fault。所有布局、分配粒度、请求与实际节点的诊断均在该组原始记录中。

## 环境、版本与再现

环境为 Windows x64、MSVC 19.51.36256、CMake 4.2.3、AMD64 Family 25 Model 97 Stepping 2、32 逻辑 CPU。使用 Release 的 full-windows 构建；各报告记录实际编译器与缓存设置，源码上的 scalar 名称不保证机器代码未向量化。普通实验未强制关闭自动向量化，严格/fast 对照另由独立专项验证，不混入这里的耗时排名。

固定 xsimd 13.2.0 / `1f8dd9c8e162968d9b4ff0251c56d431b8777f36`，stdexec nvhpc-26.05 / `6d7ad689f4d4831c5136e4abe1c601f9a3b64e43`。CMake C++23 target 在本机实际映射 `/std:c++latest`，不能仅凭这个选项声称标准库 C++26 已实现。

17 组的 runner 源码快照均为：

```text
9ec7e92b69a9860f33f40bba0d6eb5d18d58065011ae4d4b159a23ffa066dcb6
```

它是运行器所定义范围在采样时的字节快照，不是 Git commit，也不是生成最终报告之后整棵目录的 hash。采样期间源码和二进制未变；其后补写报告会改变包含文档的全树摘要，不能因此替换旧样本的摘要。文件换行格式也会影响字节 hash。原始 JSON 保留采样时数据，不伪造为后续报告版本。

从[构建指南](../../exercises/BUILD_GUIDE.md)建立对应配置，按各 `runner_command` 的参数重跑，替换本机绝对路径并使用全新输出目录；不能覆盖这里的封存结果。无需期望得到相同耗时，但应能运行同一输入、契约、检查及采样流程。独立审查范围和证据限制见[质量报告](../quality-report.md)。
