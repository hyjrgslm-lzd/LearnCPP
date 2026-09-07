# 14 并行算法：先允许重排，再承担约束

标准库执行策略改变元素函数可以如何执行；它不是创建固定数量线程的命令。先读[四种策略与异常合同](../topics/performance/02-execution-policies.md)，再读[归约、扫描与结合方式](../topics/performance/03-reduce-scan.md)。两篇分别对应 [L1](../exercises/L1_par_algorithms/README.md)、[L2](../exercises/L2_parallel_reduce_scan/README.md)，每个必做 Part 都在本题 `solution.cpp` 中有可运行检查。

[L3](../exercises/L3_par_vs_seq_bench/README.md)把轻映射与可独立验证的重多项式负载接入统一基准。[跨三个规模的连续实验](../topics/performance/05-light-heavy-scaling.md)完整解释输入域、闭式 oracle、算术强度、首次调用成本及五样本结果；先完成[测量先修](12-measurement.md)再运行。最后在 [Cap3](../topics/performance/04-parallel-compute.md) 中，把布局、输出所有权、归约与排序连接成一个完整项目。

本章默认 C++23，规范引用固定 N5050。浮点加法不满足结合律；`reduce(seq)` 也不是 `accumulate` 的左折叠保证；scan 保持操作数顺序但可改变括号。这三点将在真实代码中分别验证，不能用“允许一点误差”一句话替代推导。
