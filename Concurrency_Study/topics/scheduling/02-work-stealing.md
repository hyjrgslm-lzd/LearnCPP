# 调度 02：一条 deque 还不是一个线程池

前一篇用独立任务观察负载平衡。现在把任务换成递归求和：当前任务把左半边提交到池，自己计算右半边，最后合并两个结果。此时需要回答的不仅是“下一个任务从哪来”，还包括“父任务等谁、空闲线程何时睡、关闭时哪些任务还必须完成”。

完整实现是 [`cs::work_stealing_pool`](../../exercises/include/concurrency_study/work_stealing_pool.hpp)，递归调用和协议检查在 [M1 solution.cpp](../../exercises/M1_work_stealing_pool/solution.cpp)，多提交者与跨池检查在 [scheduling_test.cpp](../../exercises/runtime_tests/scheduling_test.cpp)。这是带锁教学实现，不是 Chase–Lev 无锁 deque，也不声称具备其算法或进展性质。

## 1. 先读公开契约

| 项目 | 本实现的约定 |
|---|---|
| 线程与容量 | 构造时指定正数 worker；队列无界，实际受内存限制 |
| submit(f) | 支持可移动 callable；调用方式为无参数左值调用，参数由 lambda 捕获 |
| 成功接纳 | callable 进入某队列后计为已提交；成功返回对应 `future<R>` |
| 失败 | 构造/分配失败向提交者抛出；关闭后提交抛 `runtime_error`，不会接纳 |
| 结果 | packaged_task 将返回值或用户异常存入 future；用户异常不杀死 worker |
| 队列顺序 | owner 从 back 取，窃取者从 front 取；没有全局 FIFO 或完成顺序保证 |
| shutdown() | 关闭全部提交入口，返回可重复取得的 `shared_future<void>`，已接纳任务排空后就绪 |
| join() | 外部线程调用；关闭、等排空、join 所有线程；多次调用允许，调用者间串行化 |
| worker 等待 | 仅对本池、非 deferred、良好嵌套的 fork-join 子任务使用 `pool.wait(f)` |
| 进展 | 锁、内存分配及用户代码都可能阻塞；不保证 lock-free、wait-free 或公平性 |
| 生命周期 | 不可从本池 worker 析构池；销毁前外部调用者必须停止访问对象 |

`shutdown()` 的 future 是“没有排队/活跃任务”的屏障，不是“所有 thread 已 join”的屏障，也不是任务成功与否的汇总。要观察任务异常，仍须 get 每个任务 future。要释放执行资源或确保 callable 包装对象都已销毁，调用 join，或让池在外部线程析构。

## 2. owner 和小偷怎样分工

每个 worker 有独立的 `mutex + deque<task>`。来自本池 worker 的提交压入自己的 back；外部提交轮转分配。worker 首先锁自己的队列，取 back；本地没有任务，再逐个检查其他队列，取 front。每次移出和删除都在同一队列锁下完成，因此一个任务不会同时被 owner 和小偷拿走。

对某条队列，可以把成功插入的生效点放在 `push_back` 完成处，把取出的生效点放在移除处。并发调用只能按同一把队列锁允许的顺序操作其中对象。这个线性化解释只针对一条队列的操作；所有 worker 开始执行和结束执行的顺序不受 FIFO 约束。

本地 LIFO 常被用于递归分治：刚生成的子任务可能访问仍然热的数据。front 上较早生成的任务有时代表较大的尚未展开子树，窃取它可能带来较粗的工作。但“最老”不等于“最大”，队列中也完全可以放常数代价任务。锁仍会被其他提交者和窃取者争用，不能把“两端访问”写成“零争用”。

本版按循环顺序扫描 victim，优点是易于推导，代价是多个空闲 worker 可能同时扫描相同队列。随机 victim、批量偷取和无锁 deque 是后续可测量的改进方向，不属于当前已经实现的保证。

## 3. 为什么不能只看所有队列是否为空

假设 A 已经把最后一个任务从 deque 取出，但还没有运行完。此时所有队列都为空。任务随后可能提交子任务，也可能继续使用捕获对象。如果池在这里宣布排空，调用者会过早释放资源。

实现用 `queued_` 和 `active_` 区分尚未交付执行的任务与正在运行的任务。提交持管理锁，再持目标队列锁，成功插入后增加 queued；取任务先持队列锁移出，释放队列锁，再持管理锁把 queued 减一、active 加一。执行结束后持管理锁把 active 减一。

有一个值得细看的过渡窗口：任务已经离开 deque，但取任务者尚未取得管理锁。此时它仍计入 queued。这个保守计数允许空闲 worker 短暂醒来后什么也没拿到，但不会把仍存在的任务漏计。管理状态的转换完成后，该任务从 queued 转为 active，计数总量仍覆盖所有未完成任务。

排空的谓词是 `closed_ && queued_ == 0 && active_ == 0`。递归合作式等待时，一个 OS worker 可能在调用栈里执行多层任务，因此 active 可能大于 worker 数；它数的是尚未返回的任务调用，不是 CPU 上同时运行的线程。

## 4. 条件变量需要一个不能丢通知的谓词

worker 尝试取任务失败后，拿管理锁，等待 `queued_ != 0 || signalled_`。提交者修改 queued 时也拿管理锁，再通知一个等待者。若提交发生在 worker 进入 wait 之前，谓词检查能看见 queued；若发生在 wait 释放锁之后，通知会唤醒它。通知本身不保存“任务数”，这个事实保存在谓词中。

完成任务后通知所有等待者，原因不只是可能排空，还包括某个 worker 正在合作式等待当前任务的 future。future ready 先由 packaged_task 建立，完成者随后取得管理锁，更新 active 并通知；等待者检查 ready 与进入 wait 也在管理锁下衔接。这把锁使“我刚检查到未完成”和“我开始睡眠”之间不会错过完成通知。

锁顺序同样重要：submit 是管理锁再队列锁；run_one 取出后必须先释放队列锁，才拿管理锁。若让 run_one 同时持有两把锁并逆序获取，就能形成 submit 与取任务者互相等待。用户 callable 在所有这些锁之外执行，允许它再 submit，也避免把任意用户耗时放进管理临界区。

## 5. shutdown 不等于取消

关闭在管理锁下设置 closed。并发 submit 若先完成接纳，就属于应排空任务；若先观察到 closed，则被拒绝。这是关闭和提交之间的边界。关闭之后连活跃任务继续提交子任务也会被拒绝；对应异常将经父任务 future 传回。

因此递归任务的常用顺序是：提交根任务，在外部 get 根结果，确认递归工作已经完成，再 shutdown/join。不能刚提交根就关闭入口，却期待它未来无限制地 fork。另一种“关闭外部入口，但允许内部派生”的契约可以设计，不过它需要区分提交角色和终止条件，不是本接口的隐含功能。

测试用 latch 让一个任务确实进入执行态，再调用 shutdown；这时即使 deque 空了，drain future 也必须未就绪。测试先解除 latch，再做可能抛出的检查，防止检查失败导致工作线程永远卡在测试门闩上。这里没有依赖 sleep 去猜测 worker 是否已经运行。

## 6. 递归等待为何会饿死，help 又解决了什么

考虑只有一个 worker。它执行父任务 P，P 提交子任务 C，然后直接 `C.get()`。C 在队列里，但唯一 worker 被 P 占用。此时不是 mutex 互相锁住，而是执行资源全部被等待者占住，导致饥饿死锁。多个 worker 也一样：只要每个 worker 都运行一个等待其队列子任务的父任务，所有执行槽位都被占住。

M1 的 sum 先提交左半，当前调用计算右半，然后调用 `pool.wait(left)`。wait 检查 future，未完成时先运行一个待执行任务；只有没有任务可取才进入条件变量等待。一个 worker 因而可以沿调用栈把子任务做完。Reference 在一个和四个 worker 上都检查了 0..4095 的和。

help 不是通用死锁消除器。任务持着某把锁等待子任务，而子任务也要这把锁，help 会在同一线程再次获取锁而阻塞；依赖循环也不可能凭调度消除。任意 DAG 与任意重入代码的证明更复杂，本池只承诺教学中的良好嵌套 fork-join 使用方式，等待时不得持有子任务需要的资源。跨池 wait 也不共享完成通知，不能把别的池或外部 promise 的 future 交给本池 worker 的 wait。

外部调用者直接 get 很正常。Reference 中用于确定“确实发生过窃取”的特殊测试有两个 worker：root 提交 child 后等由 main 发出的释放信号，另一个 worker 完成 child，main 再释放 root。这是有限的测试握手，依赖明确保留的第二个执行槽位，不是推荐的任意嵌套等待模式。普通课程不运行故意挂死的反例。

## 7. 对象生命周期和构造中途失败

队列、锁、条件变量、计数和 promise 都在 workers 成员之前声明。更关键的是构造函数启动线程的循环有 catch：如果第 k 次 thread 创建失败，先关闭并通知已启动 worker，再逐个 join，最后重抛。仅靠成员析构不够，joinable 的 `std::thread` 析构会终止程序；换成 jthread 也不能自动唤醒一个从不检查 stop_token 的条件变量等待。

析构从外部 join；在本池 worker 上显式调用 join 抛 logic_error，自身析构则违反前提并终止。调用者还要保证 lambda 中按引用捕获的对象活到任务结束。future 对共享结果状态有所有权，并不延长任意被捕获引用的目标寿命。

检查程序本身也要遵守同样规则。runtime 按 `seen/errors → pool → producers` 声明，异常展开时反向执行：先 join 仍可能 submit 或写 errors 的 producers，再让池排空仍可能访问 seen 的任务，最后销毁数据。不能只在成功路径手工 clear/join，却让线程创建失败绕过这个顺序。

`inject_creation_and_submit_failure` 让第一个 producer 接纳一个等待门闩的任务，然后在第二个 producer 启动位置注入 system_error；异常处理先打开两道门闩再重抛。仍活着的 producer 同时用本线程的单次 operator new 注入，让真实 pool.submit 的 make_shared 抛 bad_alloc。析构顺序观察器在 pool 已销毁但 seen/errors 尚存活时记录任务结果和 producer 异常；外层分别检查原 creation 异常未被替换、bad_alloc 被保存、已接纳任务恰好完成。这是确定性注入，不是操作系统真的耗尽线程资源。

## 自测与答案

**shutdown future 就绪是否说明任务全成功？** 不说明，只说明全部任务调用已经返回。失败结果仍留在各自 future 里；忽略它们就丢失了业务失败信息。

**为什么局部队列仍有一把全局管理锁？** 当前版本优先建立清楚的接纳、计数和睡眠协议。每次提交、领取后记账、执行完成都经过它，所以它可能限制吞吐；本地 deque 不自动消除全局协调成本。

**能否检查 steals 必须小于本地命中？** 不能。外部提交分布、递归结构和 OS 调度都会影响比例。检查应验证任务唯一执行和关闭协议；窃取次数只能用于解释运行现象。

**测试通过是否覆盖了线程创建失败？** runtime 已在 producer 创建路径注入失败，并组合真实 submit 分配失败检查收束；没有让 OS 真的耗尽线程资源，也没有注入 pool 构造函数内部的线程创建失败。后者仍依据关闭、通知、join 和成员寿命推导，不得把一个注入点推广成所有异常路径已验证。

## 依据与下一步

- [N5050](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf)：`[thread.condition.condvar]`、`[thread.thread.destr]`、`[futures.task]` 是等待、线程析构和异常传递的规范入口。
- [Chase 与 Lev，Dynamic Circular Work-Stealing Deque](https://doi.org/10.1145/1073970.1073974)：原算法的环形数组、扩容和并发边界需要独立学习，本版不实现它。
- [oneTBB 官方：任务调度如何工作](https://uxlfoundation.github.io/oneTBB/main/tbb_userguide/How_Task_Scheduler_Works.html)：可对照工业运行时的工作窃取和等待行为，不能据此给本教学池附加未实现的保证。
