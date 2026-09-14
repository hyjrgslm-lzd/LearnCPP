# C13 知识与应用覆盖

本表是可复用的学习导航，不记录某次机器的通过状态。实际命令和结果留在本机 build 目录。

| 下游能力/问题 | 主讲 | 实际入口 | 检查与解释 |
|---|---|---|---|
| 优化前判断数值误差 | [浮点](../chapters/02-floating-error.md) | L01、numerics.hpp | 解析抵消真值，区分 naive/Kahan/Neumaier |
| 稳定且有范围的实验输入 | [随机](../chapters/03-random-inputs.md) | L06、layout.hpp 生成器 | 同 seed、分布范围、借用视图；不承诺所有分布跨 STL 逐位一致 |
| 单位错误不进入热循环 | [单位](../chapters/04-units.md) | L10、L04 时间边界 | 16 ms→0.016 s，实际 mp-units consumer |
| 矩阵和数据的借用映射 | [span/mdspan](../chapters/05-span-mdspan-layout.md) | L03、L04、F01 | 原生 mdspan、非拥有、shape/stride 和存储要求 |
| 改变字段排列的成本 | [AoS/SoA](../chapters/06-aos-soa.md) | L02 | 字段转换、完整尾部、错误前无写入、转换与内核分开计时 |
| 线代和分块实现 | [矩阵](../chapters/07-matrix-tiling-linalg.md) | L03、L11 | 非方阵、尾块、混叠拒绝、真实 stdBLAS 算法 |
| SIMD 与执行策略 | [CPU并行](../chapters/08-simd-parallel.md) | L07、C08 既有内核 | 标量/SSE2/xsimd、哨兵重置、独立期望与线程数边界 |
| 判断实际数据放置 | [NUMA](../chapters/09-numa.md) | L08、C08 numa.hpp | 拓扑与页面驻留分开，不把亲和性请求当实际页面位置 |
| 从输入到结果的完整成本 | [综合](../chapters/10-particle-pipeline.md) | L04、L05 | 完整坐标与统计；区分内核、阶段、任务和进程 |
| 能解释上游实际实现 | [源码](../chapters/11-sources-and-frontier.md) | L10、L11、F01 | 固定版本、入口→类型/策略→循环→结果 |
| 定位缓存、分支、分配 | [成本](../chapters/12-cache-branches-allocation.md) | L05/costs.cpp、L02、C08 | 分配次数/请求字节、访问顺序、特殊浮点值；不臆测硬件事件 |
| 可复现的测量入口 | [测量](../chapters/01-measurement.md) | C08 benchmark.hpp、各观察程序 | 时间窗、原始行、有效完成量、独立正确性检查 |

既有 C08 内容不搬走：共享状态和内存模型仍由 C08 主讲，CPU 实验被明确复用。C14 接收数值、布局和归因基础后继续设备侧机制；本课不冒充 CUDA 或跨设备验证。
