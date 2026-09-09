# 源码导读 02：票据队列与分段子队列，先比较承诺再比较结构

同样是多生产者、多消费者，生产实现可能维护一条全局票据序列，也可能把工作拆成多个 producer 子队列。这不是同一个算法的两种写法：争用位置、失败含义和顺序保证都可能改变。本篇先追 Folly 的固定容量 MPMCQueue，再追 moodycamel 的 producer、block 和回收路径。不要把其中一个的性能宣传当作另一个契约下的结论。

本篇于 2026-09-08 只读核查一手源码，未编译或运行这两个库。固定版本为：

- Folly `v2024.11.18.00`，commit [`0660ead5b610047a279cc291129e98e3f2a1fc37`](https://github.com/facebook/folly/tree/0660ead5b610047a279cc291129e98e3f2a1fc37)。主入口 [MPMCQueue.h](https://github.com/facebook/folly/blob/0660ead5b610047a279cc291129e98e3f2a1fc37/folly/MPMCQueue.h)，等待器 [TurnSequencer.h](https://github.com/facebook/folly/blob/0660ead5b610047a279cc291129e98e3f2a1fc37/folly/detail/TurnSequencer.h)。以下先限定 `Dynamic=false`、普通不抛出移动路径。
- moodycamel `v1.0.4`，commit [`6dd38b8a1dbaa7863aa907045f32308a56a6ff5d`](https://github.com/cameron314/concurrentqueue/tree/6dd38b8a1dbaa7863aa907045f32308a56a6ff5d)。主入口 [concurrentqueue.h](https://github.com/cameron314/concurrentqueue/blob/6dd38b8a1dbaa7863aa907045f32308a56a6ff5d/concurrentqueue.h)，契约入口为该提交的 [README](https://github.com/cameron314/concurrentqueue/blob/6dd38b8a1dbaa7863aa907045f32308a56a6ff5d/README.md#reasons-not-to-use)。下面区分显式与隐式 producer，不把两种 block 生命周期混写。

教学锚点是 [Q0 锁基线](../queues/01-mutex-baseline.md)、[SPSC](../queues/03-spsc.md)、[有界 MPMC](../queues/05-vyukov-mpmc.md)和 [Michael–Scott](../queues/06-michael-scott.md)。当前课程实现分别在 [queue_versions.hpp](../../exercises/include/concurrency_study/queue_versions.hpp)与 [queue_linked.hpp](../../exercises/include/concurrency_study/queue_linked.hpp)。这是方案族的源码阅读，不要求把生产库复制进教学代码。

## 1. Folly：票据授予位置，turn 授予槽位使用资格

先找 `MPMCQueueBase` 的字段：`pushTicket_`、`popTicket_` 是两台票号分配器；`slots_` 指向 `SingleElementQueue` 数组；`stride_` 改变相邻票号对应的物理槽距离。`idx(ticket, cap, stride)` 计算物理槽，`turn(ticket, cap)` 用整数除法计算该槽的第几轮。票号唯一不等于槽位现在可以写：上一轮消费者可能还没完成析构。

`SingleElementQueue` 里真正保存元素的是 `contents_`，另有 `sequencer_`。第 r 轮生产等待 turn `2*r`，构造元素后完成该 turn；消费者等待 `2*r+1`，移出并销毁元素后完成该 turn。下一轮生产等待的就是 `2*r+2`。注意这不是为每次队列操作分配链式节点；固定容量数组一直存在，变化的是每个槽内 T 对象的寿命。[映射与单槽实现](https://github.com/facebook/folly/blob/0660ead5b610047a279cc291129e98e3f2a1fc37/folly/MPMCQueue.h#L1135)

沿 `blockingWrite` 追踪，可得到完整的成功路径：

```text
pushTicket_ 分配票号
  → enqueueWithTicketBase 计算 idx/turn
  → SingleElementQueue::enqueue 等待偶数 turn
  → 在 contents_ 构造 T
  → completeTurn 发布下一阶段
  → read 侧取得相应票号，等待奇数 turn
  → dequeueImpl 移出 T、destroyContents
  → completeTurn 允许同槽下一轮写入
```

本篇把票据发放理解为顺序位置的预订，把完成 turn 理解为使用阶段的交接。这一解释来自实际控制流，而不是“一个 fetch_add 发布了所有 payload”。构造、读出和销毁都发生在票据预订以后，所以元素操作不能随意抛异常留下永远无法完成的票据。源码的构造约束、不抛出移动路径和 `IsRelocatable` 分支必须分别看；后者是 Folly 扩展，不能当成任意 C++ 对象都允许 memcpy 搬迁。[元素路径](https://github.com/facebook/folly/blob/0660ead5b610047a279cc291129e98e3f2a1fc37/folly/MPMCQueue.h#L1360)

## 2. ready 与 promised：false 不是同一张“空/满快照”

`write` 调用 `tryObtainReadyPushTicket`：先看相应槽能否进入这一轮，再以 CAS 竞争票号。槽暂不可用时，它复读票号，区分“确实还是我刚检查的位置”与“竞争者已经推进位置”；竞争失败重试，不提前消费输入对象。成功才进入实际构造。因此 `write(std::move(x))` 返回 false 的这条路径没有调用 x 的移动构造。

`writeIfNotFull` 改走 `tryObtainPromisedPushTicket`，依据已发出的 push/pop 票据判断是否还有逻辑位置；它取得的位置可能依赖一个尚未完成的读操作。于是“逻辑上不满”仍可能需要等上一轮消费者释放物理槽。读取端也有 `read`/`tryObtainReadyPopTicket` 与 `readIfNotEmpty`/`tryObtainPromisedPopTicket` 的对应区别。[公开接口与说明](https://github.com/facebook/folly/blob/0660ead5b610047a279cc291129e98e3f2a1fc37/folly/MPMCQueue.h#L858)

一个可在纸上复验的交错是：P0 拿到早票后暂停，P1 在另一槽完成较晚票。此时 `read` 可以返回 false，因为最早待读票的槽未发布；不能把这次 false 解释成“所有已完成写入的集合为空”。`readIfNotEmpty` 可以预订读取位置，但必须等对应写操作完成。源码头部对线性化顺序的描述，不能用来抹掉各接口明确说明的暂时失败语义。

继续进入 TurnSequencer。`isTurn` 和循环状态读取使用 acquire；`completeTurn` 的 CAS 使用该调用的默认原子顺序，并在有等待者时调用 `futexWake`。`tryWaitForTurn` 先根据自适应阈值自旋，随后登记等待差值并进入 `futexWait`/`futexWaitUntil`。这些 futex 名字还经过 Folly 的平台抽象，不能在 Windows 上仅凭函数名断言实际使用 Linux 系统调用。[等待与完成](https://github.com/facebook/folly/blob/0660ead5b610047a279cc291129e98e3f2a1fc37/folly/detail/TurnSequencer.h#L117)

因此旧讲义把本队列统称为“槽位间无锁协调”不够准确：源码明确说明 MPMCQueue 不是 lock-free，部分接口等待其他操作完成。即使选择不会等待槽位的 ready 接口，也要单独讨论重试、元素操作和实际原子实现，不能为整个类型自动授予某种进展保证。

## 3. 缓存布局与退出路径也属于这条 trace

在字段区找 `alignas(hardware_destructive_interference_size)`，会看到 push/pop 票据和自适应阈值各有隔离；槽数组两端有 `kSlotPadding`。但每个槽并非都用一整条缓存线填满：`computeStride` 选择与容量配合的步长，将相邻票号映射得远一些。收益是可能减少伪共享，代价是映射复杂度和局部性变化；是否更快要另测，不引用头注释中的旧 benchmark 当作本课程实测。[布局与 stride](https://github.com/facebook/folly/blob/0660ead5b610047a279cc291129e98e3f2a1fc37/folly/MPMCQueue.h#L1032)

固定容量分支析构要求没有并发使用者。数组析构时，单槽析构通过未完成 turn 的奇偶识别是否仍有已构造元素，再销毁它。它不提供课程 bounded channel 的 `close()` 协议，也不会替永久等待的消费者自动发出结束消息。动态分支还保留旧 slots 数组及 ticket offset，不能把本文 `ticket/capacity` 的固定数组推导直接套过去；若选择该分支，先重新追 `trySeqlockReadSection`、`maybeUpdateFromClosed` 与扩容失败返回。

## 4. moodycamel：先选择子队列，再在其中取得元素

从 `enqueue(token, value)` 进入 `inner_enqueue`，它定位 `ExplicitProducer`；无 token 版本经 `get_or_add_implicit_producer()` 按线程身份找到或建立 `ImplicitProducer`。所有 producer 通过 `producerListTail` 和 producer 链连接，但各自有 `tailIndex`、`headIndex`、`tailBlock`、`dequeueOptimisticCount` 与 `dequeueOvercommit`。这是把生产者写入端拆开，不是给一条全局链加更多计数器。[公开入口](https://github.com/cameron314/concurrentqueue/blob/6dd38b8a1dbaa7863aa907045f32308a56a6ff5d/concurrentqueue.h#L995)

`ProducerToken` 保存特定 producer 指针；它不是锁。多个线程若共同使用同一个 token，仍要在外部串行交接 token 的使用。token 析构将 producer 标成 inactive，供队列回收或复用 producer 身份，不等于销毁整条子队列或其未消费元素。queue 析构才遍历 producer、索引与空闲块，并要求并发访问已经结束。[token 生命周期](https://github.com/cameron314/concurrentqueue/blob/6dd38b8a1dbaa7863aa907045f32308a56a6ff5d/concurrentqueue.h#L651)

`ConsumerToken` 则缓存 `currentProducer`、`desiredProducer`、偏移和消费计数，帮助在子队列之间轮转。无 token 的 `try_dequeue` 先启发式选择看起来有数据的 producer，失败后尝试其他 producer；带 token 的版本优先尝试当前 producer，再轮转。两者均不为不同 producer 的元素建立一个全局提交顺序。[消费者选择路径](https://github.com/cameron314/concurrentqueue/blob/6dd38b8a1dbaa7863aa907045f32308a56a6ff5d/concurrentqueue.h#L1120)

按该版本 README 的契约，同一个 producer 内的顺序被保留，跨 producer 不保证线性化的全局 FIFO。即便应用先同步 P0 完成 A，再让 P1 提交 B，两个不同 producer 子队列也不能据此要求全局先出 A。若应用把它们视为同一条顺序流，就要用同一 producer token 并在外部保证其交接，或选择另一个契约更强的队列。返回 false 也只是各流在本次观察中显得不可取，不能单独作为并发结束协议。[顺序限制](https://github.com/cameron314/concurrentqueue/blob/6dd38b8a1dbaa7863aa907045f32308a56a6ff5d/README.md#reasons-not-to-use)

## 5. 分段存储：显式 producer 的 block 环怎样扩展与复用

`Block` 以连续原始存储容纳 `BLOCK_SIZE` 个 T，另有 `next`、空标志或完成计数，以及用于全局 free list 的字段。`ExplicitProducer::enqueue` 在 tailIndex 跨 block 边界时，先检查 `tailBlock->next` 是否已空：若空就复用，否则准备 block index 容量，再经 `requisition_block` 取得新块，并把它接入 producer 自己的环。

这里出现两种不同索引：`tailIndex` 是元素逻辑位置；`blockIndex` 的 entry 保存块起点 `base` 和 block 指针，使消费者不用从头遍历 block 环。`new_block_index` 扩大索引时通过 `prev` 留住旧索引存储，直到 producer 销毁再释放。不能看见新索引已发布就立即 free 旧索引，因为旧消费者可能还在使用它。[显式入队及扩展](https://github.com/cameron314/concurrentqueue/blob/6dd38b8a1dbaa7863aa907045f32308a56a6ff5d/concurrentqueue.h#L1849)

跨 block 边界的发布顺序必须分构造分支看，不能统一写成“先构造，再发布索引”。若该 T 构造可能抛出，源码先在 try 中构造元素；成功后填写 block index entry、release 发布 front，再 release 发布新的 tailIndex 并返回。若构造确定不抛出，则跳过前面的 try 构造分支，先填写 entry、release 发布 front，随后在公共入队路径构造 T，最后才 release 发布 tailIndex。未跨 block 的路径则直接构造并发布 tailIndex，不需要为这次元素重发一个新块索引。[分支起点与两次发布](https://github.com/cameron314/concurrentqueue/blob/6dd38b8a1dbaa7863aa907045f32308a56a6ff5d/concurrentqueue.h#L1915)

因此，消费者看见 block index 的 front，只说明可以按该索引定位块，不代表块中的这个新元素已经构造完成或获准消费。尤其 nothrow 分支允许“索引已可见，T 尚未构造”的窗口。消费者仍要通过 dequeueOptimisticCount/dequeueOvercommit 验证消费额度，并由 tailIndex 的 acquire 读取及 headIndex 的 acquire-release 交接等完整协议取得相应元素的可见性；不能绕开这套协议，仅凭 front 的 acquire 就读取 payload。

若块或索引分配失败，返回 false；`CannotAlloc` 路径不为这次需要的新索引/块分配而继续。可能抛出的 T 构造若失败，则回退相关生产者位置，保留可以以后复用的新块，并重抛；不会把这次元素作为成功入队发布到新的 tailIndex。应用传入的对象是否被其失败的移动构造改变，仍取决于 T 的保证，不能许诺通用强异常保证。

`enqueue` 允许扩展，`try_enqueue` 选择禁止相应分配的路径；预分配容量受每 producer 分块和部分占用影响，不能等同于一个严格共享 N 槽数组。继续追 `requisition_block`，顺序是初始块池、全局空闲块、在允许时创建新块。[块来源与归还](https://github.com/cameron314/concurrentqueue/blob/6dd38b8a1dbaa7863aa907045f32308a56a6ff5d/concurrentqueue.h#L3035)

## 6. 取出、逻辑完成、块再利用不是同一个动作

`ExplicitProducer::dequeue` 先用 optimistic count 尝试取得消费额度，再 acquire 读取 tail 验证范围。若发现过度预订，增加 `dequeueOvercommit` 后返回 false；成功才通过 `headIndex.fetch_add` 取得实际元素索引，查 block index，移出并析构 T，最后 `set_empty<explicit_context>`。

因此增加 headIndex 只代表该元素已分配给一个消费者，不能代表它的对象已经销毁。`Block::is_empty` 必须检查空标志或完成计数，并通过相应 acquire fence 获取完成前效果，才能把整块交给生产者重用。若输出赋值抛出，Guard 仍析构原元素并设置空标志；这次调用可以抛异常且元素已经被消费，不能承诺“失败时元素仍留在队列”。[显式消费与 Guard](https://github.com/cameron314/concurrentqueue/blob/6dd38b8a1dbaa7863aa907045f32308a56a6ff5d/concurrentqueue.h#L1954)

隐式 producer 不沿用显式 producer 的常驻复用环：在 `ImplicitProducer::dequeue` 中，最后完成一整块的消费者清掉相应索引 entry 的 value，再把 block 交给 `add_block_to_free_list`。默认复用路径进入全局 FreeList；是否直接销毁动态块还取决于 `RECYCLE_ALLOCATED_BLOCKS`。部分占用的尾块、初始池的块、动态块的最终释放需继续看各 producer 和 queue 析构，不能用“每次 dequeue 都 free block”概括。[隐式完成与归还](https://github.com/cameron314/concurrentqueue/blob/6dd38b8a1dbaa7863aa907045f32308a56a6ff5d/concurrentqueue.h#L2550)

FreeList 的 ABA 防护也值得单独 trace：`try_get` 在读取候选的 next 前先增加 `freeListRefs`；`add` 用 `SHOULD_BE_ON_FREELIST` 标记归还意图，等引用条件允许时才重设 next 并重新链接。CAS 失败必须减少临时引用，必要时由最后释放者完成重新入链。它保护的是这个 free list 的 next 复用协议，不是全程序通用 HP，更不意味着可以在还有这些引用时把块任意 delete。[FreeList](https://github.com/cameron314/concurrentqueue/blob/6dd38b8a1dbaa7863aa907045f32308a56a6ff5d/concurrentqueue.h#L1438)

## 7. 和教学实现逐项对照

| 项目 | 教学锚点 | 本次生产源码路径 |
|---|---|---|
| 所有权 | Q0 在锁内完成整次操作 | Folly 分开票据位置与槽 turn；moodycamel 分开元素索引、对象完成、块复用 |
| 容量 | 固定 N 的环或动态逐节点链 | Folly 本篇固定 slots；moodycamel 多个部分占用 block 与可扩展索引 |
| 顺序 | 各教学版本分别声明 FIFO/LIFO 与暂时失败 | moodycamel 是 producer 内顺序；不能进入全局 FIFO 的同契约排名 |
| 失败 | 课程常限制元素不抛出 | Folly 用类型约束保住 turn；moodycamel 的消费异常仍完成原元素销毁 |
| 回收 | HP 延迟释放已摘除节点 | Folly 固定数组重用槽；moodycamel 另有 block 空证明与 free-list 协议 |
| 进展 | 核心与完整操作分别说明 | Folly 可等待；moodycamel 的分配、用户元素操作及配置要计入完整调用 |
| 退出 | 教学 channel 显式 close/join | 这里不能凭一次 false 或 token 析构代替关闭与使用者收束 |

## 8. 可复验阅读任务与答案

若已有对应仓库的只读检出，令 `$src` 指向它，先 `git -C $src rev-parse HEAD` 核对本篇 commit，再运行以下搜索；没有本地源码时，在上面的固定提交页面搜索同样符号即可，不要求安装依赖。

```powershell
rg -n 'blockingWrite|tryObtainReadyPushTicket|tryObtainPromisedPushTicket|dequeueImpl|computeStride' "$src/folly/MPMCQueue.h"
rg -n 'tryWaitForTurn|completeTurn|futexWait|futexWake' "$src/folly/detail/TurnSequencer.h"
# 将 src 改为 moodycamel 固定检出
rg -n 'inner_enqueue|ExplicitProducer|ImplicitProducer|requisition_block|dequeueOvercommit|set_empty|freeListRefs' "$src/concurrentqueue.h"
```

**任务 A：画同一个 Folly 槽连续两轮的交接，指出谁可以重用存储。** 答案：第 r 轮构造等待 2r，完成后允许 2r+1 的消费；消费完成析构才发布 2r+2，下一轮生产才能重用。提交票号、开始构造、消费预订都不能单独替代这个最后条件。

**任务 B：P0 预订后暂停，P1 已完成，为什么 `read` 的 false 不足以给 semaphore 消费端减计数？** 顺着 ready 与 promised pop 的两个函数画图。答案：ready 可能因早票槽尚未发布失败；成功写计数不能保证 ready pop 立即成功。要组合外部计数，必须选择并证明对应接口的等待协议，源码建议的 readIfNotEmpty 就会等待该预订位置。

**任务 C：向两个独立 producer 先后提交 A、B，能否要求全局先 A？** 从 README 的限制追到消费者选择 producer 的循环。答案：不能；这份队列契约没有跨 producer 线性化顺序。改成同一个 token 并外部串行使用，是改变生产者协议，不是打开一个排序开关。

**任务 D：一块的最后逻辑索引已经被取得，是否能立即覆写块？** 对照 headIndex 与 set_empty。答案：不能，取得索引的消费者可能仍在移出或析构；整块完成标志才授予重用资格。进一步分别指出显式路径重用 producer 环、隐式路径最后完成者归还全局池。

**任务 E：FreeList CAS 失败后为何还要处理引用计数？** 搜索 `fetch_sub(1)` 和 `SHOULD_BE_ON_FREELIST + 1`。答案：失败者先前取得了保护 next 稳定的临时引用，必须释放；它也可能是最后一个引用者，需要履行先前登记的重新入链请求。漏掉这一出口会让块无法复用，贸然重设 next 则会破坏其他观察者的读取条件。

**任务 F：跨 block 边界时，front 的 release 是否必然发布了新 T 的构造？** 在固定源码 L1915 起分别沿可抛与 nothrow 分支，标出 L1919/L1948 的构造、L1938 的 front 发布、L1942/L1950 的 tail 发布。答案：可抛分支先构造再发布 front；nothrow 分支先发布 front 再构造；两者最后都经 tailIndex 发布新消费范围。因此索引可见不等于元素可消费，必须继续追 dequeue 的消费额度、tail 获取和 headIndex 交接，不得把索引的同步作用扩大成任意 payload 的访问许可。

提交阅读答案时附具体 commit、函数链和自己标出的成功/失败分支。这里没有生产库吞吐数据，亦未对全部重载、动态 Folly 分支或所有 allocator/元素类型给出新的正确性证明。
