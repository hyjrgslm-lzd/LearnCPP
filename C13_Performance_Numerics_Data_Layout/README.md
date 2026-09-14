# C13 性能工程、数值与数据布局

面向已有现代 C++ 基础的工程师，从“算对同一件事”进入数据布局、CPU 测量和完整任务成本。主案例是一条三维粒子批处理管线：固定种子输入 → 单位转换 → 位置更新 → 矩阵变换 → 稳定归约。局部实验用于拆开浮点、访问方式、分配和执行策略的影响。

先修按单元取用：C02 对象与借用、C03 类型与错误、C04 泛型、C06 容器与算法、C07 内存；共享状态与并行部分再接 C08。读数值基础不要求先学完整并发课程。C08 的 CPU 实验是本课复用资产，C14 继续负责设备侧算法，不用 CPU 结论代替 GPU 证据。

## 阅读路线

| 顺序 | 正文 | 需要掌握的能力 |
|---|---|---|
| 00 | [课程地图](chapters/00-course-map.md) | 识别先修、接口和实验的不同责任 |
| 01 | [测量边界](chapters/01-measurement.md) | 计时范围、归因、汇编与向量化观察 |
| 02 | [浮点与稳定归约](chapters/02-floating-error.md) | 舍入、抵消、Kahan/Neumaier、误差预算 |
| 03 | [随机输入](chapters/03-random-inputs.md) | 引擎、分布、可复现范围和借用视图 |
| 04 | [量纲与单位](chapters/04-units.md) | 单位、quantity specification、点/差值与内核边界 |
| 05 | [span/mdspan](chapters/05-span-mdspan-layout.md) | extents、stride、layout、存储与借用 |
| 06 | [AoS/SoA](chapters/06-aos-soa.md) | 正确转换、完整尾部、转换成本与适用场景 |
| 07 | [矩阵与分块](chapters/07-matrix-tiling-linalg.md) | 朴素与分块 GEMM、尾块、混叠与数值检查 |
| 08 | [SIMD 与并行](chapters/08-simd-parallel.md) | 对齐、掩码、尾部、执行策略和实际执行位置 |
| 09 | [NUMA](chapters/09-numa.md) | 拓扑、亲和性、页面驻留与观测限制 |
| 10 | [综合粒子管线](chapters/10-particle-pipeline.md) | 所有权、完整输出、数值正确性与端到端成本 |
| 11 | [源码与前沿](chapters/11-sources-and-frontier.md) | 阅读 stdBLAS/mp-units，区分规范、实现与本机能力 |
| 12 | [缓存、分支、分配](chapters/12-cache-branches-allocation.md) | 用受控对照和计数缩小原因，不从总时间猜根因 |

数据/数值路线可先读 02—07，再回到测量与并行；性能路线从 01、06、12 进入，再补数值与综合项目。每章有自测解析，练习说明另列学生编辑位置和运行方法。

## 可运行单元

| 单元 | 形式 | 对应任务 |
|---|---|---|
| [L01 稳定归约](exercises/L01_stable_reduction/README.md) | 实现 | naive/pairwise/Kahan/Neumaier 与解析真值 |
| [L02 数据布局](exercises/L02_layout/README.md) | 实现＋对照 | AoS/SoA、尾部、错误时无部分写入 |
| [L03 分块矩阵](exercises/L03_tiled_matrix/README.md) | 实现 | 原生 mdspan、矩形 GEMM、形状与混叠 |
| [L04 综合管线](exercises/L04_particle_pipeline/README.md) | 综合实现 | 三维输出和稳定统计；完整任务计时 |
| [L05 阶段与成本](exercises/L05_measurement_observation/README.md) | 观察 | 二维独立场景、分配计数、步长与分支语义 |
| [L06 随机与视图](exercises/L06_random_views/README.md) | 实现＋观察 | 固定种子、分布和借用输入 |
| [L07 SIMD/并行](exercises/L07_simd_parallel/README.md) | 观察 | 调用真实 C08 scalar/SSE2/xsimd/执行策略 |
| [L08 NUMA](exercises/L08_numa/README.md) | 观察 | 默认拓扑；显式选择小型页面放置实验 |
| [L10 mp-units](exercises/L10_units/README.md) | 第三方 consumer | 实际单位转换和量纲约束 |
| [L11 stdBLAS](exercises/L11_stdblas/README.md) | 第三方 consumer | 实际 dot/matrix_product，参考 mdspan 隔离 |
| [F01 原生前沿](exercises/F01_native/README.md) | 能力＋主体 | C++26/29 六组接口分别探测 |

本课独立构建入口是 [BUILD_GUIDE](exercises/BUILD_GUIDE.md)。`full-windows` 构建包括固定第三方示例；`core` 默认离线。Student 初始明确失败，Reference/good 与故意错误 bad 分开；观察程序成功不表示完成了全部解释任务。

知识到实际入口的映射见 [覆盖表](references/coverage.md)，版本、原生与第三方边界见 [标准索引](references/standards-and-implementations.md)。默认运行有限，长测和跨节点性能实验显式选择；原生能力缺失不会被标成完整主体通过。
