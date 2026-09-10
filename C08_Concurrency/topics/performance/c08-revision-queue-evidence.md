# C08 修订：队列演进证据口径

> 后续正式采样已完成，见[本轮五组原始数据与结论](../../references/measurements/c08-revision-final/README.md)。下文保留样章阶段的定位过程、r1/r2计数和采样规格；这些诊断计数不替代无插桩的正式计时，也不回写为新性能样本。

本记录绑定队列 01-03 样章修订。它不新增正式 benchmark 结论，只说明哪些已有证据可用、哪些归因仍待测，以及新增诊断程序如何证明“真实接口调用路径”。

## 已核对的最终样本

仓库内已有 `references/measurements/final-20260908` 队列五组最终样本：

| 组 | 文件 | 口径 |
|---|---|---|
| storage | `queue-storage/run.json`、`samples.csv` | `mutex` 与 `ring`，P=3/C=4，capacity=64，batch=1 |
| batch | `queue-batch/run.json`、`samples.csv` | `batch`，P=3/C=4，capacity=64，batch=8 |
| spsc | `queue-spsc/run.json`、`samples.csv` | `spsc` 与 `spsc-cached`，P=1/C=1，capacity=64 |
| mpsc | `queue-mpsc/run.json`、`samples.csv` | `mpsc` 与 `mpmc`，P=3/C=1，capacity=64 |
| ms | `queue-ms/run.json`、`samples.csv` | `ms`，P=3/C=4，capacity=0 |

这些样本状态均为 PASS，且每组 run.json 保存了 `runner_command`、`source_sha256`、benchmark 二进制 hash、Reference 检查、暖机和五次独立进程正式样本。它们能证明对应历史源码快照下的采样协议成立，不能自动证明当前修改后的头文件性能。

## 样章允许的结论

`mutex` 是正确性基线：同一把 mutex 覆盖检查和修改，支持严格有界 FIFO 推导。性能问题只能先写成假设，例如锁调用、容器存储、满空重试或单项搬运成本。

`ring` 是同单元素合同下的预分配存储版本。旧最终样本中 `ring` 中位数高于 `mutex`，所以不能写“预分配必然更快”。它只说明一个可测变化：队列容器不再在热路径请求新节点或新存储块。

`batch` 改变调用粒度。它的旧样本不能和 `ring batch=1` 混成同一接口排名；应解释为“同一存储结构下，一次临界区尝试搬运前缀”。是否改善尾延迟、公平性或低流量等待，本组没有测。

`spsc` 是场景切换：单生产者、单消费者，角色所有权替代公共锁。`spsc-cached` 是 SPSC 分支内减少远端下标读取的版本。旧样本范围有重叠，不能推出固定加速比。

## 新诊断入口

新增 `exercises/benchmarks/queue_diagnostics.cpp`。它调用现有 `queue_lab::transfer` 驱动真实传输，并在 `CS_QUEUE_DIAGNOSTICS` 宏开启时统计三类实际边界：

- wrapper 统计公开接口调用次数、成功调用次数、成功元素数；
- 队列头内统计 mutex 成功获取后的临界区进入次数，以及 SPSC 读取对方原子下标的次数；
- driver 用本 TU 的 `operator new` 计数器，在单线程 push/pop 热区隔离记录分配调用次数，队列构造和 transfer 的外部 vector 分配不计入。

这些计数能证明 driver 实际走了对应公开接口和诊断插桩点，不能证明 mutex 等待时间、CAS 失败次数、cache miss 或系统调度原因。

最小自检命令：

```powershell
# 工作目录 C08_Concurrency/exercises
cmake --build ../../build/c08-evolution-author --config Release --target queue_diagnostics
ctest --test-dir ../../build/c08-evolution-author -C Release -R '^diagnostic_queue_counts$' --output-on-failure
./../../build/c08-evolution-author/benchmarks/Release/queue_diagnostics.exe
```

预期输出为 CSV。每行 `completed`、`push_items`、`pop_items` 均等于 10003；`batch8` 的成功元素数等于工作量，但公开调用和内部 mutex 进入次数低于单项版本。若以后要声明锁竞争、CAS 重试或 cache miss，需要另加 profiling、硬件计数器或更细算法插桩，并把插桩扰动和正式计时分开。正式计时 target 不开启 `CS_QUEUE_DIAGNOSTICS`。

作者 r1 自检保留在 `references/measurements/c08-revision-queue-evidence/queue_diagnostics.csv`，仅统计公开调用。r2 自检为 `queue_diagnostics-r2.csv`，结果摘要：

| variant | push calls | pop calls | mutex acquisitions | SPSC remote loads | hot `operator new` |
|---|---:|---:|---:|---:|---:|
| mutex | 11351 | 12351 | 23702 | 0 | 2 |
| ring | 11103 | 13300 | 24403 | 0 | 0 |
| batch8 | 1667 | 2118 | 3785 | 0 | 0 |
| spsc | 10433 | 11302 | 0 | 21735 | 0 |
| spsc-cached | 10587 | 10515 | 0 | 3405 | 0 |

五个 variant 的 `completed`、`push_items`、`pop_items` 都为 10003。`batch8` 的 mutex acquisitions 等于公开 push/pop batch 调用之和，支撑“一个 batch 调用进入一次 mutex 临界区”的当前源码映射。单线程热区 `operator new` 计数中，mutex baseline 为 2，ring 为 0；这个边界计的是当前实现/标准库实际调用到的分配函数，`std::deque` 可复用块，因此它不是“每次 push 一个分配”的证明。SPSC 缓存版远端下标 load 明显少于普通版，支撑“缓存减少刷新次数”，仍不支撑 cache miss 或加速结论。

## 正式采样清单

正式测量须等待主线程提供独占测量窗口，且源码和二进制在窗口内保持不变。每组一轮 warmup、五次独立进程、seed=42、timeout=30，保留负收益和所有原始样本：

```powershell
python tools/run_benchmarks.py --exe build/full-windows/benchmarks/Release/queue_bench.exe --check build/full-windows/runtime_tests/Release/runtime_queue_history_test.exe --check build/full-windows/Q0_queue_baseline/Release/Q0_queue_baseline_reference.exe --check build/full-windows/Capstone2_lockfree_queue/Release/Capstone2_lockfree_queue_reference.exe --variant mutex --variant ring --seed 42 --warmups 1 --samples 5 --timeout 30 --output ../references/measurements/c08-revision-queue-evidence/queue-storage -- --size 100003 --producers 3 --consumers 4 --capacity 64 --batch 1
python tools/run_benchmarks.py --exe build/full-windows/benchmarks/Release/queue_bench.exe --check build/full-windows/runtime_tests/Release/runtime_queue_history_test.exe --check build/full-windows/Capstone2_lockfree_queue/Release/Capstone2_lockfree_queue_reference.exe --variant batch --seed 42 --warmups 1 --samples 5 --timeout 30 --output ../references/measurements/c08-revision-queue-evidence/queue-batch -- --size 100003 --producers 3 --consumers 4 --capacity 64 --batch 8
python tools/run_benchmarks.py --exe build/full-windows/benchmarks/Release/queue_bench.exe --check build/full-windows/runtime_tests/Release/runtime_queue_history_test.exe --check build/full-windows/G3_spsc_ringbuffer/Release/G3_spsc_ringbuffer_reference.exe --variant spsc --variant spsc-cached --seed 42 --warmups 1 --samples 5 --timeout 30 --output ../references/measurements/c08-revision-queue-evidence/queue-spsc -- --size 100003 --producers 1 --consumers 1 --capacity 64 --batch 1
python tools/run_benchmarks.py --exe build/full-windows/benchmarks/Release/queue_bench.exe --check build/full-windows/runtime_tests/Release/runtime_queue_history_test.exe --check build/full-windows/Q1_mpsc_queue/Release/Q1_mpsc_queue_reference.exe --check build/full-windows/Capstone2_lockfree_queue/Release/Capstone2_lockfree_queue_reference.exe --variant mpsc --variant mpmc --seed 42 --warmups 1 --samples 5 --timeout 30 --output ../references/measurements/c08-revision-queue-evidence/queue-mpsc -- --size 100003 --producers 3 --consumers 1 --capacity 64 --batch 1
python tools/run_benchmarks.py --exe build/full-windows/benchmarks/Release/queue_bench.exe --check build/full-windows/runtime_tests/Release/runtime_queue_history_test.exe --check build/full-windows/Q2_ms_queue/Release/Q2_ms_queue_reference.exe --variant ms --seed 42 --warmups 1 --samples 5 --timeout 30 --output ../references/measurements/c08-revision-queue-evidence/queue-ms -- --size 100003 --producers 3 --consumers 4 --capacity 0 --batch 1
```

这些命令输出到新目录。若目录已存在，runner 应拒绝覆盖。

## 其余性能演进缺证据表

本轮按现有正文与 `final-20260908` 样本做只读审计，没有补跑正式性能组：

| 主题 | 现有证据 | 当前结论边界 |
|---|---|---|
| 布局/伪共享 | `final-20260908/layout` 有 runner 样本；J1/J2 正文有布局地址、atomic 更新次数和合同说明 | 可讨论不同对象布局与共享更新次数；未测硬件 cache miss 和线程亲和，不能把耗时差直接归因为某个缓存事件 |
| 并行策略 light/heavy | `final-20260908/policy-light-*` 与 `policy-heavy-*` 有三档规模样本 | 可比较同一负载、同一规模下的样本趋势；`threads=0` 仍表示标准库实际线程数未测 |
| GEMM/Cap3 | `final-20260908/cap3`、`cap3-empty` 有样本 | 可验证驱动元数据与小规模算法成本；不是经调优 BLAS 结论，不能外推到大矩阵或不同布局 |
| SIMD | `final-20260908/simd` 有样本，SIMD 正文另有精度与带宽诊断说明 | 可说明当前实现和输入下的样本；C++26 `std::simd` 原生路径仍按能力探测单独记录 |
| 调度/NUMA | `final-20260908/scheduling`、`numa` 有样本 | 可说明课程驱动在本机的观测；NUMA 位置、亲和请求和实际页放置不能互相替代 |

缺少硬件计数器、亲和隔离、频率控制或正式空闲窗口时，正文只能保留为“可解释趋势”或“待验证假设”。后续若要采纳/排除具体优化，需要用同口径 runner 加上能定位到具体操作或资源的对照证据。
