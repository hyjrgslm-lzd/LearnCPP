# 原子操作 02：CAS 是一次条件提交，循环才是算法

本篇对应 [E3](../../exercises/E3_cas_loop/README.md)，完整函数在 [solution.cpp](../../exercises/E3_cas_loop/solution.cpp)。我们已经知道 load 加 store 不能组成原子更新。现在假设需要的操作是“把历史最大值提高到候选值”，或者“把一个数乘以给定系数”。问题不是如何写计算公式，而是如何防止计算期间目标被别人改过。

## 1. 把校验和提交合成一个动作

CAS 接收 expected 和 desired。以整数为例，它比较目标的当前值与 expected：匹配就原子地写入 desired，不匹配就不修改目标。成功返回 true，属于 RMW；失败返回 false，在目标上属于 load，并将这次比较观察到的目标值写回 expected。

Reference 先把目标设为 5、expected 设为 4，尝试换入 9。失败后目标仍为 5，expected 变成 5。再次尝试成功后目标为 9，expected 仍为 5。成功路径不把 expected 改成 desired。

expected 是调用者持有的普通对象，失败时库会写它。若多个线程共享同一个 expected 而没有保护，目标是 atomic 也不能保护 expected。通常每次调用使用线程局部变量。

“回填当前值”也不能写成“取得之后永不失效的最新值”：CAS 返回后，目标可能立即又被其他线程修改。它提供的是这次原子比较的观察值。下一次条件提交仍需重新比较。

泛型 CAS 比较的是值表示，不调用用户的 operator==。浮点数的正负零、NaN，以及某些存在不同值表示的类型，需要额外分析。本篇选整数，避免把数值比较和表示比较混在一起。C++20 后不参与值表示的填充位被忽略，也不能据此假设任意 union 的活动成员切换都适合 CAS。

## 2. 为什么失败后必须重新计算

E3 的 `fetch_multiply` 采用这个顺序：

```cpp
unsigned old = target.load(std::memory_order_relaxed);
for (;;) {
    // 检查 old * factor 是否可表示。
    const unsigned desired = old * factor;
    if (target.compare_exchange_weak(old, desired,
                                    std::memory_order_relaxed,
                                    std::memory_order_relaxed))
        return old;
}
```

完整代码在计算前检查 `old > max / factor`，并单独处理 factor 为 0 时不能做除法的情况。公式放在循环内，是因为失败后 old 可能已变化。若第一次用 3 算出 6，竞争者将目标改成 4，而重试仍提交旧 desired=6，就把本应得到的 8 写成了 6。原子提交成功也不能挽救错误的业务计算。

Reference 的输入域是 unsigned，且要求乘积可表示。发现基于当前观察值的乘积超出范围时抛 overflow_error，目标不被这次调用修改。它不承诺“只要未来某线程可能把值降低，就必须一直等到成功”。四个 worker 各乘四次 2，初始为 1，最终应为 65536；单独检查溢出和乘以零的分支。普通有符号乘法溢出仍是 UB，不能在 CAS 后才检查。

循环中的计算还可能被执行多次。不要在“构造 desired”期间发消息、扣外部款项或进行不可重放副作用。一个成功 CAS 只提交它管理的那个对象，不替你回滚此前尝试做过的外部动作。

## 3. weak、strong 与两条内存序

weak 允许伪失败：即使值表示相等也可能返回 false，此时 expected 回填的表示可以与原来一样。strong 不允许这种伪失败。循环本来就会重试时，weak 往往合适；单次判断需要“匹配就成功”时，strong 更直接。不能把一次 weak 失败都算成“有另一个写者赢了”，也不能要求运行时必须观察到伪失败。

成功和失败可以使用不同内存序，因为成功是 RMW，失败只有读取效果。普通 atomic 的失败序不能是 release 或 acq_rel。常用的两组选择是纯数值统计的 relaxed/relaxed，以及需要同时取得旧发布状态并发布新状态的 acq_rel/acquire。I1 的多写者快照更新会用到后一组。

只传一个 order 时，成功序就是它；失败序从中去掉 release 部分：release 对应 relaxed，acq_rel 对应 acquire，其他值保持原样。因此传入 release 并不意味着失败后得到的指针已经 acquire；若下一轮要解引用 expected，必须重新证明其数据可见性与寿命。

历史教材常写“失败序不能比成功序强”。对于课程默认的现代普通 atomic 接口，这不是完整准确的现行条款：C++17 起已移除该一般性强弱限制；N5050 [atomics.types.operations] 的失败序前提列出 relaxed、acquire、seq_cst（弃用 consume 按 acquire 处理）。不要用枚举数值大小排序，也不要把所有特化和所有旧标准概括成一张不经核验的强弱表。工程中仍应为成功、失败两条实际路径各选所需的序；本题不使用不常见的组合。

## 4. 最大值：相同数值不等于相同操作契约

`raise_max` 首先 load。如果 old 已不小于 candidate，就直接返回；否则用 weak CAS 尝试提高，失败后用回填 old 再判断。所有调用都只提高数值时，目标单调不减；并发候选范围是 0 到 3999，最终应为 3999。

这个函数的契约是“提高数值”，不是标准 fetch_max 的完整替代品。shortcut 分支可能只执行 load，没有修改事件、没有 release RMW，也不能靠它向后发布本线程新写的数据。给初始 load 填入 memory_order_release 更是非法。

`fetch_max_rmw` 则每次都尝试 CAS，把 `max(old, candidate)` 写回，即使 desired 与 old 相等，也要等一次 CAS 成功才返回。它完成的是 RMW，返回该操作修改前的值。数值没有变化，不意味着修改顺序中没有该 RMW 事件。

C++26 提供 fetch_min/max，E3 用 `#if CS_HAS_ATOMIC_MIN_MAX` 检查原生整数接口及旧值语义；能力不可用时仍验证两种 CAS 基线，只打印该专项 SKIP，不将整题当作未运行。不要只凭语言标准开关推断库已经实现，也不要用滚动草案中的浮点接口推断本题的整数能力探测覆盖了其他特化。

## 5. 实验与可证明的范围

在 `C08_Concurrency/exercises` 中：

```powershell
cmake -S E3_cas_loop -B build/e3 -G "Visual Studio 18 2026" -A x64
cmake --build build/e3 --config Release
ctest --test-dir build/e3 -C Release --output-on-failure
```

每个独立 worker 都通过 future 返回，分配或检查异常不会消失。没有线程等某个可能因检查失败而永远不发送的确认；本题也没有把打印放到 CAS 重试窗口里。测试不要求重试次数大于零，不比较 weak 与 strong 速度。

CAS 单个操作可以是无锁的，循环不保证某个线程在有界步数内完成。竞争时可能一直失利；本篇的数值操作没有公平承诺，标准也不把操作数量有限等同于调度期限有界。锁自由、分配器、回调、析构和等待协议都需要在完整算法层面重新分析。

## 自测与答案

**为什么 F1 中某个 CAS 循环会每次把 expected 重置为 1，而乘法循环不重置？** F1 只允许状态转移 1→2；若失败读到 0 后继续用 0，就可能提前发布 2。乘法允许针对任何当前值重算，应该保留回填值。expected 的使用取决于状态机契约。

**strong 能解决 ABA 吗？** 不能。值从 A 变为 B 再回 A，strong 可以成功；它验证本次比较时的表示，不保存修改历史。指针还涉及对象是否已经销毁或复用，需要另一个寿命协议。

**最大值已是 10，候选为 3，两种函数都返回 10，是否可以随意互换？** 只检查最终数值时可能看不出区别。若算法需要调用产生 release RMW 或延续某种同步论证，shortcut 的纯 load 路径就不能替代。Reference 的数值检查不能单独证明内存序契约。

**失败序用 relaxed 总是安全的吗？** 若失败路径只用整数重新算值，本题可以；若失败回填的是待解引用的已发布指针，必须证明取得其初始化内容的同步关系，并同时保护寿命，通常需要 acquire 读取。

## 规范入口

[N5050](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf) 的 [atomics.types.operations] 是 CAS 依据；[P0418R2](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0418r2.html) 解释失败序限制的历史修改。[P0493R5](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p0493r5.pdf) 第 5 节专门讨论条件写最大值与 RMW 契约的差异。提案是设计背景，最终接口以固定草案为准。
