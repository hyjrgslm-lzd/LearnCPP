# 内存模型 03：fence 需要一座可证明的原子桥

本篇独立展开 fence，代码位于 [F3 solution.cpp](../../exercises/F3_seqcst_fence/solution.cpp) 的 `fence_publication(form)` 与 `store_buffering(mode::sc_fence)`。学习目标是能指出 fence 与哪些具体原子操作合作，而不是记忆“前面不能后移、后面不能前移”的口号。

## 1. 为什么只放两个 fence 不够

先想象发布者写普通 data 后执行 release fence，接收者执行 acquire fence 后读 data。如果两线程之间没有连接事件，仅凭它们都执行过 fence，不能建立 SW：接收者没有证据表明自己位于指定发布之后。它可能先运行，也可能根本没观察过发布状态。

fence 不直接读写一个指定原子变量，却可以和其前后的原子访问组合建立同步。因此推导每个实例时，要明确四项：哪个 release 起点，哪个 acquire 终点，桥接用哪个原子对象，以及那个读实际读自哪个写。本篇桥接对象为 ready，初值 false，只有一次写 true。

## 2. release fence 到 acquire 原子读

form=0 的实际顺序是：

```text
写 data -> Frel: fence(release) -> X: ready.store(true, relaxed)
                                             │ read-from
                    Y: ready 的 acquire 读到 true -> 读 data
同步边：Frel SW Y
```

release fence 在 X 前，X 修改 ready，Y 为同一对象上的 acquire 读取并读到 X 的值，满足 fence-atomic 规则。普通写在 Frel 前，普通读在 Y 后，所以写 HB 读。这里 X 本身没有 release，不妨碍 fence 承担发布起点。

注意发布保证覆盖的是 fence 之前的普通写。如果把 data 的写移到 fence 与 X 之间，就不能沿这条边发布它。不能因为“最后依然有一个原子 store”而忽略 fence 的位置。

## 3. release 原子写到 acquire fence

form=1 将发布者改为 release store，接收者通过 relaxed 读取 ready 后再执行 acquire fence：

```text
写 data -> X: ready.store(true, release)
                         │ read-from
          Y: ready 的 relaxed 读到 true -> Facq: fence(acquire) -> 读 data
同步边：X SW Facq
```

Y 在 Facq 前且读到 X；规则让 release 原子操作与 acquire fence 同步。因此 payload 的读必须放在 Facq 后。若先读 data 再补 acquire fence，fence 不会追溯修复已经发生的无保护读取。

这里 Y 是 wait 内部最终返回所依据的 relaxed load。由于 ready 只有 false→true，wait 返回意味着它观察到发布值。若用普通 load 循环，同样需要找到最终读到 true 的那一次读取。notify 不充当 read-from 边。

## 4. release fence 到 acquire fence

form=2 的 X、Y 都使用 relaxed：

```text
写 data -> Frel -> X: store(true, relaxed)
                        │ read-from
          Y: load(relaxed)==true -> Facq -> 读 data
同步边：Frel SW Facq
```

Frel SB X，X 修改桥接原子，Y 读取 X 且 SB Facq，于是 fence-fence 同步成立。完整标准还允许相应的 release sequence 或假想 release sequence 来源，本例只有一次直接写，先不增加不必要状态。

三种形式都在 `fence_publication` 中检查 observed==42，读取发生在 producer.get() 之前。它们是同一“一次发布普通 int”的契约下不同的同步表达，不能据此宣称 fence 比带序的原子操作更省指令。实际代码布局、架构和优化决定成本。

## 5. SC fence 为什么需要对称分析

F3 的 SC fence 版本是两边各自：

```text
线程 0：Sx: x.store(1,relaxed) -> F0: fence(seq_cst) -> Ly: y.load(relaxed)
线程 1：Sy: y.store(1,relaxed) -> F1: fence(seq_cst) -> Lx: x.load(relaxed)
```

假设两个读都取得初值。由于初始修改在每个对象的 MO 中位于其后写 1 之前，Ly coherence-ordered before Sy，Lx coherence-ordered before Sx。第一对再结合 F0 HB Ly、Sy HB F1，依据 [atomics.order] 关于两侧 SC fence 的约束，得到 F0 在 S 中先于 F1；第二对得到 F1 先于 F0。矛盾，因此双零被禁止。

这段证明没有说 relaxed 的 Sx、Ly 等“自动加入了 SC 全序”。真正进入 S 的是 F0 和 F1；普通原子访问通过连贯顺序和 HB 限制两个 fence 的相对位置。把所有 relaxed 操作当成 SC，是很容易在更复杂程序里产生错误结论的简化。

删掉任一侧的 SC fence，上述矛盾的两条约束就不再完整，不能声称单侧 fence 也足够。把两侧换成 acq_rel fence，也没有 SC 全序可用；在双零结果中又没有读取对方发布，所以不能用刚才的发布桥证明。SC fence 也不是一般意义上“覆盖所有数据的锁”。

## 6. signal fence、volatile 和性能边界

`atomic_thread_fence(relaxed)` 没有同步效果。acq_rel fence 同时具有 acquire 和 release 角色，但仍要满足相应桥接条件。`atomic_signal_fence` 处理同一线程与其信号处理程序之间的相关顺序，不能替代线程间的 atomic_thread_fence 发布协议。

volatile 不提供这里的原子桥。把 ready 变为 volatile bool，不能保留这三种证明；普通数据的访问不因两边插了 fence 就免于数据竞争。标准规则是从具体操作及读值来源推导，不是从“编译器大概不能优化掉”推导。

从工程角度看，给某个原子 load/store 直接指定 acquire/release 往往更容易审查；fence 适合需要明确独立排序点且能够写出完整证明的场合。本篇没有计时排名，不能把“栅栏集中在一点”直接等同于更少硬件屏障、更快或更低功耗。

## 7. 运行和自测答案

在 `C08_Concurrency/exercises` 中：

```powershell
cmake -S F3_seqcst_fence -B build/f3 -G "Visual Studio 18 2026" -A x64
cmake --build build/f3 --config Release
ctest --test-dir build/f3 -C Release --output-on-failure
```

Reference 验证三种桥都得到 42，以及双側 SC fence 的双零统计为 0。barrier 只管理轮次，窗口内没有日志、mutex 或附加同步；读值先记录，全部完成后才断言。若未来修改 body 加入可能抛出的操作，必须先设计 barrier 参与者退出策略，不能让一个线程异常离开而另一个永远等它。本版 body 只有不抛异常的原子操作与固定数组赋值。

**两个 fence 执行时间有先后，为什么不算同步？** 因为墙钟先后不是 SW 规则。必须有对应的原子修改和取得该修改的读取，或其他规范规定的同步操作。

**form=2 的两个 relaxed 操作能删除一个吗？** 不能沿用同一证明；失去修改或读取就失去桥。若另有同步，需要另写证明。

**fence 能保护读者持有的裸指针不被 delete 吗？** 单凭可见性不能。对象销毁可能发生在读者 acquire 之后，需要所有权或回收协议，下一章讨论。

规范依据为 [N5050](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf) 的 [atomics.fences] 与 [atomics.order]；SC 规则背景可读 [P0668R5](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0668r5.html)。在线 [fence 条款](https://eel.is/c++draft/atomics.fences) 仅作定位，若文字已进入 C++29 演进，以固定版本为课程依据。
