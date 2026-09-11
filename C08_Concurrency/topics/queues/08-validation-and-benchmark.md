# 队列 08：正确性检查与基准方法

队列性能必须建立在正确性检查之后。一个错误队列丢元素时可能“更快”；一个只统计 push/pop 次数的检查也可能漏掉重复、缺失和越界。本篇只保留复查方法，原始样本、`run.json`、`samples.csv` 和诊断 CSV 属于运行产物，不提交到课程源码。

## 正确性层

[`queue_checks.hpp`](../../exercises/include/concurrency_study/queue_checks.hpp) 的 transfer 为每个生产者划定连续 ID 区间。消费者只写自己的结果容器；join 后合并、排序，并检查第 `i` 项等于 `i`。成功总数相等只是第一步，逐项相等才能发现重复、缺失和越界。

[`queue_history_test.cpp`](../../exercises/runtime_tests/queue_history_test.cpp) 对有限数量的实际操作记录 begin/end、类型、返回值和元素。若 A.end 小于 B.begin，合法串行解释必须把 A 放在 B 前；区间重叠则尝试可行顺序。不同队列使用不同模型：严格 FIFO、允许暂时失败的有界队列、LIFO 或无界 FIFO。不能为了让历史通过就忽略成功取值顺序。

## 基准层

[`queue_bench.cpp`](../../exercises/benchmarks/queue_bench.cpp) 一个进程只测一个具体 variant，stdout 只有公共 CSV 表头和一行数据。错误写 stderr；没有 `all` 版本，没有内部隐藏重复。外部 runner 负责 warmup、五次独立进程样本、固定 seed、超时和全新输出目录。

| variant | 合法线程角色 | 容量与额外参数 |
|---|---|---|
| mutex | 任意正数 P/C | 正容量；batch=1 |
| ring | 任意正数 P/C | 正容量；batch=1 |
| batch | 任意正数 P/C | 正容量；batch 为正数 |
| spsc / spsc-cached | P=1、C=1 | 正容量；batch=1 |
| mpsc | 任意正数 P、C=1 | 容量至少 2 且为二次幂；batch=1 |
| mpmc | 任意正数 P/C | 同上；batch=1 |
| ms | 任意正数 P/C | capacity=0 表示无界；batch=1 |

计时从 transfer 调用前开始，到所有线程 join 并聚合成功计数后结束。队列构造与最终析构在区间外；节点分配、hazard pointer 获取、退休和热路径扫描在区间内。`completed` 表示成功传输元素数，吞吐可用 `completed * 1000 / milliseconds` 得到元素/秒，不再乘二。

## 可比较范围

- `mutex` 与 `ring`：相同容量、相同 P/C、单元素严格 try，用于研究预分配存储影响。
- `ring` 与 `batch`：同一存储结构，改变调用粒度；吞吐提升可能伴随延迟与公平性变化。
- `spsc` 与 `spsc-cached`：P=1、C=1 的角色所有权场景。
- `mpsc` 与 `mpmc`：P 多、C=1 时比较分支成本；P=1、C 多是另一种拓扑，不等于专用 SPMC。
- `ms`：无界节点队列，必须单独标明 capacity=0，并包含实际节点分配与回收成本。

mutex 与 sequence 队列的单次 false 语义不同。若放在同一图表，只能声明它们完成了同一批传输工作，不能声明提供相同 try 契约。

## runner 示例

从 `C08_Concurrency/exercises` 执行，路径按实际构建目录替换：

```powershell
python tools/run_benchmarks.py --exe build/bench/benchmarks/Release/queue_bench.exe --check build/bench/Capstone2_lockfree_queue/Release/Capstone2_lockfree_queue_reference.exe --check build/bench/runtime_tests/Release/runtime_queue_history_test.exe --variant mutex --variant ring --output build/results/queue-storage-1 -- --size 100003 --producers 4 --consumers 4 --capacity 1024
python tools/run_benchmarks.py --exe build/bench/benchmarks/Release/queue_bench.exe --check build/bench/G3_spsc_ringbuffer/Release/G3_spsc_ringbuffer_reference.exe --variant spsc --variant spsc-cached --output build/results/queue-spsc-1 -- --size 100003 --producers 1 --consumers 1 --capacity 1024
python tools/run_benchmarks.py --exe build/bench/benchmarks/Release/queue_bench.exe --check build/bench/Q1_mpsc_queue/Release/Q1_mpsc_queue_reference.exe --check build/bench/Capstone2_lockfree_queue/Release/Capstone2_lockfree_queue_reference.exe --variant mpsc --variant mpmc --output build/results/queue-mpsc-1 -- --size 100003 --producers 4 --consumers 1 --capacity 1024
python tools/run_benchmarks.py --exe build/bench/benchmarks/Release/queue_bench.exe --check build/bench/Capstone2_lockfree_queue/Release/Capstone2_lockfree_queue_reference.exe --variant batch --output build/results/queue-batch-1 -- --size 100003 --producers 4 --consumers 4 --capacity 1024 --batch 8
python tools/run_benchmarks.py --exe build/bench/benchmarks/Release/queue_bench.exe --check build/bench/Q2_ms_queue/Release/Q2_ms_queue_reference.exe --variant ms --output build/results/queue-ms-1 -- --size 100003 --producers 4 --consumers 4 --capacity 0
```

每个 output 必须是新目录。保留所有样本、失败和异常值；不要只选最好样本。通过 ASan 或一次有限历史搜索不证明所有交错正确，仍需保护协议、发布关系、代次前提和代码审查共同支撑。

## 诊断入口

[`queue_diagnostics.cpp`](../../exercises/benchmarks/queue_diagnostics.cpp) 用于样章自检：统计公开接口调用数、成功调用数、完成元素数、诊断宏开启下的 mutex 进入和 SPSC 远端下标 load，并用单线程热区 `operator new` 计数隔离预分配差异。它不是正式 benchmark，也不观察等待时间、CAS 失败或 cache miss。
