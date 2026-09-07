# 调度 01：同一份工作，为什么会有人提前闲下来

假设有 4096 个相互独立的记录，前 1024 个需要执行 12000 轮计算，其余记录只需要 120 轮。四个 worker 如果各负责连续四分之一，第一个拿到全部重任务，其他三个很快完成。没有数据竞争，也没有遗漏；问题是最后阶段只剩一个 worker 推进。正确和高效是两个不同的判断。

本篇的真实代码在 [`work_stealing_pool.hpp`](../../exercises/include/concurrency_study/work_stealing_pool.hpp) 的 `cs::scheduling::work` 与 `run`，入口为 [M1](../../exercises/M1_work_stealing_pool/README.md)。`work(id,size)` 使用无符号整数递推，溢出按无符号运算规则回绕，避免把计算负载建立在有符号溢出的未定义行为上。轮数取决于 ID，不依赖 worker 或运行顺序，所以每个版本的任务内容相同。

## 1. 先固定问题，再挑分工方法

输入是任务数 N 和线程数 P，输出是长度 N 的结果数组，位置 i 必须是任务 i 的计算结果。不同任务只写不同数组元素；数组大小在启动 worker 前确定，执行期间不扩容。调用者必须等所有 worker 退出后才读取整体结果。执行顺序没有要求，返回顺序由结果槽位决定。

静态连续分片先确定每个 worker 的起止位置。如果 N 不能整除 P，把余数各分给前几个 worker。这正是 `run("static",...)` 使用商和余数计算 begin/end 的原因：它既覆盖最后几个元素，也允许 N 小于 P 时有空区间。N 为零是合法的空工作，P 为零没有执行者，接口拒绝它。

这个安排的优势是领取工作不需要共享游标，同一 worker 连续处理相邻元素。但“数量平均”只在每个元素代价近似时，才有机会成为“时间平均”。若重任务集中在数组开头，连续分片恰好会放大偏斜。换成轮转静态分配能改善本例，但这属于另一种数据布局假设：若重任务恰好集中在每 P 个元素的同一余数位置，轮转也可能失衡。

## 2. 动态领取把决定推迟到运行时

`run("dynamic",...)` 共享一个原子 next。worker 取得一个 ID，计算完成后再领取下一个：

```cpp
const auto id = next.fetch_add(1, std::memory_order_relaxed);
if (id >= size) break;
values[id] = work(id, size);
```

这里原子操作负责唯一分配 ID。它不发布输入数据，也不宣布整个结果已经完成，因此 `relaxed` 足够承担这个单一角色。线程启动前准备好输入、不同槽位独立写入、future/get 与线程 join 的同步承担其余责任。不能把此处的 relaxed 复制到“写数据后设置 ready”的发布协议中。

动态分配的直觉是：先完成的人继续领，因此实际代价比预估代价更直接地影响分工。不过，每个任务都要访问同一个计数器；任务太短时，共享缓存行和原子操作本身可能比有效计算更贵。批量领取 B 个任务能摊薄领取成本，但最后一个大批次又可能让其他 worker 无事可做。B 是粒度参数，不存在对所有负载通用的最佳值。

## 3. 工作窃取改变待执行任务的存放位置

`run("stealing",...)` 把同样的 ID 封装为 callable，交给每 worker 有一条 deque 的池。外部提交轮转进入不同队列，本地优先取最新任务，空闲时从别人的另一端取最早任务。这个版本是真实的入队、出队、窃取和 future 结果通道，没有把“随机挑一个 ID”命名成工作窃取。

三种版本仍然是在比较同一组独立任务的最终结果，但调度成本不同：静态和动态使用每 worker 一个 packaged_task 来搬运 worker 异常；窃取版本为每个记录创建独立任务和 future。计时记录必须保留这个差异。它回答的是这些完整实现面对该负载的端到端代价，不是孤立的 deque 出队指令延迟。下一篇会说明池的管理锁也可能成为瓶颈，因此不能预先要求 stealing 一定更快。

## 4. 先检查内容，再比较时间

Reference 对 N=0、1、257 和三个版本逐项检查 `values[id] == work(id,N)`。runtime 检查另用原子计数记录 400 个 ID 在多个提交者下是否各执行一次。求和不能代替这个检查：丢失一个值、重复另一个值，有时会产生相同总和。

从 `Concurrency_Study/exercises` 构建普通练习：

```powershell
cmake -S M1_work_stealing_pool -B build/m1 -G "Visual Studio 18 2026" -A x64
cmake --build build/m1 --config Release
ctest --test-dir build/m1 -C Release --output-on-failure
```

统一构建启用 benchmarks 后，可以分别运行：

```powershell
./build/full-windows/benchmarks/Release/scheduling_bench.exe --variant static --size 4096 --threads 4
./build/full-windows/benchmarks/Release/scheduling_bench.exe --variant dynamic --size 4096 --threads 4
./build/full-windows/benchmarks/Release/scheduling_bench.exe --variant stealing --size 4096 --threads 4
```

每次只运行一个 variant，stdout 只有统一 CSV。size 是任务数，completed 是已逐项核验的任务数；时间包含结果存储分配、线程/池构造、提交、计算、等待和 join，检查本身在计时后。进程只测一轮，外部 [`run_benchmarks.py`](../../exercises/tools/run_benchmarks.py) 负责预热和重复采样。脚本参数以其 `--help` 为准，不能再在可执行程序内部悄悄重复五次。

观察时先看 completed 是否一致，再看样本中位数和范围。如果 dynamic 比 static 快，这组输入支持“运行时分工改善当前偏斜”的解释；它不能推出动态领取对所有任务更快。如果 stealing 慢，应分别考虑每任务分配、future、管理锁、队列扫描以及任务粒度，不要未经剖析就把原因写成“上下文切换过多”。

## 自测与答案

**P 大于 N 是否错误？** 不是，部分 worker 得到空区间。错误是漏掉尾部或生成越界 ID。接口拒绝 P=0，而不是把它悄悄替换成硬件线程数。

**把重任务打乱后 static 变快，说明实现改进了吗？** 算法没变，输入分布变了。应保留随机种子并明确这是场景变化，不能与原偏斜输入混成同一组排名。

**一个 relaxed 原子计数器能发布整个结果数组吗？** 本例不能。游标是在计算之前增加的，领完不代表算完。最终可见性来自结果通道和 join；即使游标改为 seq_cst，也不会把尚未发生的计算提前完成。

**把 N 增大是否等同于把单任务变粗？** 不等同。N 改变任务数量和总工作量；提高每个任务轮数才改变计算与调度的比例。一次实验只改变其中一个。

## 规范与实现依据

- [C++26 N5050](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf)，按稳定条款名查找 `[atomics.order]`、`[thread.thread.member]`、`[futures.state]`。这里的线程和原子机制不要求 C++26。
- [oneTBB 官方：分块与粒度](https://uxlfoundation.github.io/oneTBB/main/tbb_userguide/Controlling_Chunking_os.html)，用于进一步对照动态分工的代价；本练习并未使用 TBB 调度器。
