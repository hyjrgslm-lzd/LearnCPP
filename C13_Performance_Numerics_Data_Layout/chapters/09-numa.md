# 09 NUMA 与内存放置

先修：C07 的虚拟内存和页，C08 的线程、affinity 与同步。NUMA 研究 CPU 和内存节点的距离，但它不改变 C++ 对象模型和数据竞争规则。两个线程无同步写同一个对象仍是数据竞争；页面在本地还是远端不能修复语言层错误。

C08 已实现 `concurrency_study/numa.hpp`。C13 使用它作为源码入口，重点讲报告边界：CPU 绑在哪里、页面放在哪里、读到了什么，是三件不同的事实。

## 1. 拓扑不是从商品名猜出来的

NUMA 输出至少包含：

- `group,cpu`：操作系统可调度逻辑 CPU。Windows 有 processor group，Linux 本实现 group 为 0。
- `socket`：物理封装或系统呈现的 package。
- `core`：物理核心编号，同一 core 的多个逻辑 CPU 可能是 SMT 兄弟。
- `node`：内存接近性域。
- `allowed_memory_nodes`：当前进程允许分配的内存节点。

这些编号不能互相替代。一颗 CPU 可以有多个 NUMA node；容器可以允许某些 CPU，却限制可用 memory node；Windows CPU set 和 group affinity 也会改变可见集合。`hardware_concurrency()` 只给提示数量，不给合法 CPU ID。

## 2. affinity 请求不等于实际运行

设置线程 affinity 只是请求。C08 的 `on_cpus` 只在新建专用线程里绑定，并在操作前后调用 `current_cpu()` 检查实际 CPU。它不修改可复用线程池，也不声称能恢复调用者原本的复杂 affinity。

C13 默认 L08 程序只运行拓扑观察，不做绑定实验。传 `--placement 1` 才用少量页面进入 C08 的 `placement_experiment`。这仍是教学级短观察，计时包含 reader 线程创建、绑定、扫描和 join；不适合拿来写延迟排名。

## 3. first-touch 讨论的是页面

匿名内存常在第一次写入时分配物理页。若主线程先 `vector(n)` 清零，后续 worker 再初始化自己的段，页面可能已经由主线程触发分配。要研究 first-touch，需要未预先构造/写入的页，并按 OS page size 切分。

C08 的 `pages` 包装按基础页建立实验：

- Windows 使用 `VirtualAlloc` / `VirtualAllocExNuma`，并用 `QueryWorkingSetEx` 查询 `Node`。
- Linux 使用 `mmap`、`madvise(MADV_NOHUGEPAGE)`，并用 `move_pages` 查询页面节点。
- 查询失败、权限不足、只有单 memory node 或请求节点未全部实现时，返回 SKIP。

首选节点不是强制节点。API 成功只能说明请求被接受；最终仍要逐页查询。如果请求 remote，但一半页面落在 local，就不能输出一行标成 remote 的有效性能数据。

## 4. 单节点机器能证明什么

单节点机器仍能学习：

- 当前进程允许哪些 CPU 和 memory node。
- SMT 关系是否能从 `(socket, core)` 看出来。
- 基础页大小是多少。
- 程序能否构造安全的小型页面观察。

它不能证明跨节点访问成本。报告应写“remote/interleaved unavailable: only one eligible memory node”或类似 SKIP。不要通过增加循环次数、换普通多线程耗时或把 CPU 0/1 当作不同 node 来制造 NUMA 结论。

WSL、容器和受限权限也常见。`move_pages` 被拒绝时，只能说明当前环境不能查询页驻留；源码有 Linux 分支不等于本机验证通过。

## 5. 运行 L08

默认安全拓扑观察：

```powershell
cmake -S C13_Performance_Numerics_Data_Layout/exercises/L08_numa -B build/c13-l08 -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/c13-l08
ctest --test-dir build/c13-l08 --output-on-failure
.\build\c13-l08\C13_L08_numa.exe
```

显式小页放置观察：

```powershell
.\build\c13-l08\C13_L08_numa.exe --placement 1 --pages 4
```

`--pages` 以 OS 基础页计，默认 4，最大 64。程序先 discover topology，再按 `page_size * pages` 构造 `firsttouch` 短实验。它会在 stderr 打印拓扑和页面快照，在 stdout 输出 CSV。若条件不足，返回 77，表示 SKIP，不是失败性能结论。

## 自测与解析

**CPU 0 和 CPU 1 一定是两个 core 吗？** 不一定。它们可能是同一 core 的 SMT 兄弟，也可能属于不同 core。看 `(socket, core)`，不要看编号奇偶。

**绑定线程到 node 0 的 CPU 后，内存就一定在 node 0 吗？** 不一定。必须查询页面驻留；CPU 位置和页面位置是两个观察。

**只有一个 memory node 时，能否证明 remote 不慢？** 不能。remote 没有实验条件，应标 SKIP。

**页面节点前后相同能证明计时区间没有迁移吗？** 不能。它只证明两个查询点相同。要证明迁移事件，需要额外事件或计数器。
