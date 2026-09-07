# 队列 05：Vyukov 有界 MPMC 的票据、代次与边界

MPSC 已经解决了多个生产者分配不同位置的问题。现在增加消费者数量，原先那个“消费者独占 dequeue”的假设不再成立。最直接的变化是在消费端也使用 CAS 抢唯一票据，而不是让两个消费者都读取同一格。这个变化不会自动消除发布空隙，也不会改变有限计数的数学性质。

实现是 [`queue_versions.hpp`](../../exercises/include/concurrency_study/queue_versions.hpp) 的 `sequence_ring<T,false>`，别名 `mpmc_ring<T>`；[Capstone2 Reference](../../exercises/Capstone2_lockfree_queue/solution.cpp) 真正执行 CAS/sequence 协议，已没有内部 mutex 占位实现。数组、SPSC 和 MPSC 的背景分别见前面三篇。

## 1. 两端分别抢什么

生产者首先 relaxed 读取 `enqueue_` 得到候选 p，acquire 读取 `cells[p & (C-1)].sequence`。相等表示该代槽位可取得；CAS 把 enqueue 从 p 改到 p+1 成功后，当前生产者成为该轮槽位唯一写者。失败时 CAS 更新 p，循环重新选择对应槽并读取 sequence，不能继续使用失败前的槽指针。

消费者的候选来自 dequeue，要等待的 sequence 是 p+1。CAS 把 dequeue 从 p 改到 p+1 成功后，它成为该轮唯一消费者。复制 data 到自己的输出后，release 写回 p+C，归还下一轮空间。两个消费者即使同时看到正确 sequence，最多一个能用相同 p 成功修改 dequeue。

抢票 CAS 是 relaxed，因为它只分配独占资格；载荷可见性由同一槽上的 sequence 交接建立。生产者 acquire 消费者上轮的归还，确保旧读取完成后才覆盖；消费者 acquire 生产者本轮的发布，确保写好后才读取。两个方向缺一不可。对非原子的 T 赋值与读取，只要这条所有权链完整，就不需要把 T 本身改成 atomic。

## 2. sequence 为什么有三个比较结果

先在不回绕的纸面模型中看生产者。若 seq=p，就轮到当前票据；若 seq 落后于 p，该槽仍未释放到本轮，当前尝试返回 false；若 seq 超前于 p，候选票据已经过时，应重新读取 enqueue。这些是相对于候选 p 的局部判断，不是一个额外加锁得到的全局 size 快照。

消费者与 p+1 比较同理。落后可能代表下一项还没发布；即使别的较晚 push 已返回，也仍然可能失败。生产者侧也可能被一个已抢到 dequeue 票、尚未复制完成并归还槽位的消费者堵住。因此不能把两个落后分支直接翻译成严格全局“满”和“空”。

一张票的逻辑路径是：

| 阶段 | 槽 sequence | 谁能推进 | 数据规则 |
|---|---|---|---|
| 本轮空槽 | p | 一个生产者 CAS 抢 p | 抢到前不能写 |
| 已预订未发布 | 仍是 p | 该生产者 | 其他消费者不能读取 |
| 已发布 | p+1 | 一个消费者 CAS 抢 p | 抢到后才能复制 |
| 已取票未归还 | 仍是 p+1 | 该消费者 | 下一轮生产者不能覆盖 |
| 下一轮空槽 | p+C | 下一轮生产者 | 重复相同交接 |

这个表也解释了异常限制：取得票据后的 T 复制赋值不能抛异常。析构也不能抛；构造期可能分配失败，此时队列尚未进入并发使用。固定数组不在每次 pop 时销毁 T，而是等待下轮赋值覆盖，最后析构整个数组。只可移动类型不在当前接口范围内。

要落实这个限制，实际源表达式必须和 trait 一致：入队 value 是 `const T&`，出队使用 `std::as_const(slot->data)`。一个同时定义非抛 const 赋值与会抛可变源赋值的 T 不会误走后者。Capstone2 的 `check_const_copy` 用这种类型运行出入与复用，并核对最终寿命计数；[预分配篇](02-bounded-and-batch.md)给出完整重载推导。

## 3. 为什么拒绝容量 0、1 和非幂次

容量 0 没有可寻址槽，掩码会下溢。非幂次不能使用 `p & (C-1)` 做映射。容量 1 更隐蔽：生产者发布 p+1，同时恰好等于下一轮生产者 p+C 所等待的“空槽”值，同一个 sequence 值混淆了可读与可写两个状态，第二个生产者可能覆盖尚未读完的元素。不是“留一格浪费”导致拒绝 1，而是本协议的状态编码要求 C 至少为 2。

构造函数在分配前执行 `valid`，明确抛出 invalid_argument；Reference 在 Release 下用 `cs::check` 验证 0、1、3、6 都被拒绝，不依赖会被 NDEBUG 删除的 assert。

仅把掩码替换成 `% C` 也不是无条件支持任意容量的完整修复。有限宽度票据回到零时，计数模数是 2^w；当 C 不整除这个模数，映射不再无缝接到原来的槽代次。若只在完全不回绕的有限运行里讨论任意容量，可以另作证明；本实现保留二次幂约束。

## 4. 有限计数不是无限整数

默认 Counter 是 uint64_t，但算法不能因为“很久才回绕”就忽略回绕。原始常见写法把两个无符号值先转有符号再相减，若没有明确范围约束，还可能遇到有符号溢出。本实现先在无符号域求模差：`Counter(observed-expected)`，最高半范围表示“落后”；比较函数是 `sequence_behind`。

这只是循环序号的局部顺序。必须保证比较的真实距离严格小于 2^(w-1)，容量也小于半范围。更具体地说，任何仍要用于判断或 CAS 的旧票据、旧 sequence 观察，都不能在其他线程前进半个计数范围后继续使用；完整计数周期后，旧值会再次与新值相同。操作系统若允许任意长暂停，程序本身并不能无条件保证这个前提。默认 64 位使约束在通常有限实验中有很大余量，但没有把它变成数学上不可能违反。

[`check_narrow_wrap`](../../exercises/include/concurrency_study/queue_checks.hpp) 将同一个模板的 Counter 换为 uint8_t、容量设为 4，顺序运行 4096 次 push/pop，真实经过 16 个完整周期。每次操作完成后才开始下一次，因此没有跨周期旧快照。它还枚举所有 256 个起点和 1 至 127 的距离，验证模比较方向。最后保留一个旧 ticket，再递增整整 256 次，检查新旧值确实相等。前两个检查支持有前提的回绕运算，最后一个提醒前提不能删除。

无堆节点意味着这里没有逐节点 delete 引入的悬空解引用，但有限整数仍可出现代次混淆。故“有界数组天然根除一切 ABA”的旧说法必须改正。

## 5. 顺序与进展保证分开说

成功的 enqueue 按票据分配顺序组织，成功 dequeue 按读取票据顺序对应它们。两个消费者的返回顺序可能相反：C0 先拿到前一个元素后暂停，C1 处理后一个元素并先返回。将输出日志排序不能还原线性化顺序，必须记录调用和返回区间。

包含严格 empty/full 失败语义的线性化 FIFO 不是本版承诺。[MPSC 篇](04-mpsc.md)的暂停历史同样适用于 MPMC；Reference 实际暂停第一个预订者，完成第二次 push 后得到 false。这足以反驳“完整严格 FIFO 且 lock-free”的说法。CAS 失败说明候选状态不合适，weak 还允许伪失败；即使某次 CAS 成功，也可能只是取得一张尚未发布的票，不能把它当成一次完成的用户操作。

核心算法可以受到停顿持票者影响，整个成功传输不承诺 lock-free；单线程也可能反复重试，所以更不能声称 wait-free。`is_lock_free()` 实测的是计数和槽 sequence 原子，不能改变这些算法事实。

## 6. 运行与验收答案

```powershell
# 在 Concurrency_Study/exercises 下
cmake -S Capstone2_lockfree_queue -B build/cap2 -G "Visual Studio 18 2026" -A x64
cmake --build build/cap2 --config Release
ctest --test-dir build/cap2 -C Release --output-on-failure
```

容量 2、4、64 分别检查顺序填充与复用；并发覆盖 3P/1C 的生产者顺序、1P/4C 的 SPMC 拓扑、3P/4C 的 MPMC 完整 ID 集合，每轮 10003 个元素。SPMC 是本实现允许的一种运行拓扑，未实现或宣称专用 SPMC 最优算法。独立历史测试再搜索最多 12 个调用的串行解释，对暂时失败采用允许 false 的模型。

**CAS 成功改成 release 能代替 sequence 的 release 吗？** 不能，抢票发生在写 data 之前，不能发布后面的尚未执行写入。

**谁应该对 p+C 做 release？** 读完当前 data 的消费者。它发布的是存储可以复用这个事实。

**能否从消费者总数相等推出不丢不重？** 不能。丢一个同时重复另一个，计数不变；Reference 保存 ID 后排序逐项检查。多消费者的完整顺序另由历史验证，不用集合检查冒充。

**把 yield 换成 atomic_wait 是否就完成阻塞队列？** 没有。还需要不会丢通知的等待谓词、关闭与取消协议，且等待者必须能等到相关 sequence 真正变化。这里的 try 接口没有承诺这些扩展，不提供未经证明的等待包装。

算法出处为 [Vyukov 的 bounded MPMC queue](https://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue)；本站本轮抓取失败，暂停反例与模比较结论由本课程代码、实际检查及上述推导给出。规范的原子与整数运算依据见 [N5050](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf) `[atomics.order]`、`[basic.fundamental]`。
