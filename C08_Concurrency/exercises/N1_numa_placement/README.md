# N1：把 NUMA 放置请求变成可验证的页面证据

先完成 [J3](../J3_numa_concept/README.md)，再读[页面放置正文](../../topics/numa/03-placement.md)和[节点分片/共享](../../topics/numa/04-sharding.md)。[main.cpp](main.cpp) 运行 16 页本地观察；[solution.cpp](solution.cpp) 是完整 Reference，公共算法在 [numa.hpp](../include/concurrency_study/numa.hpp)。默认 C++23，Windows 链接 Psapi，Linux 使用内核 syscall。

入口类型：main 是已实现的本地页面诊断探针，不是未填算法 Starter。它启动时明确输出 PLATFORM PROBE ONLY，0 仅证明所运行的 local 16 页检查通过；不代表远端、交错或跨节点 shared-read Part 已完成。完整 Reference 逐 Part 汇总，缺任一要求仍返回 77。两者都不 include solution.cpp 来冒充学生实现。

## 构建与成功含义

从 `C08_Concurrency/exercises` 的统一构建运行：

```powershell
cmake --build build/full-windows --config Release --target N1_numa_placement N1_numa_placement_reference
ctest --test-dir build/full-windows -C Release -R N1_numa_placement_reference -V
```

叶项目 CMake 由主线程注册。0 表示全部请求方案均获得完整证据并通过正确性检查；77 表示硬件、平台或放置证据不足，日志保留已完成的本地检查；1 表示输入或数据检查失败。单节点不能伪造 remote 数字。API 成功但实测页面不在请求节点，也属于无法验证该方案，不能输出有效排名。

Reference 逐 Part 捕获并报告 PASS/SKIP/FAIL，继续可运行的检查；最终 FAIL 优先返回 1，否则存在任一 SKIP 返回 77。特别地，一个 reader 所在 node 加两个 memory node，不能满足跨节点 shared-read。model_checks 在平台探测前验证这个模拟拓扑的状态合并，以及 Linux 循环相位模型；模型通过不表示对应硬件实测通过。

## Part 1：未触页、初始化与驻留查询

Reference 为每种方案新建 128 页虚拟内存。记录 before-touch，写入每个 word 为 1，记录 after-touch；用 QueryWorkingSetEx 或 move_pages 逐页读取实际 node。

答案：虚拟分配/commit 不等于每页已经驻留，首次写入常触发物理后备分配。vector(n) 预初始化会污染 first-touch 实验。未驻留页标 -1，不能读取无效 Node 字段当证据。页大小来自 OS，Linux 禁用本映射的透明大页，Windows 不请求大页并检查 LargePage 标志。页范围和总大小以实际基础页为单位。

## Part 2：local 与 remote

保持 reader CPU、内存大小、初始化线程、读取次数和计时边界相同，只把首选 memory node 从本地改到另一个允许节点。要求实际每一页都匹配预期。

答案：VirtualAllocExNuma/MPOL_PREFERRED 是首选，不是保证；必须验证。只有一个允许 memory node 时，在完成本地检查后返回 77。即使有 remote 条件，也不要求 remote 比 local 慢，顺序扫描可能命中缓存，启动成本也可能占主导。

## Part 3：串行 firsttouch 与 parallel-init

两版保留相同 reader 集合和页分片。firsttouch 由第一个 CPU 初始化全部页；parallel-init 由各 reader 初始化自己的完整页范围。Reference 查询并比较每一页的预期 node，然后验证全部结果。

答案：改变的是初始化责任及其留下的页分布，正式读取量不变。按完整页分片避免两个初始化者争同一页；有多个 node 时选两个 node，无条件时尽量选同 node 不同 core。单 node 上仍能验证两版覆盖相同数据，但不能推断远端性能改善。

## Part 4：interleaved

在两个允许 memory node 间按页交替，reader 保持一个。Linux 使用 mbind interleave：按排序 node 集合验证允许固定循环移位的完整逐页序列，并保存该相位供后续快照比较。Windows 每页以 nullptr 地址新建 RESERVE|COMMIT，在排序后的 node 间交替请求首选节点；对已 reserve 地址单独 commit 会忽略首选 node，因此旧方案不可用。

答案：交错可能分散 home 内存控制器负担，但单 reader 仍访问远端部分。Windows 五个 variant 全部改为同样的独立页指针布局，避免只让 interleaved 承担碎片化、TLB 和指针扫描成本；日志记录页大小、reservation 数及分配粒度，部分分配失败逐块释放。Linux 连续 VMA 与该 Windows 布局不同，不能直接跨平台排名。纯模型检查双节点奇相位、反向请求加奇相位、三节点顺序及错误逐页排列，不能只排序或比直方图。多节点 Windows 和 Linux API 分支仍未实测。

## Part 5：共享不可变页面

solution 的 shared_read 将 32 页固定在 home node，初始化后选择至多两个不同 node 的 reader，让每个 reader 读取同一完整范围。记录读前/读后页分布和每个 reader 的实际 CPU，并分别校验总和。

答案：多个 reader 共享不可变数据没有写写/读写冲突，但可能有远端流量；page home 不等于每次读取都到 DRAM。这个场景每 reader 读全量，与分片总计只读一遍的工作量不同，不加入同工作量排名。单节点只做本地共享读取协议检查，说明缺少跨节点 reader。

即使有两个 memory node，只要可用 reader 未覆盖两个 CPU node，Part 5 仍标 SKIP，并参与最终 77 汇总，不被其他成功 Part 覆盖。

## Part 6：计时解释和答案

基准 [numa_bench.cpp](../benchmarks/numa_bench.cpp) 支持 `--variant local|remote|interleaved|firsttouch|parallel-init --size 字节数`。stdout 仅统一 CSV，拓扑、CPU 与页分布送 stderr。正式扫描一次；预热/独立进程采样由公共 runner 管理。

runner 的 PARTIAL_SKIP 默认返回 77；明确使用 `--allow-partial` 时才允许已有有效样本的部分结果返回 0，报告仍保留未测条件。本次修订使用新布局重新采样，旧连续布局数字不作为新版结果。

答案：计时排除分配、初始化触页、一次预读、页查询和正确性检查，包含 reader 创建、affinity 设置、扫描和 join。它排除了显式初始 fault 阶段，未证明正式区间绝无 fault；前后页面快照也未证明期间没有迁移。小规模毫秒数不能当作 DRAM load latency。比较 local/remote 时不改变 CPU；比较 firsttouch/parallel-init 时不改变 reader 集合。

作者实测记录见[专题验证记录](../../topics/scheduling/verification.md)。独立 review 在后续批次进行，本文不预先标记独立验收通过。
