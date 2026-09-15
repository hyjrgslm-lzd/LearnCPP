# A1 two executions: algorithm policy vs sender graph

本题用同一组整数记录比较两种 execution。`std::execution::par` 附在标准算法调用上，库选择怎样分块、是否真的并行；sender 版本把四个分支和合流点显式写进图里。这里不测性能，也不谎称线程数。后端可能顺序执行，结果仍必须正确。

## Part 1: 独立 oracle

先写普通循环：偶数平方求和，同时数偶数个数。它是数值 oracle，不依赖并行 STL，也不依赖 sender。

## Part 2: 并行算法版本

用 `std::transform_reduce(std::execution::par, ...)` 表达“算法允许并行”。如果本机标准库没有实际并行后端，这只是允许，不是保证。

## Part 3: sender 图版本

把输入切成四段：`[0,c) [c,2c) [2c,3c) [3c,n)`。旧版本把最后一段从 `2c` 开始，导致第三段和第四段重叠。checker 会记录每个下标被访问次数，0、1、非整除、大输入都必须恰好一次。

每段返回 `{sum, even_count}`，`when_all` 合流后再相加。`when_all` 表达四个结果汇合，不自动保证创建四条线程。

## Part 4: 解析

并行策略版把“怎么执行”交给算法实现；sender 版把“哪些工作并列、在哪里合流”暴露为图结构。一个是算法策略，一个是工作描述和完成通道。
## IDE 工程入口

VS solution 中本题主入口是 `A1_two_executions_student`。学生只编辑 `src/student/solution.hpp`；`main.cpp` 是共同检查器，Reference 在 `src/reference/solution.hpp`，good/bad 控制在 `validation` 下。`A1_two_executions` 聚合目标只负责显式构建学生目标，收在 Support；Reference 与控制目标保留为独立项目，用来区分答案、正确对照和错误拒绝。单题可用 `cmake -S <本目录> -B <build>` 独立生成。
