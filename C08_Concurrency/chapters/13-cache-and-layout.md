# 13 缓存与数据布局

先完成[测量先修](12-measurement.md)。本章从“线程各写自己的变量，为何仍可能互相拖慢”开始，把语言层的数据竞争与硬件层的一致性流量分开，再比较相邻布局、分散布局、真共享与线程局部批量发布。

完整连续正文是[缓存、共享和布局对照](../topics/performance/01-cache-layout.md)。请按正文顺序运行 [J1](../exercises/J1_false_sharing/README.md) 和 [J2](../exercises/J2_interference_size/README.md)，检查地址、尺寸、每个线程的完成量，最后进入 `layout_bench`。不以任何特定加速倍数作为验收。

接下来有两条延伸。一个核内的连续数据访问与 AoS/SoA 重排见 [SIMD 标量与布局起点](../topics/simd/01-scalar-and-layout.md)；多核如何分配不重叠输出见[并行算法](14-parallel-algorithms.md)。NUMA 是 CPU 放置和物理页面放置的另一层问题，进入独立作者的 [NUMA 正文](../topics/numa/01-topology.md)；本章不把 padding 当作 NUMA 放置。
