# 15 SIMD：从一个可信结果到多通道实现

SIMD 在一个线程内并行处理多个数据通道，与多线程划分任务可以组合。本章先确定每个输入应该得到什么结果，再研究如何同时计算若干个结果，以及这样做是否真的改善了瓶颈。

依次阅读四篇完整正文：

1. [标量 oracle、循环与 AoS/SoA](../topics/simd/01-scalar-and-layout.md)：输入域、别名、数据表示和自动优化基线。
2. [显式向量、对齐、尾部、掩码与索引](../topics/simd/02-explicit-vectors.md)：SSE2、可选 xsimd、N5050 原生接口，以及不能被 mask 挽救的无效访问。
3. [点积、水平归约、FMA 与数值精度](../topics/simd/03-reductions-and-precision.md)：独立真值、运算相关误差界与特殊值策略。
4. [诊断、ISA 回退和带宽瓶颈](../topics/simd/04-diagnostics-and-bandwidth.md)：证明实际生成了什么代码，再解释测量。

练习入口为 [K1](../exercises/K1_simd_basics/README.md)、[K2](../exercises/K2_simd_dotproduct/README.md)、[K3](../exercises/K3_simd_where_select/README.md)。默认不安装 xsimd 也能编译并检查基线；x64 上 SSE2 是实际显式实现。原生 `<simd>` 分支受 `CS_HAS_STD_SIMD` 控制，本机探测缺失时明确跳过，不能把第三方实现的结果记成标准库验证。
