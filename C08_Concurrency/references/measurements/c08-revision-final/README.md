# C08 本轮正式队列采样

五组均PASS：8个variant，每variant一次预热＋五次独立进程正式采样，seed=42；共40次正式、8次预热、12次前置正确性进程。各组run.json保留完整argv、stdout/stderr、状态、环境、源码/二进制摘要及所有样本；samples.csv只含正式有效样本，没有删除离群值。

本机为Windows11、AMD Ryzen9 9900X（12核/24逻辑CPU）、MSVC19.51/STL202604、CMake4.2.3、Python3.13.11。使用最终Release queue_bench，未开启诊断插桩、Sanitizer或fast-math。主线程在本任务其他构建/测试及文件写入暂停后串行运行；没有控制整个操作系统的后台负载、CPU频率或亲和性。窗口和总命令记录见[窗口](../../validation/c08-revision/measurement-window.json)、[总运行](../../validation/c08-revision/final-measurements-run.json)。

## 原始样本汇总

单位毫秒。每行5个独立进程，下面的范围与标准差不是置信区间，也不是固定加速保证。

| 数据组 | variant | 中位数 | 最小–最大 | 样本标准差 |
|---|---|---:|---:|---:|
| queue-batch | batch | 9.4782 | 7.7606–11.0680 | 1.2272 |
| queue-mpsc | mpmc | 8.0734 | 7.2294–9.3230 | 0.8739 |
| queue-mpsc | mpsc | 7.7030 | 6.7382–16.7129 | 4.1749 |
| queue-ms | ms | 50.3672 | 43.9556–58.4877 | 6.2716 |
| queue-spsc | spsc-cached | 1.3730 | 1.2705–1.8191 | 0.2213 |
| queue-spsc | spsc | 1.2527 | 1.2044–1.5345 | 0.1307 |
| queue-storage | ring | 21.8832 | 19.0096–27.0526 | 3.0145 |
| queue-storage | mutex | 23.4159 | 16.0466–27.7353 | 5.5194 |

所有组size=100003。storage和batch均P=3/C=4、capacity=64；batch的批量为8，独立解释前缀事务与等待代价。SPSC组P=1/C=1，MPSC/MPMC组P=3/C=1，容量64。MS组P=3/C=4、无界capacity=0，包含分配与HP锁成本。各组的顺序、失败及进展保证以details为准，不跨组混排。

## 可以得出的结论

- 同一单元素域内，ring中位数低于mutex，但范围明显重叠；这五个样本不支持普遍或稳定的预分配加速。诊断阶段记录容器热区operator-new调用差异，只证明那个命名成本发生变化。
- spsc-cached本轮中位数高于spsc。先前真实远端下标load计数较少，仍不保证端到端更快；保留此负面结果，不据此排除其他负载下的缓存方案。
- batch单列接口粒度变化，不能从这组数字推断低流量尾延迟、公平性，或与不同契约相除得到通用加速比。
- MPSC/MPMC样本范围重叠，MPSC较慢的一轮原样保留；不从总耗时猜测某次CAS、调度或缓存事件是根因。MS包含不同容量、分配和回收责任，不加入有界队列排名。
- 计时包含线程创建、传输和join，排除最终对象析构，完整边界保留在每条details。吞吐只能按实际完成量计算；未测硬件事件，不把源级计数解释成cache miss或内核切换次数。

## 版本与复现

[本轮诊断与推进依据](../../../topics/performance/c08-revision-queue-evidence.md)和[样章独立复验](../../validation/c08-revision/reviews/queue-sample-review-r2.md)先于正式采样。历史final-20260908数据保留在原目录，机器与快照不同，不与本轮计算跨历史加速比。

从各run.json的runner_command复跑，替换为自己的可执行文件与全新output目录即可。源码和可执行文件在组内不得变化。runner的source_sha256描述当时规则包含的文件集合；测量后补写报告会改变整树摘要，不回写旧样本来迎合新报告。独立数据审查见[测量审查](../../validation/c08-revision/reviews/measurement-review.md)。
