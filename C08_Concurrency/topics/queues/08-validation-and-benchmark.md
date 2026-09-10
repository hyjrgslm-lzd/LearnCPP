# 队列 08：把正确性历史和吞吐样本分别解释

2026-09-10的新源码正式采样见[本轮五组数据](../../references/measurements/c08-revision-final/README.md)，对应[新增定位证据](../performance/c08-revision-queue-evidence.md)。本篇保留原协议与历史实验说明；旧样本、当前诊断、当前正式计时分别使用各自的版本绑定。

如果一个错误队列丢掉一半元素，它可能“跑得更快”。如果检查只看 push/pop 次数，另一个丢一项又重复一项的队列可能被误判正确。本系列把协议推导、逐项数据检查、小规模历史和计时分成几层，每一层解决不同的问题。

实现都来自相同头文件：基线是 [`queue_baseline.hpp`](../../exercises/include/concurrency_study/queue_baseline.hpp)，数组演进是 [`queue_versions.hpp`](../../exercises/include/concurrency_study/queue_versions.hpp)，动态结构是 [`queue_linked.hpp`](../../exercises/include/concurrency_study/queue_linked.hpp)。Reference、历史检查与 [`queue_bench.cpp`](../../exercises/benchmarks/queue_bench.cpp) 复用这些类型；没有为基准偷偷写一套少做回收或少做同步的快版本。

## 1. 先逐项检查传输

[`queue_checks.hpp`](../../exercises/include/concurrency_study/queue_checks.hpp) 的 transfer 为每个生产者划定连续 ID 区间。若总量为 N、生产者数为 P，前 N%P 个生产者各分到 N/P+1 个，其余各 N/P 个。第 p 个区间的起点是 `p*(N/P)+min(p,N%P)`，所有区间连续、不重叠，合起来恰好覆盖 `[0,N)`。

消费者只往自己拥有的 vector 中记录 ID，没有每次操作都更新同一个全局计数器。join 后合并、排序，并检查第 i 项等于 i。成功总数相等只是第一步；逐项相等还能发现重复、缺失和越界。单消费者场景另外检查每个生产者的序号，SPSC 的完整消费序列因此严格递增。

每个 worker 都捕获异常，保存到专属 exception_ptr 槽并发出取消。重试循环检查取消；主线程 join 后重新抛出。线程创建失败也会释放启动门并取消已经创建的线程。取消不等于“成功传完”，不会把失败计成有效基准样本。

成功传输完成后，生产者发布一次完成计数。消费者先 acquire 确认所有生产者完成，再做最后一次队列观察，避免使用完成事件之前的一次空观察直接退出。对于本系列的槽位版本，全部 push 已结束意味着生产者发布空隙也已关闭。这个驱动适用于有限、预先给定的批次，不是通用在线任务服务的 close 实现。

## 2. 历史检查为什么记录两个时间点

[`queue_history_test.cpp`](../../exercises/runtime_tests/queue_history_test.cpp) 对每次调用记录 begin、end、操作类型、返回值与元素。序号来自仅用于正确性测试的原子时钟。若 A.end 小于 B.begin，任何合法串行解释都必须把 A 放在 B 前面；若区间重叠，两者可尝试任意顺序。

检查器对最多 12 个实际操作做深度优先搜索，每次选一个没有未满足实时前驱的操作，用顺序容器执行其语义。mutex 两版采用严格有界 FIFO；MS 采用严格无界 FIFO；Treiber 用 LIFO；SPSC 采用保守失败模型；MPSC/MPMC 明确允许 false 不改变逻辑状态。成功取值仍必须匹配模型中的队首或栈顶，不能为了让历史通过就忽略成功顺序。

检查器自身也有反例：push 10 后 pop 20 必须拒绝；另外给定“早票未发布、晚票 push 已返回、随后 pop false”的历史，严格 FIFO 模型必须拒绝，允许暂时失败的模型可以接受。然后真正运行两种 sequence 队列的暂停预订实验，把抽象反例连接到实际代码。

这里仍有边界。有限历史搜索不穷尽无限执行；宽计数不能在压力测试里等待几百年回绕，所以另用同一模板的 8 位模型检查安全距离内的回绕，并展示完整周期后旧值重新相等。当前自动历史搜索按单元素操作建模；批量的原子前缀语义由同一临界区推导及 Capstone 的前缀/并发逐项检查覆盖，未把批量调用伪装成已做完整批量历史穷举。任何故意 UB 的扩展都应放到显式启用、外部超时隔离的诊断程序，不能混入性能排名。

## 3. 一次进程只承担一轮计时

queue_bench 使用公共 `cs::bench::arguments` 消费参数、`measure_ms` 计时、`emit_row` 输出。一个进程只选一个具体 variant，输出一行结果；stdout 仅有公共 CSV 表头和数据行，错误写 stderr。没有 `all` 版本，没有内部暖机或隐藏重复。

| variant | 合法线程角色 | 容量与额外参数 |
|---|---|---|
| mutex | 任意正数 P/C | 正容量；batch=1 |
| ring | 任意正数 P/C | 正容量；batch=1 |
| batch | 任意正数 P/C | 正容量；batch 可为正数，例如 8 |
| spsc / spsc-cached | P=1、C=1 | 正的可用容量；batch=1 |
| mpsc | 任意正数 P、C=1 | 容量为至少 2 的二次幂；batch=1 |
| mpmc | 任意正数 P/C | 同上；batch=1 |
| ms | 任意正数 P/C | 必须显式 capacity=0 表示无界；batch=1 |

驱动为避免耗尽教学 HP 域，把生产者、消费者各限制为最多 32。即使 MS 同时有 32 个生产者和 32 个消费者，最多使用 32+2×32=96 槽，低于本进程 128 槽预算。这个独立程序没有其他 HP 使用者；把队列嵌入更大应用时应重新计算总预算。没有合适角色参数时返回错误，不是硬件 SKIP。只有实际缺失硬件/能力的专项才应使用 77；队列本身没有默认需要 SKIP 的硬件功能。

计时从 transfer 调用之前开始，到所有线程 join 并聚合成功计数后结束，包括线程创建、启动门、循环重试和 yield。队列构造与最终析构在区间外；MS 的逐次 new、HP 获取、退休和热路径触发的扫描在区间内，最后析构清场在外。内部按消费者本地成功数量统计 completed，一入一出算一个成功传输元素，吞吐可用 `completed*1000/milliseconds` 得到元素/秒。这里不把它乘二后继续叫元素吞吐。

计时关闭 ID trace，避免每元素 vector 记录成为被测主要工作；完整正确性在 runner 开始计时前独立运行。每个消费者仍有自己的计数，满空失败不计入 completed。details 记录 P、C、容量、batch、语义、实际原子属性、重试与计时边界。

## 4. 哪些结果可以放在同一张对照表

第一组是相同容量、相同 P/C、单元素严格 try 的 mutex 与 ring，研究预分配存储的影响。第二组固定同一 ring 存储，比较 ring 的 batch=1 与 batch 的指定批量，明确这里改变了调用粒度，吞吐提升可能伴随延迟与公平性变化。

第三组在 P=1、C=1 下比较普通和缓存 SPSC，可加入 mutex/ring 作为共同成功传输工作量的参考。第四组可在 P=4、C=1 比较 mpsc 与 mpmc 的分支成本，二者共享相同暂时失败契约；在 P=1、C=4 运行 mpmc 是 SPMC 拓扑测试，不把它标为专用 SPMC 最佳实现。

mutex 与 sequence 队列的单次 false 语义不同。若展示它们在相同容量/拓扑下的传输耗时，必须明确比较的是本驱动“失败就重试，直到所有元素成功完成”的共同工作负载，不能声称它们提供相同严格 try 契约。无界 MS 必须单独标明 capacity=0，并包含实际节点分配与回收成本，不和有界容量限制混成同一契约排名。

## 5. 外部 runner 的完整命令

以下在 `C08_Concurrency/exercises` 下执行，假设已由公共 CMake 构建 benchmark、runtime 与各 Reference。可执行文件目录因总构建或叶构建而不同，实际使用相应路径。runner 本身为 [`tools/run_benchmarks.py`](../../exercises/tools/run_benchmarks.py)。

```powershell
python tools/run_benchmarks.py --exe build/bench/benchmarks/Release/queue_bench.exe --check build/bench/Capstone2_lockfree_queue/Release/Capstone2_lockfree_queue_reference.exe --check build/bench/runtime_tests/Release/runtime_queue_history_test.exe --variant mutex --variant ring --output build/results/queue-storage-1 -- --size 100003 --producers 4 --consumers 4 --capacity 1024
python tools/run_benchmarks.py --exe build/bench/benchmarks/Release/queue_bench.exe --check build/bench/G3_spsc_ringbuffer/Release/G3_spsc_ringbuffer_reference.exe --variant spsc --variant spsc-cached --output build/results/queue-spsc-1 -- --size 100003 --producers 1 --consumers 1 --capacity 1024
python tools/run_benchmarks.py --exe build/bench/benchmarks/Release/queue_bench.exe --check build/bench/Q1_mpsc_queue/Release/Q1_mpsc_queue_reference.exe --check build/bench/Capstone2_lockfree_queue/Release/Capstone2_lockfree_queue_reference.exe --variant mpsc --variant mpmc --output build/results/queue-mpsc-1 -- --size 100003 --producers 4 --consumers 1 --capacity 1024
python tools/run_benchmarks.py --exe build/bench/benchmarks/Release/queue_bench.exe --check build/bench/Capstone2_lockfree_queue/Release/Capstone2_lockfree_queue_reference.exe --variant batch --output build/results/queue-batch-1 -- --size 100003 --producers 4 --consumers 4 --capacity 1024 --batch 8
python tools/run_benchmarks.py --exe build/bench/benchmarks/Release/queue_bench.exe --check build/bench/Q2_ms_queue/Release/Q2_ms_queue_reference.exe --variant ms --output build/results/queue-ms-1 -- --size 100003 --producers 4 --consumers 4 --capacity 0
```

每个 output 必须是新的目录，不能覆盖旧样本。默认每版本暖机一次、正式五个独立进程样本，固定种子打乱同组版本顺序，外部超时限制整个子进程。查看 run.json 中所有 samples_ms、median_ms、min_ms、max_ms 与环境字段，保留 samples.csv；不要只选最好的样本或静默排除离群值。这里给的是可复现方法，不预设哪个版本一定加速。

修复前 Windows x64 上的 40 个样本、环境与当时的 Release/ASan 结果保留在[作者验证记录](VALIDATION.md)，已明确标成旧实现快照。独立 review 随后发现 bool 压位存储与可变源赋值重载问题；修复改变了算法头版本，旧耗时不能作为当前实现的性能证据。修复后的回归、构建、基准接口冒烟及源码哈希在该记录另节列出，等待原非作者复验；本轮不从旧样本推算新实现加速比。

仓库还保留了 2026-09-08 的最终样本目录：`references/measurements/final-20260908/queue-storage`、`queue-batch`、`queue-spsc`、`queue-mpsc`、`queue-ms`。这些文件不是作者本机 build 目录里的临时日志，读者可以直接核对 run.json 与 samples.csv；但它们绑定的是当时的源码 hash，不能自动继承到后续头文件修订。

修订期间新增的 [`queue_diagnostics.cpp`](../../exercises/benchmarks/queue_diagnostics.cpp) 用于样章自检：它统计公开接口调用数、成功调用数、完成元素数、诊断宏开启下的 mutex 进入和 SPSC 远端下标 load，并用单线程热区 `operator new` 计数隔离预分配差异。它不是正式 benchmark，也不观察等待时间、CAS 失败或 cache miss。完整修订采样命令和归因边界见[队列修订证据口径](../performance/c08-revision-queue-evidence.md)。

## 自测与答案

**为什么 history 能接受消费者先返回较晚元素？** 只要调用区间重叠，搜索可以把先取得前项但晚返回的调用排在前面。FIFO 约束的是合法线性化次序，而不是打印或返回的全局顺序。

**允许 false 的模型是否太弱？** 它刻意匹配本课程原子环的失败契约，成功数据的唯一性、值与顺序仍检查。严格队列必须使用严格模型；反例检查确保没有把两者混淆。

**warmup 与正式样本都在一个进程里做会怎样？** 会改变分配器、缓存与进程状态，并偏离统一实验设计。本 driver 不做这种内部重复，由外部 runner 管理独立进程。

**通过 ASan 或一次历史搜索是否证明正确？** 不能。工具只检查实际执行或有限模型覆盖范围；安全性仍需保护协议、发布关系、代次前提与独立审查共同支撑。

**看到原子 is_lock_free 为 1，可以把 MS 称作完整无锁吗？** 不可以，分配器与 HP 域锁都没有被这个查询覆盖。CSV details 直接记录这一区别，避免把进展保证压成一个布尔值。
