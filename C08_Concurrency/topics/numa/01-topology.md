# NUMA 01：先画出实际可用的拓扑

并行循环只有一个地址 `data`，看起来所有线程都在访问“同一块内存”。但虚拟地址不告诉我们物理页属于哪个内存节点，也不告诉我们当前线程运行在哪个处理器上。在 NUMA 系统上，这两个位置共同影响访问路径。只增加 worker 数，而不追踪这两个位置，可能让更多执行者争用同一个内存控制器或跨节点互联。

本系列从 [J3](../../exercises/J3_numa_concept/README.md) 的 `discover()` 开始。真实实现集中在 [`numa.hpp`](../../exercises/include/concurrency_study/numa.hpp)，后面的亲和性、页分配、驻留查询和分片实验都复用它。先读本机探测表，再推导应如何放线程和数据；不要从机器商品名猜测 node 数量。

## 1. socket、core、逻辑 CPU 与 node 是四个问题

socket/package 描述处理器封装；core 是执行核心；一个 core 可以通过 SMT 暴露多个逻辑 CPU；NUMA node 描述操作系统呈现的内存接近性域。它们不是可互换的编号。

一颗处理器可以暴露多个 NUMA 节点；某个节点也可能主要提供内存而没有可运行 CPU。固件配置、虚拟机拓扑和操作系统策略可能改变可见结构。因此“一路机器就是一个 node”“两路机器就是两个 node”都不能作为实验前提。同理，CPU 0 和 CPU 1 可能是同一个物理核心的 SMT 兄弟，也可能不是，不能按奇偶编号直接推断。

J3 输出 `group,cpu,socket,core,node`。Windows 的逻辑 CPU 用 `(group,number)` 表示，不能把多个组的 number 合并成一个不带组号的索引；Linux 使用操作系统 CPU ID，本实现将 group 列记作 0，仅作为统一显示格式。判断 SMT 时要结合组、封装和核心信息，而不是只比较 core 数字。

NUMA 并不改变 C++ 数据竞争规则。两个线程无同步写同一个普通对象，仍然是数据竞争；它们是否在同一 node 与这个语言层面的结论无关。本地/远端是访问路径属性，不是同步原语。

## 2. 硬件上存在，不代表当前程序可以使用

`hardware_concurrency()` 是提示值，既不是进程配额，也不提供 CPU ID 列表。进程可能受到 affinity、CPU sets、容器 cpuset 或作业环境限制。若直接取 `0..N-1` 绑核，既可能选中不允许的 CPU，也可能让两个 worker 落在同一物理 core。

Windows 探测读取 CPU set 信息、当前线程 primary group 的 group affinity、进程 affinity，以及线程 selected CPU sets 或进程 default CPU sets。它还排除分配给其他进程的 CPU set。教学实现只列出 primary group 内与这些掩码相交的候选子集，随后由专用新线程验证绑定请求。`GetThreadGroupAffinity` 返回一个组的结构，不能证明原线程原本被限制在单组：Windows 11/Server 2022 的默认 affinity 可以跨组。discover 不修改调用者的 affinity，也不声称枚举了完整跨组范围。进程掩码为零等无法形成候选集合的情况报 77。

这个边界不表示 Windows 只能使用 64 个逻辑 CPU。它表示当前实验选择了一个能清楚报告的执行范围。输出中的 scope 必须随数据保存，不能把“当前组内找到一个 node”夸大成“整个服务器只有一个 node”。如果要研究跨组调度，应扩展组级可用集合、硬 affinity 和 CPU set 的交互，并在目标系统上逐组验证，不能只把掩码类型换大。

Linux 探测通过 `sched_getaffinity` 获取当前线程允许的 CPU，读取 sysfs 的 package/core/node 关系，再从 `/proc/self/status` 的 `Mems_allowed_list` 获取内存节点限制。CPU 的 allowed 集合与内存的 allowed 集合不同：容器可以允许某 CPU 运行，却限制可分配内存的 node。代码只有在 CPU 的本地节点也属于可用内存集合时才选择本地放置实验的 reader。

本实现的 Linux CPU 掩码使用固定 `cpu_set_t`；若系统需要更大的掩码并返回错误，报 77，不截断后假装完整。sysfs 或 `/proc` 不可读也给出原因。Windows 内存节点通过 NUMA API 枚举有可用内存的节点；实际 allocation 与页面查询仍要再次验证，因为“可用”不是预留承诺。

## 3. 怎样读一份真实输出

作者在 Windows/MSVC 19.51 的本次运行中观察到 group 0 的 32 个逻辑 CPU、一个 package、16 组 SMT 核心关系，以及 memory node 0。基础页为 4096 字节。J3 选出的前四个允许 CPU 在操作前后均与请求一致。这个记录只描述该次探测；不是所有桌面系统的模板，也不是跨组服务器验证。

例如输出的 `(0,0,0,0,0)` 与 `(0,1,0,0,0)` 共享相同核心身份，表示这次探测中的 SMT 兄弟。N1 在单 node 情况下优先选另一个 core，因此本次使用 `0:0` 与 `0:2`，而不是把 0 和 1 当成两个独立物理核心。实际策略仍由读取到的表决定。

单节点机器足以学习拓扑过滤、绑核和页面查询，也能验证并行初始化是否覆盖全部数据；它没有本地/远端对照条件。我们继续运行可验证部分，然后明确跳过跨节点实验，而不打印一组人为增加循环次数得到的“remote latency”。

## 4. 运行和记录

从 `C08_Concurrency/exercises` 执行：

```powershell
cmake -S J3_numa_concept -B build/j3 -G "Visual Studio 18 2026" -A x64
cmake --build build/j3 --config Release
ctest --test-dir build/j3 -C Release -V
```

Linux 可用默认生成器配置同一叶项目，再 `cmake --build build/j3 --config Release` 和 `ctest --test-dir build/j3 -V`。本课程已实现 Linux 分支，但当前作者验证环境是 Windows，未取得 Linux 运行证据；不要把源码中有 `#elif` 写成已通过 Linux 实测。

保存 scope、完整拓扑表、allowed memory nodes、基础页大小、系统版本和 affinity 限制。后续修改线程数时，从这个集合中选择，并明确先增加物理核心还是先使用 SMT 兄弟。前者和后者增加的执行资源不一样，不能统称为“翻倍核心数”。

## 自测与答案

**socket 0 是否意味着 node 0？** 不意味着。它们属于不同关系，必须查各自映射；本机恰好数字相同不构成通用规则。

**进程只允许 CPU 8 和 10，能否创建两个线程并绑定到 0 和 1？** 不能据线程数推断 CPU ID。应从允许集合选 8 和 10，再确认是否是不同物理核心和相同/不同 node。

**一个内存节点没有 CPU 就应从列表删除吗？** 不应。它仍可能作为远端内存目标，但不能为它凭空选择本地执行 CPU。本实验将 CPU 与可用 memory nodes 分别记录。

**为什么停驻 parked CPU 不直接当成永久不可用？** 停驻是电源/调度状态，可重新启用；硬限制与当前闲置状态不同。本代码记录这一范围说明，不把 parked 当成不存在。

## 官方入口

- [Microsoft：NUMA Support](https://learn.microsoft.com/en-us/windows/win32/procthread/numa-support)，CPU、节点和页面查询 API 的关系。
- [Microsoft：CPU Sets](https://learn.microsoft.com/en-us/windows/win32/procthread/cpu-sets) 及 [SYSTEM_CPU_SET_INFORMATION](https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-system_cpu_set_information)，用于理解组、core、分配状态等字段。
- [Linux 内核：NUMA Memory Policy](https://www.kernel.org/doc/html/latest/admin-guide/mm/numa_memory_policy.html)，cpuset 限制与内存策略分属不同机制。
- [libnuma 官方手册源码](https://github.com/numactl/numactl/blob/master/numa.3)，可对照 `numa_get_mems_allowed`、拓扑掩码和节点接口；本课程 Linux 代码直接调用内核接口，不需要 libnuma 链接依赖。
