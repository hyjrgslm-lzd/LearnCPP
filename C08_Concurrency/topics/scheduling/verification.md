# NUMA / 调度专题：验证口径

本页保留调度与 NUMA 单元的复查口径。构建目录、原始 runner 输出、文件指纹和作者复测记录属于过程产物，不作为课程源码提交。

## 正确性复查

M1/M2 的 main 是观察入口，成功只覆盖本程序实际运行的场景；完整结果通道、关闭、递归、拒收和窃取结论来自各自 Reference 与 runtime 检查。J3/N1 是平台诊断探针，成功只说明本机实际 CPU、亲和性和页面证据满足当前检查。

建议复查顺序：

```powershell
cmake --preset verify-core
cmake --build --preset verify-core --target M1_work_stealing_pool_reference M2_execution_bridge_reference J3_numa_concept_reference N1_numa_placement_reference
ctest --preset verify-core -R "M1|M2|J3|N1" --output-on-failure
```

N1 在单 NUMA 节点机器上可以返回 77：local、first-touch、parallel-init 能验证本地页面；remote、interleaved 和跨节点 shared-read 需要真实多节点拓扑。CTest 的 “0 failed” 不能写成“所有 NUMA 条件已验证”。

## 基准复查

`scheduling_bench` 和 `numa_bench` 都由 `tools/run_benchmarks.py` 驱动。每个进程只输出一个 variant 的统一 CSV；Reference 检查先运行，失败样本不进入统计。输出目录使用 `build/measurements/...` 之类的本地目录，并保持全新，避免覆盖旧记录。

调度样本只描述指定 size、threads、variant 和本机负载；不能推出某个策略普遍更快。计时包含分配、创建执行资源、调度、计算和 join。

NUMA 样本只在有真实页面放置证据的 variant 上有效。remote/interleaved 缺硬件条件时应报告 PARTIAL_SKIP 或 77，不得填入伪造的有效 CSV。两次 CPU/页观测只是端点证据，不是连续迁移轨迹；没有页锁定、硬件计数器或“零 page fault”保证。

## 明确边界

- Windows 多 processor group 完整拓扑与复用线程 affinity 恢复不在本教学实现范围。
- Linux 分支使用内核接口，不增加 libnuma 链接依赖；未在本机运行时不能写成 Linux 通过。
- M1 无 NUMA 感知 victim 选择，无锁 Chase-Lev 未实现；按节点分片是 N1 的独立场景。
- M2 验证固定 stdexec，不把库实现通过写成标准库 C++26 全支持。
