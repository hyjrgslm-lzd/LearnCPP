# F3 最终实验判读

2026-09-11，Ubuntu Clang 18.1.3、WSL2、Ryzen 9 9900X；同一源码，`-std=c++23 -O2 -DF3_UNINSTRUMENTED_HALO`。构建、IR、汇编命令与退出码保存在同目录 JSON；[summary](summary.json)保存输入、产物 SHA-256 与复算统计。可执行文件只保留在忽略的 build 目录。

## 按实际调用路径判断

检查[完整 IR](f3-uninstrumented.ll.txt)和[完整汇编](f3-uninstrumented.s.txt)：

- `_Z13local_consumei` 与 `_Z15escaped_consumei` 均没有动态分配调用，均已化简为向量化求和与标量尾循环。前者向量归纳使用 i32 后扩展，后者使用 i64，循环控制也不同。
- 独立导出的 `_Z12range_valuesi` 仍调用 `_Znwm` 分配 48 字节；两条实际测量的 consumer 不调用这个独立函数。因此，仅搜索整个 IR 是否含 `operator new` 会得出错误结论。
- `bench_ns` 的计数循环每次仍通过传入函数指针调用 consumer 并累计 checksum，没有把全部 iterations 的计算提到循环外。计时结束后再核验 aggregate sink。
- `-Rpass=coroutine-elide` 没有输出 remark。我们观察到的是两条 consumer 的动态分配被消除及计算被化简，不能指定是哪一个优化 pass 的功劳，也不能证明 B 的全局地址写入阻止了 HALO。

这修正了旧作者报告根据模块中仍存在分配调用而写的“未观察 HALO”。旧原始记录保留，但当前结论以上述调用路径证据为准。

## 采样结果

每版 1 个独立预热进程（1000 iterations），随后每版 5 个独立测量进程（各 200000 iterations）；seed=20260911 打乱 A/B 顺序。两组分别检查总和，无隐藏预热。

| n | local median [min, max] ns/iteration | escaped median [min, max] ns/iteration | 每个测量进程 sink |
|---|---|---|---|
| 32 | 2.660 [2.593, 2.747] | 2.237 [2.217, 2.558] | 99200000 |
| 256 | 34.714 [34.670, 35.413] | 26.161 [23.876, 30.881] | 6528000000 |

所有原始样本与命令见 `f3-final-n32-*.json`、`f3-final-n256-*.json`。escaped 在本次样本更快，但两版都无动态分配，且生成循环不同；不能将差值归因于“一版发生 HALO，另一版没有”，也不提供普适倍率门槛或平台排名。

## 干扰和局限

本轮采样期间没有启动本任务的并行构建。Windows 快照在采样前记录了 cl/cmake/MSBuild，采样后仍有 cmake/MSBuild，说明其他构建干扰未排除；未追溯这些进程属于哪个任务。WSL 端点快照未见编译器，但系统服务仍在运行。端点快照也不能覆盖整个期间。没有绑核或锁频。`CLOCK_MONOTONIC` 报告分辨率 1 ns，但它不是采样误差界，也不等于每次 consumer 都单独读钟：这里测批量总耗时再除以 iterations。插桩 allocation hook 的 fixture 与无插桩性能/IR 证据分列。
