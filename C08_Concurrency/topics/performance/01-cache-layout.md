# 布局 01：不同变量，为什么仍会互相影响

先读[测量先修](../../chapters/12-measurement.md)。本篇的实现位于 [J1/reference.hpp](../../exercises/J1_false_sharing/reference.hpp)，正确性答案在 [J1/solution.cpp](../../exercises/J1_false_sharing/solution.cpp) 和 [J2/solution.cpp](../../exercises/J2_interference_size/solution.cpp)，性能驱动为 [layout_bench.cpp](../../exercises/benchmarks/layout_bench.cpp)。这里研究两线程计数，不把观察到的耗时差当成同步正确性的证明。

## 1. 先在语言层判断是否冲突

两个线程同时对同一个普通整数执行 `++x`，读写没有同步，构成数据竞争。把两个对象分得再远也不能修复它。反过来，线程 0 只写 `a`、线程 1 只写 `b`，并且对象寿命覆盖线程执行，主线程在 join 后读取结果，那么它们访问的是不同存储位置，可以没有数据竞争。

无数据竞争不等于没有硬件代价。在常见缓存一致性多核上，核心为了修改某个缓存行中的字节，需要取得相应一致性权限。另一个核心频繁修改同一行中的其他字节，也可能触发这类权限转移。C++ 把两个对象看成不同的存储位置，硬件却可能把它们放在同一个一致性粒度中：这就是伪共享。

本实验使用 `atomic<uint64_t>`。这样每次 `fetch_add` 都是明确的原子读改写，不会因为最终只需一个整数而把整个增量循环化简成一次普通赋值。`memory_order_relaxed` 放宽的是与其他内存操作的排序约束，不会让原子修改免去缓存一致性协议。它也不会把几个线程对同一个对象的增量变成数据竞争。

请避免“所有 CPU 永远以 64 字节搬运”这样的描述。缓存行长度、一致性粒度、指令跨行访问、内部传输和目标平台都有具体条件。本课程先使用实现给出的布局提示，在实验记录中打印地址和尺寸；物理缓存结构仍要查目标机器资料。

## 2. 给四个版本分别写合同

每线程处理 `N` 个事件，最后 join；不要求消费者在运行中读取实时计数。输入只有事件数量，事件没有额外负载。`counters.run` 每次清零，使重复调用仍是独立任务，`verify` 不计时。

| 版本 | 写入对象 | 原子更新次数 | 观察合同 |
|---|---|---:|---|
| packed | 每线程一个相邻 atomic | `2N` | join 后两个值各为 N |
| padded | 每线程一个分散 atomic | `2N` | 与 packed 相同 |
| shared | 两线程同一个 atomic | `2N` | join 后单值为 2N；失去分线程明细 |
| batched | 局部累积后加到 shared | `2*ceil(N/B)` | join 后单值为 2N；运行中可滞后 |

packed 到 padded 是同合同的布局改进。shared 是真共享对照：即使把它单独放在一个缓存行，所有线程仍修改同一个逻辑对象，padding 无法让同一个整数同时变成两个独立整数。batched 则在最终聚合合同下减少共享更新；若产品还需要即时可见或每线程统计，必须把它标成场景切换。

这解释了为什么“把所有 atomic 都加 alignas”不是通用优化。在真共享路径上，热点是一个对象本身；应研究分片或批量发布。只读数据则可能受益于相邻布局，同一段代码一次读取几个相关字段，过量 padding 会浪费缓存和内存容量。

## 3. 用对象布局表达实验，而不是猜测地址

实际代码的核心类型如下：

```cpp
struct alignas(destructive) packed_pair {
    std::atomic<std::uint64_t> value[2]{};
};
struct alignas(destructive) padded_counter {
    std::atomic<std::uint64_t> value{0};
};
```

给 `packed_pair` 整体对齐，保证第一个元素从提示边界开始。两个 atomic 的步长由 `sizeof(atomic<uint64_t>)` 决定。并不假设所有实现的 atomic 恰好为 8 字节；J2 打印实际偏移和“按提示尺寸划分是否同区域”。如果两个元素的总布局超过提示区域，或真实硬件粒度与提示不同，就不能宣称已经建立了某个物理缓存行上的伪共享。

对 padded 而言，类型本身的对齐要求会影响它的大小，使数组后续元素仍满足对齐要求。因此 `padded_counter p[2]` 的相邻元素步长是 `sizeof(padded_counter)`，至少达到该对齐要求。只把一个普通 atomic 数组的首地址对齐，并不会改变数组内部元素的步长；这是 J2 必须分清的两种操作。

`std::hardware_destructive_interference_size` 是实现定义的编译期布局提示，用于分离可能互相干扰的对象；`constructive` 描述把一起使用的数据聚集时的布局提示。它们不是一次运行时 CPUID 查询，不负责告诉你哪个线程在哪个核上，也不保证跨编译目标的 ABI 一致。缺少特性宏时，本实验退回 64，并输出 `implementation_hint=false`；这个回退是教学参数，不是自动探测结论。

如果这种类型出现在动态库公开 ABI 中，两个编译单元使用不同目标选项，可能对成员偏移和 sizeof 产生不同认识。这里的类型只在同一课程目标内使用。真实库若要固定二进制布局，应该把布局常量纳入版本化 ABI 约定，而不是期待所有构建都得到同一个实现值。

## 4. 局部累积如何减少代价，也如何改变可见性

batched 的关键是 worker 自动存储期的 `local`，只有该 worker 使用。每次事件把 local 加一，到 B 时做一次共享 `fetch_add(local)` 并清零；循环后还有不足 B 的尾部，也要发布。这种“线程拥有的局部变量”不要求使用 C++ `thread_local` 关键字。若变量只在一个任务生命周期内存在，普通局部变量更直接；跨任务保存统计时才研究 TLS 的初始化、析构和复用问题。

```cpp
if (++local == batch) {
    target->fetch_add(local, std::memory_order_relaxed);
    local = 0;
    ++published[t];
}
```

考虑 `N=257, B=256`。第 256 个事件发布 256，最后一个事件留在 local；退出前再发布 1。每线程恰好两次共享更新，总共四次，总数 514。如果漏掉尾部发布，`N=256` 的检查会通过，`N=257` 才暴露错误。因此 Reference 特别覆盖空输入、一个事件、257、4097。

在循环中，每个 worker 在一批尚未发布时最多持有 B 个待发布事件：第 B 次局部递增之后、原子发布之前也可能被抢占。若有 T 个 worker，这个算法层面的瞬时滞后上界是 T*B；只在完成每次“检查并必要时刷新”之后观察才可写为 T*(B-1)。此外，relaxed 并不为其他线程承诺墙钟时间上的即时可见性，所以这个计数差界不能被解释成实时显示的延迟上界。

该实验只计数，编译器可能简化部分局部算术，这也是该工作负载允许的行为。不要把它外推成任意复杂事件处理都能获得相同收益；真实负载要重新加入事件读取和处理成本，并保持四个版本的有效工作一致。

## 5. 线程收尾也是算法的一部分

`parallel_chunks` 使用 `jthread` 管理已启动线程。每个 worker 将异常放入自己的槽位，主线程 join 完全部线程后重新抛出。不同槽位没有并发写冲突；主线程也不在 join 前读取它们。若创建后续线程失败，已构造的 jthread 会在栈展开时 join，不会遗留访问已销毁计数器的线程。

实验没有使用 sleep 来“等另一个线程开始”。本版本包含线程创建时间，没有启动门保证两线程同时进入热循环；调度重叠程度属于测量条件。若额外加 barrier，需要处理“只创建了一个线程而第二个创建失败”时的收尾问题，不能让已有线程永远卡在 barrier。当前有限任务无需这个额外协议。

## 6. 预测与复现

从 `C08_Concurrency/exercises` 配置 J1/J2 的叶项目：

```powershell
cmake -S J1_false_sharing -B build/j1 -G "Visual Studio 18 2026" -A x64
cmake --build build/j1 --config Release
ctest --test-dir build/j1 -C Release --output-on-failure
cmake -S J2_interference_size -B build/j2 -G "Visual Studio 18 2026" -A x64
cmake --build build/j2 --config Release
ctest --test-dir build/j2 -C Release --output-on-failure
```

预测所有版本都通过完成量检查；不预测某个确定倍数。基准可通过已有 benchmarks 叶项目构建 `layout_bench`，然后调用 `tools/run_benchmarks.py`，分别指定 packed/padded/shared/batched，参数 `--size 100000 --threads 2 --batch 256`。把 size 改为 1000000，是新的规模实验；把 batch 改为 1，应得到和 shared 相同的共享更新次数，但循环结构不同，时间仍不必相同。

如果 padded 没有更快，先检查 atomic 是否 lock-free、布局提示与实际地址、线程是否并发执行、采样范围和任务规模。不能为了“证明伪共享”删掉这个结果。如果 batched 更快，先结合 CSV 中的 atomic_updates 解释更新次数差异，不要把它归功于 padding。

## 自测与完整答案

**relaxed 是否意味着不再争抢缓存行？** 否。原子对象仍有修改顺序，硬件仍需完成原子读改写和一致性权限管理。内存序与物理布局是不同维度。

**padded 改进了 true sharing 吗？** 它能分离不同对象，不能把一个共享对象的逻辑冲突消除。shared 的两个 worker 仍更新同一个 atomic。

**为什么局部求和只在结束时写一个槽位，不立即 padding？** 高频写已经变成寄存器或 worker 私有存储，槽位只写一次，可能的伪共享次数很少。应先测是否值得为每个槽位消耗一个提示区域。`sum_threaded` 正是这一版本。

**只看 sizeof 足够证明物理缓存行布局吗？** 不够。sizeof 给出 C++ 对象布局，配合地址可验证提示区域；物理粒度和运行时核放置仍需平台证据。

**读者实时读取 batched 总数，能看到多少滞后？** 算法中每 worker 发布前最多暂存 B 个事件；真实可见性还受调度与同步合同限制。若要求每事件发布，就不能接受这个版本。

## 资料

- [WG21 P0154R1](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0154r1.html)：硬件干扰尺寸的设计理由。
- [N5050](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf)：`[hardware.interference]`、`[basic.align]`、`[atomics.order]`。本篇布局和发布合同是教学选择。
- [Intel 手册入口](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)：查询实际目标的一致性、缓存和性能监控资料。
