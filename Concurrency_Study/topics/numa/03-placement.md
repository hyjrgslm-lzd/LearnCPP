# NUMA 03：first-touch、首选节点和真正驻留的页

一个数组分配成功，不等于它的全部物理页已经分配完成。操作系统可以先保留虚拟地址并承诺访问权限，等真正写入时才为匿名页建立物理后备。如果主线程先把整个数组清零，后面的 worker 再“初始化自己的部分”，可能只是在改写已有页面，已经错过了决定初始放置的时机。

本节的可运行实现是 [`pages` 与 `placement_experiment`](../../exercises/include/concurrency_study/numa.hpp)，完整检查为 [N1](../../exercises/N1_numa_placement/README.md)。我们把分配、触页、观测和计时明确拆开；`vector<uint64_t>(n)` 不适合作为本实验的未触页内存，因为它会初始化元素。

## 1. 页级决定不能按元素理解

first-touch 讨论的是页在首次需要物理后备时的分配策略，不是每个 C++ 对象都有独立 NUMA 归属。若两个 worker 的分界落在同一页里，先触页的人可能决定整页的初始节点。给每个线程分一段元素还不够，要按操作系统页大小对齐分片边界。

本例运行时查询基础页大小，要求总字节数是它的正整数倍。Windows 使用普通 VirtualAlloc/VirtualAllocExNuma，不请求大页；Linux 用匿名 mmap，并请求 `MADV_NOHUGEPAGE`，失败则跳过，避免拿透明大页的实际粒度假装基础页粒度。Windows 查询若看到 LargePage 标志也跳过。

`pages::initialize(begin,end)` 按完整页面范围为每个 uint64_t 建立对象并写 1，既真正触页，也给后续正确性检查一个确定值。`sum` 读取分片全部 word；各 reader 区间不重叠，最终总和必须等于 word 总数。读取使用 volatile 迫使每次计划中的读取发生，这不提供同步，也不把循环变成内存延迟测试。

## 2. 五个版本分别改变什么

| variant | 初始化/页面请求 | 读取安排 | 主要比较问题 |
|---|---|---|---|
| local | 首选 reader 的本地 memory node，单线程触页 | 一个固定 reader CPU | 已验证的本地扫描 |
| remote | 同样的初始化方式，首选另一个允许 memory node | 与 local 相同 CPU | 只改变请求页面节点 |
| interleaved | 在本地和另一个允许 node 间按页交错 | 与 local 相同 CPU | 改变分配策略，保留读取线程 |
| firsttouch | 不设显式 node，由第一个 CPU 初始化所有页 | 固定的两个 reader 分片，若不足则一个 | 串行初始化对分片读取的影响 |
| parallel-init | 不设显式 node，各 reader 初始化未来负责的页 | 与 firsttouch 同一组 CPU 和分片 | 只改变初始化者与页面分布 |

firsttouch/parallel-init 优先选不同 node 的两个 CPU；只有一个 node 时，若可能，选不同 core 的两个 CPU。它们的线程数和读取范围彼此一致。local/remote/interleaved 是另一组单 reader 实验，不能把 parallel-init 的双 reader 时间直接除以 local 的单 reader 时间并称作纯 NUMA 收益。

Linux 的默认策略常使匿名页在首次写入者附近分配，但继承的内存策略、cpuset、内存压力和回退都可能改变结果。显式单节点使用 mbind 的 preferred 模式，多节点使用 interleave，节点列表先排序为 nodemask 顺序。Linux 分支保留一段匿名 VMA，禁用该映射的透明大页。

Windows 的 nndPreferred 只在新建 reservation 时起作用。先 reserve 整段、再对已保留地址逐页 MEM_COMMIT，后面的首选 node 会被忽略；那不是可用的交错分配方案。这是首版的实现错误，不能归咎于测试机器只有一个节点。修订后的 `pages` 对每个基础页调用一次地址为 nullptr 的 `VirtualAllocExNuma(..., MEM_RESERVE | MEM_COMMIT, ..., node)`，按排序后的 node 列表交替请求。没有显式 node 的 firsttouch/parallel-init 也每页新建 reservation，但使用 VirtualAlloc。该选择直接遵循 [VirtualAllocExNuma 的参数说明](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualallocexnuma)。

为保持比较公平，Windows 的五个 variant 全部使用同样的独立页指针数组和逐页扫描，local/remote 也不再使用连续大段内存。每块包含一个基础页，每个新基址受系统 allocation granularity 约束；本机为 4096 字节页、65536 字节分配粒度。虚拟地址可能离散，不能把逻辑 page index 当成地址等差关系，也不能忽略它对 TLB、预取和地址空间碎片的影响。日志和 CSV 记录布局、reservation 数、分配粒度及页大小。与首版连续布局的时间不属于同一比较条件；跨平台布局也不同，不能直接排名。

这一方案有每页一次系统分配及一份指针表的成本。分配和释放不计入扫描时间，指针查找和页内扫描计入；所有 variant 的这个边界相同。构造前先分配完整指针表，之后任意一个系统分配失败，catch 逐个释放已成功的独立 reservation；正常析构同样逐块 MEM_RELEASE，没有对离散块使用一个大范围释放。更粗的独立块可以减少 reservation 数，但会把交错粒度从页变成块，必须作为另一组实验记录。

首选不是强制。API 成功意味着请求被接收，不保证每一页最终在首选节点。N1 比较全部页的实测 node 与预期向量；不满足就返回 77 并给出页面直方图，不把错误标签的数据写入性能 CSV。

Linux 还必须区分循环顺序与起始相位。匿名内存的 interleave 选择与 VMA/page offset 有关，不能把所查地址范围的第一个页无条件当成相位 0。`interleave_phase` 先按排序且去重的 node 集合检查完整的逐页循环，允许一个固定循环移位，至少需要观测一个完整周期。after-touch 确认该序列后保存它，后续 before/after 快照必须逐页与已确认序列一致，不重新挑一个相位掩盖变化。它验证实测布局符合循环策略，不声称反推出内核内部的全部 VMA 状态。[Linux 内核说明](https://www.kernel.org/doc/html/latest/admin-guide/mm/numa_memory_policy.html) 给出了 page offset 与策略的关系。

N1 的纯模型检查包含双节点奇相位、反向请求次序加奇相位、三节点排序，以及“直方图正确但连续两页放同一 node”的反例。它们在 Windows 也能运行，只验证算法；Linux 系统调用和实际页面粒度仍未在本机实测。MADV_NOHUGEPAGE 是本实验基础页前提，不以排序或直方图替代页面粒度约束。

## 3. before/after 查询到底验证了什么

Windows 使用 `QueryWorkingSetEx` 获取每个页地址的状态。只有 Valid 页的 Node 字段才被当作驻留证据；未驻留页记为 -1。Linux 对本进程使用 `move_pages`，nodes 参数为 null，因此只是查询，不请求迁移；逐页失败也记为不可验证，系统调用被权限/容器规则拒绝则整体 SKIP。

实验执行四个阶段：

1. `before-touch`：分配后、写入前查询。常见结果为未驻留，但这不是后续证明的前提。
2. `after-touch`：全部初始化线程结束后查询，逐页验证请求是否实现。
3. `before-timing`：执行一次不计时读取，核验结果，再查页节点。读路径已经过一次，初始触页成本不在正式扫描中。
4. `after-timing`：单轮正式读取及 join 完成后，再查页并核验总和。

同时保存初始化者和 reader 的实际 CPU before/after，二者独立于页面查询。一个记录里必须能分别回答“谁在读”和“读的页在哪”。CPU 端点正确而页面混在不同 node 时，不能用 reader 的 node 给全部内存贴标签。

查询是快照，前后相同也不能证明中间从未迁移。Linux 自动 NUMA balancing、显式迁移、回收后重新驻留和其他系统活动都可能改变页面；Windows 首选分配也有回退。当前程序不迁移现有页，只检测阶段端点。如果研究迁移本身，要单独记录迁移调用、成功/失败页和迁移前后映射，并把迁移耗时与稳定扫描时间分开。

## 4. “排除 fault 成本”能承诺到什么程度

代码把虚拟分配、初次写入、第一次读取和页面查询放到计时外，因而排除了显式初始化触发的首轮 fault 工作。它没有锁定页面，也没采集正式扫描期间的页错误计数；内存压力下的后续 fault 仍可能发生。因此准确表述是“排除已知初始触页阶段”，不是“测量区间零 page fault”。

此外，正式计时包含 reader 线程创建、设置 affinity、读取及 join。小数组时这些固定成本可能盖过扫描。不能把总毫秒数除以元素个数，当成单次 DRAM 延迟。顺序读取有预取和缓存效果，本程序提供端到端扫描观测；若要测依赖访问延迟，应另写受控随机指针链并固定同样的放置证据。缓存冲洗、页锁定和硬件计数器属于新的实验因素，需要明确新增，不能暗中改变当前版本。

## 5. 运行与预期

N1 的 main 用 16 页演示本地路径，Reference 用 128 页检查所有可用方案。统一构建后：

```powershell
cmake --build build/full-windows --config Release --target N1_numa_placement N1_numa_placement_reference numa_bench
ctest --test-dir build/full-windows -C Release -R N1_numa_placement_reference -V
./build/full-windows/benchmarks/Release/numa_bench.exe --variant local --size 33554432
./build/full-windows/benchmarks/Release/numa_bench.exe --variant remote --size 33554432
```

本次 Windows 作者运行中，128 页在触页前均为 -1，触页后及扫描前后均为 node 0；firsttouch/parallel-init 的 reader 在 CPU `0:0` 和 `0:2`。可验证的本地部分通过，只有一个允许 memory node，所以 Reference 最终以 77 标记 remote/interleaved 条件不足。这不是“整个题没有运行”，更不是远端速度等于本地速度。

Reference 为 shared-read 和五个 placement variant 分别输出 PASS/SKIP/FAIL 后汇总：任一 FAIL 最终为 1，否则任一 SKIP 最终为 77，全部完成才为 0。只有一个可用 reader、但有两个 memory node 时，remote/interleaved 可能仍可运行，shared-read 的跨节点 Part 却不完整；后续成功不会清除这个 SKIP。纯模型覆盖这个条件，不能把两个 memory node 当成两个可用 CPU node。

基准 stdout 仅在完整验证后发出一个 variant 的 CSV，诊断均在 stderr。size 单位是字节，completed 是读取并核验的 uint64_t 个数，线程数由上述方案决定。每进程正式扫描一次；外部 runner 负责多次进程采样。若改变大小，保存全部样本、页大小、CPU/页面节点和 scope，不要求 remote 必须更慢。

公共 runner 的 PARTIAL_SKIP 默认返回 77；只有明确指定 `--allow-partial` 才允许有有效样本的部分结果返回 0，报告中的 PARTIAL_SKIP 标签仍保留。这个选项不把未运行的硬件条件变为已验证。

## 自测与答案

**先 `vector(n)` 再让各 worker 清零，能研究 first-touch 吗？** 通常不能据此声称未触页，vector 构造已经初始化了元素。应使用本例未预先初始化的虚拟页并查询 before-touch。

**第一次读零值是否一定为每页分配独立本地物理页？** 不能这样推断。系统可以有共享零页或其他优化。本例首次写入实际值，并在之后查询驻留节点。

**请求 remote 成功但一半页在 local，应该如何报告？** 打印两部分实测分布，返回不可验证，不能仍输出标签为 remote 的有效排名行。

**前后页节点相同，是否证明没有迁移或 fault？** 不证明。它仅说明两个观察点一致。要进一步声称区间内事件，必须收集相应事件证据。

## 官方入口

- [VirtualAllocExNuma](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualallocexnuma)：首选节点、保留/提交和实际后备的边界。
- [QueryWorkingSetEx](https://learn.microsoft.com/en-us/windows/win32/api/psapi/nf-psapi-queryworkingsetex) 与 [PSAPI_WORKING_SET_EX_BLOCK](https://learn.microsoft.com/en-us/windows/win32/api/psapi/ns-psapi-psapi_working_set_ex_block)：Valid、Node 和 LargePage 的解释。
- [Linux 内核 NUMA Memory Policy](https://www.kernel.org/doc/html/latest/admin-guide/mm/numa_memory_policy.html)、[Page migration](https://www.kernel.org/doc/html/latest/mm/page_migration.html)。
- [libnuma 官方手册](https://github.com/numactl/numactl/blob/master/numa.3) 和 [move_pages 手册](https://man7.org/linux/man-pages/man2/move_pages.2.html)，用于核对 preferred/interleave 和查询失败行为。
