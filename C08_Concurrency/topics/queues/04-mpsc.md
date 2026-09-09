# 队列 04：MPSC 的预订位置不等于发布数据

现在让多个采集线程把结果交给一个写盘线程。消费者仍然只有一个，但生产者不再能独占 tail。若把 SPSC 的普通 tail 更新直接换成 `fetch_add`，生产者能得到不同号码，却不能保证排在前面的号码已经写完。队列需要分别记录“谁拥有位置”和“该位置目前处于哪一轮、能否读写”。

本篇使用有界数组的 MPSC 专用分支，代码是 [`queue_versions.hpp`](../../exercises/include/concurrency_study/queue_versions.hpp) 中 `sequence_ring<T,true>`，别名 `mpsc_ring<T>`。它与下一篇 MPMC 共用槽位协议，但单消费者路径在编译时去掉 dequeue CAS。它不是节点交换式 MPSC 队列，也不从别的算法继承进展保证。完整运行与答案见 [Q1](../../exercises/Q1_mpsc_queue/solution.cpp)。

## 1. 先给一格存储写出生命周期

容量取 C=4，第 0 格开始时 sequence=0。生产者票据 p=0 看见 sequence=p，表示这格属于本轮且允许写入。它先成功 CAS `enqueue_`，独占这张票，再写 data，最后 release 将 sequence 设为 p+1=1。消费者要读取票据 p=0，必须 acquire 看见 sequence=1；读完后 release 写 sequence=p+C=4。下一轮票据 p=4 恰好又映射到第 0 格。

这三个值分别表示不同所有权：

```text
sequence=p       允许本轮生产者取得并填写此槽
    预订 enqueue ticket；填写 data
sequence=p+1     允许本轮消费者取得并读取此槽
    取得 dequeue ticket；读 data
sequence=p+C     允许下一轮生产者复用此槽
```

生产者之间通过 CAS 仲裁唯一的 p；消费者之间无需仲裁，因为只有一个消费者。消费者仍要读写原子的 dequeue 计数，便于与 MPMC 复用布局，但该字段在本分支只有一个写者。决定 data 能否使用的是 sequence 上的 acquire/release，而不是 enqueue/dequeue 计数本身，所以抢票 CAS 可以 relaxed。

T 在构造时默认初始化，之后只赋值；要求复制赋值、析构不抛异常。如果一个生产者在取得票据后从赋值抛出，它已经占据的位置就没有可发布的有效值。因此不支持任意抛异常元素，也不允许测试钩子抛异常或永久放弃票据。传入的 value 在调用期间由生产者拥有，不能被别人同时改写。

这里的非抛复制具体指 `T& = const T&`。入队参数已是 const 源，出队从 `std::as_const(slot->data)` 复制，与静态约束一致；不能因源码都写着赋值，就忽略可变源重载可能不同。Q1 的双重载回归实际检查这条路径，类型与理由见[元素约束推导](02-bounded-and-batch.md)。

## 2. 确定性制造发布空隙

令 P0 取得票据 0 后暂停，尚未写 data；P1 取得票据 1，写入 20、发布并返回 true。此时从外部看，一个完整 push 已经结束，但消费者正在等待的是第 0 张票。它不能读 P0 的未初始化逻辑数据，也不能擅自跳过票据 0，于是本次 pop 返回 false，输出保持原值。

```text
P0: 调用 push(10) ── 预订票0 ──[暂停]──────── 发布票0 ── 返回
P1:                       调用 push(20) ── 发布票1 ── 返回
C :                                                   pop -> false
恢复 P0 后:                                             pop -> 10, pop -> 20
```

时间线中的关键关系是 P1 返回发生在消费者调用之前。若把 false 定义成严格线性化 FIFO 的“队列为空”，这个历史就无法解释：无论把仍在进行中的 P0 放在线性化序列的哪里，P1 的 20 都应当已经存在。把所有原子改成 SC 也不能填补 P0 尚未完成的普通赋值。

`queue_checks.hpp` 的 `check_reservation_gap` 正在实际队列的预订之后执行 noexcept 钩子。钩子通过 release/acquire 布尔信号告诉主线程“票已经拿到”；主线程完成第二次 push 并记录 pop 的结果，再放行 P0。检查在放行和 join 之后抛出，避免测试失败时把线程永远留在暂停点。全过程没有使用 sleep 猜测调度。

## 3. 重新写清契约

本分支支持多个生产者、一个消费者；容量必须是至少 2 的二次幂，且小于计数半范围。可用容量等于构造参数，没有隐藏减一格。默认票据是 `uint64_t`，其有限宽度前提会在[下一篇](05-vyukov-mpmc.md)展开。

成功项按取得的 enqueue 票据顺序被取出；同一生产者不重叠的连续调用保持原顺序。重叠的生产者调用可以按抢票先后排列，即使返回顺序相反。false 只表示当前观察未取得一个可用位置或可消费的下一项，可能是暂时失败，不能用来报告精确 empty/full，也不能据此断言全部工作已经结束。

单消费者 pop 的核心没有 CAS 重试，单次可以很快返回 false；这并不让整个传输协议变成 wait-free。只要 P0 长期不发布，消费者就无法越过它；后面最多填满有限个槽，之后也无法继续成功提交。我们既不把重复返回 false 说成“队列仍有有用进展”，也不把这个类标成严格 lock-free FIFO。

完整操作还受 T 与实际原子实现约束。构造后容器不为每次操作分配槽位，但 noexcept 赋值仍可能在内部阻塞。Reference 实测票据和槽位 sequence 的 `is_lock_free()`；这个输出只说明原子对象本身。

## 4. 从一次失败到安全结束

消费循环不能写成“碰到一次 false 就退出”。正确的教学驱动对每个生产者发布一个完成事件，且事件发生在它最后一次 push 返回之后。消费者先 acquire 确认所有生产者已经完成，再进行一次最终 pop 观察。对本实现，此时已经没有尚未发布的生产者票据；真正没有剩余可取得数据才能结束。完整代码见 [`transfer`](../../exercises/include/concurrency_study/queue_checks.hpp)。

这是驱动层结束协议，不是队列自带 close。若生产者可能继续接收工作，必须增加明确的关闭与取消设计，不能用一个“目前很空”的统计量替代关闭事件。worker 捕获异常后设置取消，其他线程的重试也会观察取消，主线程 join 后重新抛出异常。

## 5. 运行与练习答案

从 `C08_Concurrency/exercises` 执行已接入的叶项目：

```powershell
cmake -S Q1_mpsc_queue -B build/q1 -G "Visual Studio 18 2026" -A x64
cmake --build build/q1 --config Release
ctest --test-dir build/q1 -C Release --output-on-failure
```

Reference 在容量 8 的队列中验证基本容量契约，再让四个生产者合计发送 12007 个 ID。每个生产者拥有连续、不重叠的 ID 区间，单消费者保存顺序并验证每个生产者的下一项，最后检查完整集合。暂停实验则检验本篇的反直觉历史：第二个 push 已完成，第一次 pop 仍返回 false；恢复后必须依次读到 10 和 20。

**为什么不让消费者直接去读票据 1？** 那会改变既定顺序，而且需要另外记录被跳过的票据、保证随后补读并处理容量复用。本版没有这种协议，跳读会破坏它的状态机。

**为什么不使用单独的 ready 布尔值？** 槽位跨轮复用，同一个 true/false 不携带代次。sequence 同时表达当前轮和阶段，让取得 p 的线程可以确认看到的不是别的轮次。有限计数仍有回绕前提，不能宣称从此绝无 ABA。

**消费者没有 CAS，能直接允许第二个消费者吗？** 不能。两个消费者可能读取同一 dequeue 值并对同一个槽赋值到输出，还同时释放它。下一篇需要增加取票 CAS；角色约束是算法前提。

**原子全为无锁是否足以消除发布空隙？** 不足。阻碍来自尚未执行的普通数据操作，换原子硬件指令不能替暂停的线程执行它。

sequence 设计的算法出处是 Dmitry Vyukov 的 [bounded MPMC queue](https://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue)。本篇的单消费者裁剪、失败契约和暂停检查是本课程推导；作者站点在本轮在线核验时未能抓取，因此没有把页面的不可见内容当作本轮已核验引文。内存序规范参见 [N5050](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf) `[atomics.order]`。
