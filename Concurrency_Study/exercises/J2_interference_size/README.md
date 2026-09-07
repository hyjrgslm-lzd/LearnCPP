# J2：对齐、数组步长与硬件干扰提示

本题 main.cpp 分类为 OBSERVATION：运行给定基线并进行本程序实际列出的观察/检查。退出成功只说明这些检查通过，不表示下面全部实现、推导或测量 Part 已完成；完整答案与更广检查见独立 solution.cpp。需要实现的 Part 请在自己的函数中完成后对照 Reference，不把运行答案视为完成作业。

完整连续正文：[课程正文](../../topics/performance/01-cache-layout.md)。

本题使用 [J1/reference.hpp](../J1_false_sharing/reference.hpp) 的真实类型，避免打印的布局与基准布局不同。main 观察 sizeof/alignof 提示，[solution.cpp](solution.cpp) 给出地址与步长检查。

## 必做 Part 与答案

1. 读取 destructive/constructive 常量与特性宏。实现未提供时回退 64，并标记为教学假设，不称为硬件探测。标准常量是编译期实现提示，不能说明真实 CPU 绑定。
2. 比较“只对齐容器起点”和“把数组元素类型对齐”。packed 的内部步长仍是 atomic 大小；padded 类型的 sizeof 包含为后续数组元素保持对齐所需的填充。Reference 检查两个元素起点都满足提示对齐、差值等于 sizeof 且不少于 destructive。
3. 观察 packed 两个地址是否落在同一个提示区域。程序打印 same_hint_region，而不把它直接命名为 same_physical_cache_line。真实一致性粒度需平台证据。
4. 解释 hot_record 的紧凑布局。一起读取的字段可能受益于相邻放置；对每个字段都 padding 会增大工作集。constructive 不意味着“应该制造多线程对一个可写对象的争用”。

## 答案中的边界

该类型只供单个课程目标使用。若放进公共 ABI，编译目标改变提示尺寸可能改变成员偏移；应把布局纳入库的固定 ABI 约定。默认 MSVC C4324 提示结构因对齐而填充，是本实验要观察的行为，不能为消除该告警把 alignas 删掉。

本题是布局正确性观察，不计时、不声称已实测缓存行弹跳。性能使用 J1 的同一实现和 layout_bench。

## 构建与运行

从 `Concurrency_Study/exercises` 执行：

```powershell
cmake -S J2_interference_size -B build/J2_interference_size -G "Visual Studio 18 2026" -A x64
cmake --build build/J2_interference_size --config Release
ctest --test-dir build/J2_interference_size -C Release --output-on-failure
```

C++ 默认 23，cs::check 在 Release 中保持有效。可选能力缺失不阻止普通基线；平台及标准事实的官方链接、完整推导见本题对应正文。
