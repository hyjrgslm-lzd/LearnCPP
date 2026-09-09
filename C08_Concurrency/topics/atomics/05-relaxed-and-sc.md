# 内存模型 02：relaxed 不丢计数，SC 也不是事务

本篇代码为 [F2 Reference](../../exercises/F2_relaxed_counter/solution.cpp)、[F3 Reference](../../exercises/F3_seqcst_fence/solution.cpp) 与 [SC 小模型](../../exercises/runtime_tests/atomic_protocol_test.cpp)。上一节已经证明一次普通 payload 发布，现在改变问题：如果只需要一个数值的精确累计，还需要 acquire/release 吗？如果多个原子对象必须满足共同的顺序约束，单对象的修改顺序又是否足够？

## 1. 先选择合法的操作，再选择所需的顺序

memory_order 不是按枚举大小比较的性能等级。不同操作允许的参数不同。下表用于本课程普通 atomic 的 C++23 路径；C++26 的 consume 说明在表后。

| 操作 | 可用的非弃用内存序 | 为什么 |
|---|---|---|
| load、wait、atomic_flag::test | relaxed、acquire、seq_cst | 读取不能承担 release 写 |
| store、atomic_flag::clear | relaxed、release、seq_cst | 只有写，不能承担 acquire 读 |
| exchange、fetch_*、成功 CAS、test_and_set | relaxed、acquire、release、acq_rel、seq_cst | RMW 既读取又修改 |
| 普通 atomic 失败 CAS | relaxed、acquire、seq_cst | 失败在目标上是 load |
| atomic_thread_fence | relaxed、acquire、release、acq_rel、seq_cst | relaxed fence 无效果；其他见独立篇 |
| notify_one/all | 不接收 memory_order | 通知等待者，不独立发布普通数据 |

C++23 的 consume 可用于具有读语义的相应位置，但语义原本是依赖排序，不是一般 acquire；本课程新代码不使用它。N5050 的 [depr.atomics.order] 已将 consume 弃用，并允许它出现在 acquire 可出现的位置，含义也按 acquire 处理。它仍是 consume=1，而 acquire=2，不能写静态断言称两者枚举等值。历史事实、常见编译器实现和 C++26 新规范是三个不同层次。

acq_rel 在 RMW 上很常见：读取部分可以取得前驱 release 发布的数据，修改部分可以发布本线程在它之前的求值。但仍须存在相应读自关系；一个 RMW 读到了初始值，不会凭字样从一个尚未发生的 release 取得同步。

## 2. relaxed 计数的证明不需要猜缓存刷新

F2 的四个线程各自调用 2000 次 relaxed fetch_add。每次是 RMW，沿 total 的 MO 读取紧邻前驱并加一。初值为 0，且没有其他 store 或减法，所以完成 8000 次之后最后修改为 8000。最后主线程取得全部 future 的完成结果，再 relaxed load total：完成同步与原子连贯性共同保证它不能停在某个已经完成的早期自增之前。

如果主线程在 worker 尚未完成时 load，当然可能得到一个中间数值。“每次更新不丢”不等于“任意时刻读取都得到最终值”。如果用 `total == 8000` 判断可以读取另一份普通数据，还要证明那些普通写都被合适的同步发布，不能把纯计数证明直接套过来。

Reference 同时让每个 worker 增加自己独占的 local[t]。元素互不重叠，主线程仅在全部 get 之后读取它们，因此它们不必是 atomic。这演示另一个实用选择：如果只在工作结束时需要总数，可以先在线程私有状态里计数，再汇总；是否有必要维护一个实时共享计数器，应由需求决定。

relaxed 通常减少排序约束，却仍可能有 RMW 对共享缓存行的争用。换成 seq_cst 后结果不变，速度不一定有可测差异。本实验没有公平性能测量边界，不能以输出执行时间给这些操作排序。

## 3. 有定义的 message-passing 反例

F2 的 `message_passing(false)` 把 data 和 flag 都做成 atomic。发布者 relaxed 写 data=1，再 relaxed 写 flag=1；接收者读一次 flag，再读一次 data。初值均为 0，读到 (flag=1,data=0) 是可允许的结果，因为没有把两个对象关联起来的 SW。由于两者都是 atomic，这个结果属于可讨论的弱序观察，而非普通数据竞争 UB。

将 flag 的写改为 release、读改为 acquire，若接收者确实读到本轮写入的 1，就得到：

```text
data.store(1, relaxed) ——SB——> flag.store(1, release)
                                      │ SW
                         flag.load(acquire)==1 ——SB——> data.load(relaxed)
```

data 的写 HB 它的读。依据原子写读连贯性，读不能再取得该写之前的初值，所以对应旧值统计必须为零。若读到 flag=0，则前提不成立；这轮不属于“看见发布却读旧数据”的统计。

程序用两线程 barrier 在每轮开始前完成清零，在每轮结束后允许下一次清零。barrier 不放进 flag/data 之间，不会把本轮发布者的 body 排到接收者 body 之前。所有轮数固定，接收者只读一次，不需要等待某个弱序结果出现才能结束。统计为零同样是 relaxed 的合法运行结果。

## 4. 为什么 release/acquire 仍允许两边都读到零

F3 的 store-buffering 模式让线程 0 写 x=1，再读 y；线程 1 写 y=1，再读 x。令写为 Sx、Sy，读为 Ly、Lx。若写用 release、读用 acquire，两边读零时，两次读都取得初始化值，而非对方的 release，因此没有跨 body 的 SW。各线程的 SB 仍然存在，但并不足以禁止 (Ly=0,Lx=0)。

这不能解释成“release/acquire 没用”或“所有这类问题只有 SC 才能解决”。它准确说明的是：对这个四操作协议，仅把 store/load 标成 release/acquire 不足以排除双零。改变协议、使用锁或增加其他有证明的同步，也可能改变允许结果。

## 5. SC 增加的全序怎样排除双零

现在把四个访问都设为 seq_cst。它们属于 SC 全序 S，并且两条线程内顺序要求 Sx < Ly、Sy < Lx。如果 Ly 读零，那么在这个只有初始化与一次 SC 写的对象 y 上，Ly 必須排在 Sy 前；否则它会读到已经排在它前面的 SC 写。Lx 读零同理要求 Lx < Sx。

四条边连接成：

```text
Sx < Ly < Sy < Lx < Sx
```

全序不允许环，所以双零不可发生。这是基于协议的禁止结果证明，不是“x86 会插入某条指令”的推测。runtime 小模型枚举尊重两条线程程序顺序的全部六种 SC 排列，结果集合为 01、10、11；这是四步 SC 模型的穷举，不是完整 C++ 弱内存模型求解器。

独立写者和不同读者的 IRIW 也可用相似方法分析：若两个独立原子各被写 1，读者 A 先见 x=1 再见 y=0，读者 B 先见 y=1 再见 x=0，全部 SC 时同样迫使一个全序环。仅有 acquire/release 时不能仅靠各自一次观察建立全体读者一致的跨对象全序。这里作为推导题，不声称当前 F3 executable 实测了四线程 IRIW。

## 6. SC 的三个边界

第一，S 的成员是 seq_cst 原子操作及 SC fence，不是所有普通求值。普通数据仍要先证明没有数据竞争。若看到一个 SC flag 后反复覆盖其对应的普通 payload，SC 不会自动保护随后的访问。

第二，混用弱序时不能把所有访问都塞进 S，然后假设每次读都读取“全局时间线上最后一次写”。N5050 的 [atomics.order] 对 S 给出的约束涉及 strongly happens-before 和 coherence-ordered before；它明确不要求 S 无条件兼容所有 HB 关系。SC 读与非 SC 修改混用也有额外读值规则。本篇的简单环证明只在指定协议和排序选择下使用，不外推到任意混序程序。

第三，SC 不把两个原子操作合成事务，也不保证多字段快照。E1 已在全 SC 的 load+store 中强制丢更新。多个独立 SC load 之间仍可能夹入写者操作；想拿到配套字段，需要单个不可变快照、同一把锁或其他完整协议。

## 7. 运行与答案

在 `C08_Concurrency/exercises` 中分别运行：

```powershell
cmake -S F2_relaxed_counter -B build/f2 -G "Visual Studio 18 2026" -A x64
cmake --build build/f2 --config Release
ctest --test-dir build/f2 -C Release --output-on-failure
cmake -S F3_seqcst_fence -B build/f3 -G "Visual Studio 18 2026" -A x64
cmake --build build/f3 --config Release
ctest --test-dir build/f3 -C Release --output-on-failure
```

F2 总数为 8000，RA message-passing 旧值统计为 0。F3 的 relaxed 和 RA 双零统计可以为 0 或正数；SC 及双侧 SC fence 的双零统计必须为 0。所有输出都在观察窗口外。不要为“没看到允许结果”添加睡眠或 logger 锁。

**relaxed 是否完全无序？** 不是。仍有每对象 MO、原子性、连贯性和线程自身的求值顺序；它自身不建立这里需要的发布 SW。

**读取 total 用 acquire 就能代替 get 吗？** 不能凭这一个改动证明。若写者全是 relaxed RMW，没有相应 release 来源，acquire 不凭空发布其他对象，也不说明所有 worker 已经退出。

**store(acquire) 是否只是较慢但安全？** 不是合法参数选择。应该先按操作种类核对前提，再讨论是否足以实现协议。

**能否从一次 SC fence 推出所有之前普通数据对所有线程可见？** 不能。必须结合另一线程及连接的原子事件分析，详见下一篇。

## 规范入口

[N5050](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf) 的 [intro.races]、[atomics.order]、[atomics.types.operations]、[depr.atomics.order]；[P0668R5](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0668r5.html) 说明现代 SC 规则调整的背景；[P3475R2](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3475r2.pdf) 说明 consume 的弃用与语义调整。
