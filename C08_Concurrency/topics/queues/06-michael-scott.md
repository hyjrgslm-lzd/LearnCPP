# 队列 06：Michael–Scott 的帮助推进与真实回收

固定数组给了前面几篇一个稳定的存储空间，但容量也成为明确限制。现在考虑一个动态队列：生产者根据需要申请节点，消费者移除节点。接口看起来仍然是 push/pop，内部却多出一个比指针更新更难的问题：已经离开队列的节点，可能仍被另一个线程读取。

本篇实现是 [`queue_linked.hpp`](../../exercises/include/concurrency_study/queue_linked.hpp) 的 `ms_queue<T>`，Reference 是 [Q2 的 solution.cpp](../../exercises/Q2_ms_queue/solution.cpp)。它使用经典 Michael–Scott 连接和帮助推进结构，并接入课程现有 `cs::hazard_pointer`。它没有通过永久保留节点回避回收，停止后会释放剩余链条与所有本队列退休节点。

## 1. 为什么保留一个 dummy

初始化只有一个不含值的 dummy，head、tail 都指向它。逻辑元素存放在 head 之后的节点中。插入 10、20 后结构如下：

```text
head -> [dummy] -> [10] -> [20] -> null
                           ^
                          tail
```

出队 10 时，将 head 从旧 dummy 移到节点 10；节点 10 成为新的 dummy，逻辑第一个元素是 20。旧 dummy 退休。出队并不是直接删除“刚读出值的那个节点”，这是理解保护需求的第一步。

本实现用 `const optional<T>` 表示 payload，初始 dummy 的 optional 为空，后续节点在构造时含值。节点变成 dummy 后保留它的原值，直到这个节点真正被回收。这样同一个 payload 一经发布就保持不变。队列不提供从中取得可修改引用的接口。

## 2. 入队在哪一步完成

生产者先用 unique_ptr 构造新节点，然后保护当前 tail，再读取 tail->next。如果 next 为空，就 CAS 把它链接到自己的新节点。这个连接 CAS 是成功 enqueue 的线性化点：从这一刻起节点属于队列，不再由局部 unique_ptr 删除。

随后尝试把 tail 向前移动。这个 CAS 即使失败，enqueue 仍然成功，因为其他线程可能已经帮助 tail 前进。反过来，如果初次观察到 next 非空，说明 tail 落后，当前线程先帮助它前进一步再重试。发布内容与分配 ticket 不同：成功连接发生在节点已经完整构造之后，其他线程不需要等待某个持票者继续写 payload。

tail 自身可能正在变化甚至其旧节点已经退休，因此解引用 tail->next 前要有保护。这里只用一个 HP 槽保护 tail。next 作为 tail CAS 的候选值使用，入队端不解引用它；若受保护的 tail 已不再是根，CAS 失败。由于被保护的旧 tail 不能被释放并以同地址冒充新 tail，这个 CAS 不会被该地址复用骗过。

节点分配、元素复制或 HP 槽申请发生在链接之前，失败时 unique_ptr 自动删除尚未发布节点。链接后只做不抛异常的指针更新和返回，不能在已经生效的 enqueue 后再从普通错误路径撤销所有权。

## 3. 出队为什么需要两个保护槽

消费者会访问两个对象：旧 head 的 next，以及 next 节点中的 payload。因此有 `head_guard` 与 `next_guard` 两个同时存活的句柄。先保护 head，再从 `head->next` 保护 next，最后重新读取根 head 验证它仍然是自己观察到的那个节点。

最后这次根验证非常重要。设旧 dummy H 已被别的消费者移除，但我们用 HP 保住了 H 的生命；H->next 仍指向 N。更多消费者可以继续把 N 移除并退休，H->next 却不会因此改变。因此仅仅确认“H->next 还是 N”不足以确认 N 仍可从当前队列安全取得。实现必须在 next 保护发布之后检查根 head 仍为 H，且在这个检查成功之前不解引用 next。

课程 HP 要求所有作为保护源的原子读、写、CAS 都用 SC，包括 head、tail、节点 next。沿 SC 顺序看：若根在验证前已经推进，验证失败而重新开始；若根验证成功，随后发生的摘除与扫描必须面对已经发布的 next 保护。旧 head 本身也被保护，不能先释放并复用成同地址来伪装根未变。这里不能仅凭“用 acquire 读也能看见数据”就削弱源原子的内存序；可见性与回收握手是两个需要同时满足的协议。

通过根验证后，next 为空可以返回 false，线性化点在这次已验证观察中的空 next 读取。如果 head==tail 而 next 非空，tail 只是落后，需要先帮助 tail 前进。否则尝试 CAS head 到 next；成功时逻辑队首被移除，失败则从头重新保护，不得直接解引用 CAS 自动写回的未保护指针。

## 4. 成功 CAS 以后也不能随意 move

常见直觉是“我已经赢得出队，所以这个 payload 归我，可以 move 走”。这个结论在本结构中不成立。另一个消费者可能仍在读取同一个 next 的 payload，或者正在用它准备自己的出队判断。HP 只保持对象生命，不会给 payload 加互斥锁；移动 `string` 等对象会修改源对象，与其他读者冲突。

本实现让节点 payload 永远只读，出队成功后在 next_guard 保护下复制到调用者输出。T 要求复制赋值不抛异常、析构不抛异常，构造节点时的复制构造可以抛，抛出发生在发布前。T 不必默认构造，因为初始 dummy 的 optional 可以为空。T 的 const 读取/复制也必须支持并发读取，不能用 mutable 字段在内部偷偷无同步修改。只可移动类型不在此接口承诺内。

`*next->value` 从 const optional 自然得到 `const T&`，与 `is_nothrow_copy_assignable_v<T>` 检查的源类别一致。Q2 另用[双重载回归类型](../../exercises/include/concurrency_study/queue_checks.hpp)实际验证：即使存在会抛异常的 `operator=(T&)`，出队仍选择 const 重载，并在停止后释放全部该类型节点。这个检查保留 MS 已有的不可变 payload 机制。

将复制放在 CAS 后还有一个好处：失败重试不会反复修改用户输出。本实现 false 时输出保持原值；成功 CAS 后赋值不抛，保证不会出现“元素已经移除，调用者却只收到一个异常”的半完成状态。next_guard 在整个复制期间都有效，即使其他消费者已经让 next 也成为退休节点，仍然不能释放它。

## 5. 摘除、退休、释放是三个时刻

CAS head 成功表示旧 head 不再是逻辑队列的一部分。接着对旧 head 调用 `retire()`，把删除责任交给 HP 域；函数返回时两个局部保护句柄析构，结束当前线程保护。达到阈值的退休操作可触发扫描，扫描只能删除已经退休且不被任何 HP 保护的节点。

当前共享教学域最多有 128 个保护槽。单次 enqueue 同时需要 1 槽，单次 dequeue 需要 2 槽；Treiber pop 还会共享此预算。槽不足可能在操作生效前抛出 bad_alloc。队列不保留线程本地的永久句柄，每次调用结束释放槽。供主线程集成的接口需求是现有 `make_hazard_pointer`、`protect`、`retire` 和 `hazard_pointer_cleanup()`，无需另加第三个槽或改公共头。

最终清场要求所有调用者已停止并 join、所有局部保护句柄已经释放。析构函数直接删除仍在当前 head 链条里的活节点，随后调用 cleanup 处理已退休节点。活链条与退休集合互不重复，不能对同一个节点既 delete 又 retire。如果程序在别处仍持有同一全局 HP 域的其他对象保护，cleanup 不保证替那些对象结束保护；本队列不向外暴露内部节点，因此自己的调用全部结束后已无节点保护残留。

## 6. 进展保证与原始算法的距离

在节点生命周期正确、底层指针原子无锁、节点已准备好的抽象模型中，MS 的连接/帮助推进核心是经典 lock-free 算法，单个线程仍可能持续失败而不是 wait-free。与 MPMC 预订空隙不同，已经链接的节点内容完整，落后的 tail 可以由别人帮助修正。

但这份 C++23 Reference 的完整操作包含 new、HP 槽扫描、退休记录分配、mutex 保护的退休表、串行回收和 T 的析构。完整 API 不承诺 lock-free。`atomics_lock_free()` 只在静止状态检查 head/tail/next 的原子属性并打印。无界也不等于不会失败：内存耗尽或 HP 槽不足可能抛异常，try_push 并没有通过返回 false 隐藏它们。

[Michael–Scott 作者原始伪代码](https://www.cs.rochester.edu/research/synchronization/pseudocode/queues.html)明确附有特殊分配器前提，并给出 HP 等回收方向。本篇使用真实 HP 而非照抄伪代码的 free；原页面在 CAS 前读取 payload 是在其内存管理假设下的安排，本实现通过第二个保护槽支持 CAS 后复制。不要把两套保护前提拆开混用。

## 7. 用暂停读者检查回收

```powershell
# 在 C08_Concurrency/exercises 下，叶项目已接入
cmake -S Q2_ms_queue -B build/q2 -G "Visual Studio 18 2026" -A x64
cmake --build build/q2 --config Release
ctest --test-dir build/q2 -C Release --output-on-failure
```

Reference 首先检查基本 FIFO，再跑 4P/1C 的生产者顺序、1P/4C 的 SPMC 和 4P/4C 的完整 ID，工作量足以触发运行期间多次退休扫描。它没有用“程序退出时操作系统会收回内存”替代回收。

然后构造两个带析构计数的 payload 10、20。慢消费者取得 10 并成功 CAS head 后，在读取 payload 前暂停；快消费者取出 20，并把保存 10 的节点退休。此时主动 cleanup，10 的节点析构计数必须仍为零。恢复慢消费者后，它仍能读到 10；等待结束保护后再次 cleanup，退休节点才被析构。检查先释放暂停信号再抛异常，worker 异常通过 future 回传。

最后顺序传输 1000 个非平凡 payload，cleanup 后只允许输入、输出和当前 dummy 所保留的三个 payload 存活；再留下一个未消费元素交给队列析构。离开作用域后 live 必须为零。这分别检查运行中回收、残留 dummy、非空析构和最终退休表清场。它们是具体交错证据，不替代上述保护推导或独立审查。

## 自测与答案

**一个 HP 只保护 head，为何不够？** head 的生命周期不会自动延长后继节点的生命周期。后继可能已被别的消费者移除；保护可达的第一个节点不是保护整条链。

**为什么 head->next 的二次验证也不能替代根验证？** 已退休 head 的 next 可以永远保持旧值，而其后继已经退休。根验证确认当前拓扑仍然支持这个 next 的保护取得。

**入队 tail 更新失败是否要删除新节点？** 不能；链接 CAS 已把所有权交给队列。tail 更新只是帮助维护快捷入口，不是插入是否成功的标志。

**可以在 pop 后立即 delete 旧 head 吗？** 不能。另一个线程可能正用它读取 next，必须先 retire，再由扫描确认没有保护者。

**能够和容量 1024 的数组版直接排名吗？** 不能作为相同容量契约的排名。本版无界且含节点分配与回收，基准要求显式 `--capacity 0`，单独说明场景与计时边界。

C++26 的 HP 规范形状固定参考 [N5050 的 `[saferecl.hp]`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf)。`cs` 域的全 SC、固定槽数、cleanup 和锁行为是课程实现约定，不应写成标准 HP 必须如此。
