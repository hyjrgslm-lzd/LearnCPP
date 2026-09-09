# 源码导读 05：Folly 的保护记录缓存、退休分片与 RCU 宽限期

课程 HP 基线用清楚的 SC 协议解释“保护—复核—退休—扫描”，但生产实现还要面对记录申请争用、批量扫描、线程退出和对象所属容器的最终销毁。把这些机制统称为“更可扩展”还不够，本篇分别追它们改变了哪个共享位置、由谁接管待回收对象，以及为什么不能因此省掉应用的生命期义务。

固定 Folly `v2024.11.18.00`，commit [`0660ead5b610047a279cc291129e98e3f2a1fc37`](https://github.com/facebook/folly/tree/0660ead5b610047a279cc291129e98e3f2a1fc37)。2026-09-08 只读核查，未构建、运行 Folly，不将这个旧版本当作部署推荐。对应课程的 [HP 正文](../reclamation/02-hazard-pointers.md)、[RCU 正文](../reclamation/05-rcu.md)、[hazard_pointer.hpp](../../exercises/include/concurrency_study/hazard_pointer.hpp)和 [rcu.hpp](../../exercises/include/concurrency_study/rcu.hpp)。

真实源码入口是 [Hazptr.h](https://github.com/facebook/folly/blob/0660ead5b610047a279cc291129e98e3f2a1fc37/folly/synchronization/Hazptr.h)，但实现分散在 Holder、Domain、Obj 与 ThrLocal 头中。后半篇追 [Rcu.h](https://github.com/facebook/folly/blob/0660ead5b610047a279cc291129e98e3f2a1fc37/folly/synchronization/Rcu.h)，保留旧 18 的 RCU 阅读知识，同时纠正该版本真实接口与旧称谓的差异。

## 1. 先区分三种所有权

`hazptr_holder` 拥有一个保护记录 hprec_ 的使用权；hazptr_domain 管理记录总链、可用记录和退休对象；被保护对象在结构算法摘除以前仍由应用数据结构拥有。holder 并不拥有 T，也不负责把它从链表摘下。

从 `make_hazard_pointer(domain)` 进入：如果启用线程缓存且选择默认域，先尝试 `hazptr_tc_tls().try_get()`；没有可用记录则 `domain.acquire_hprecs(1)`，必要时创建新记录。holder 析构先清空记录中的保护地址，再尝试放回 TLS 缓存，失败才 `release_hprec` 回到 domain。[holder 获取与释放](https://github.com/facebook/folly/blob/0660ead5b610047a279cc291129e98e3f2a1fc37/folly/synchronization/HazptrHolder.h#L69)

TLS 的 `hazptr_tc` 在这个提交中容量为 9，缓存的是默认域的 `hazptr_rec`，不是每个线程独占的一袋退休对象。线程缓存析构通过 evict 把记录链还给默认域。由此能解释本地复用减少 domain 记录申请争用，但不能说“线程退出就把它退休的所有对象都释放了”。[线程记录缓存](https://github.com/facebook/folly/blob/0660ead5b610047a279cc291129e98e3f2a1fc37/folly/synchronization/HazptrThrLocal.h#L62)

domain 的 hcount_ 是记录管理的一部分，不能简单代称当前正在解引用对象的线程数。拿着一个空保护的记录、缓存中的记录、正在保护地址的记录，是不同状态；教学固定 128 槽的容量契约也不能搬来给 Folly 设上限。

## 2. protect 的复核没有因“成熟库”而消失

`protect(src)` 先 relaxed 读候选，再循环 `try_protect`。后者保存原候选 p，公布经过过滤后的保护地址，执行 light fence，再 acquire 重读 src；若 p 与重读值不同，清空保护并返回 false，同时让调用者取得新的候选。只有成功返回才可以依靠这次保护去使用对象。[protect 与 try_protect](https://github.com/facebook/folly/blob/0660ead5b610047a279cc291129e98e3f2a1fc37/folly/synchronization/HazptrHolder.h#L109)

这保留了课程中最关键的窗口分析：第一次读到地址后可能暂停，写者可以在登记之前摘除对象；后来的保护登记不能追认已经结束的寿命。失败回填指针也还没有成功保护，不能在下一次复核前解引用。

生产版本还有指针规范化：holder 将对象指针转换到其 hazptr_obj 基类地址，Domain 匹配 `raw_ptr()`。课程教学版明确比较完整对象地址，故不能把 Folly 的带基类偏移处理和教学版的地址约定混用。Filter 重载可辅助处理指针标记位，也不授权在保护成立之前访问对象字段。

## 3. light/heavy 配对不能只摘半边抄走

这个提交在读侧使用 `asymmetric_thread_fence_light`，收集侧取得退休候选后使用 heavy，再扫描保护记录。查看 [AsymmetricThreadFence.h](https://github.com/facebook/folly/blob/0660ead5b610047a279cc291129e98e3f2a1fc37/folly/synchronization/AsymmetricThreadFence.h)：非 Linux 分支调用普通 atomic_thread_fence；Linux 的 light 使用编译器层的 memory barrier，heavy 则进入 [AsymmetricThreadFence.cpp](https://github.com/facebook/folly/blob/0660ead5b610047a279cc291129e98e3f2a1fc37/folly/synchronization/AsymmetricThreadFence.cpp) 的进程级 membarrier 或 mprotect 机制。

这里的低读侧成本依赖完整的平台协议，不是把课程 SC store 改成 relaxed、加一个编译器 fence 就能得到的可移植结论。要声称某个目标平台真正走了哪条分支，还需要编译配置与运行环境证据；本篇只核查选择结构，没有执行这些系统路径。

复验时把源码中的三处顺序标出来：读者公布保护后复核；退休列表发布前的 light fence；收集者摘取退休候选后、读取 hazard 值前的 heavy fence。只关注 hazard store 的 memory_order 会遗漏其他两条腿。

## 4. retire 到实际删除：批次先属于谁，再判断保护

本篇主追 intrusive `hazptr_obj_base<T>::retire`：pre_retire 检查重复退休并保存删除器，set_reclaim 安装具体回收回调，再 push_obj。无 cohort 的对象进入 domain；有 cohort 的对象先归所属 cohort 处理。应用必须已经按结构协议摘除对象，retire 不会替你修改 head 或 next。[对象退休入口](https://github.com/facebook/folly/blob/0660ead5b610047a279cc291129e98e3f2a1fc37/folly/synchronization/HazptrObj.h#L499)

domain 的 `push_list` 把退休列表放进 untagged/tagged 分片并更新计数。该提交有 8 个分片，触发阈值取固定基数 1000 与 `2*hcount()` 的较大者，另有时间触发检查。达到条件时尝试把回收交给 executor，否则当前线程执行 do_reclamation。阈值是尝试收集的策略，不是内存积压硬上限。[分片、阈值与调度](https://github.com/facebook/folly/blob/0660ead5b610047a279cc291129e98e3f2a1fc37/folly/synchronization/HazptrDomain.h#L329)

`do_reclamation` 先 `extract_retired_objects` 取得这批候选，再 heavy fence，然后 `load_hazptr_vals` 收集保护地址。未标记对象通过 match_reclaim_untagged 分成受保护与不受保护两组；不受保护组调用 reclaim 回调，受保护组重新留在退休列表。回调还可产生 children，代码将这些后续对象纳入处理，而不是在持有旧遍历指针时递归修改同一链。[扫描与回收出口](https://github.com/facebook/folly/blob/0660ead5b610047a279cc291129e98e3f2a1fc37/folly/synchronization/HazptrDomain.h#L425)

这条顺序不能倒过来：如果先收集 hazard 快照，再把之后刚退休的节点也纳入候选，快照可能不适用于那些节点。可扩展扫描仍须遵守“先取得本次回收候选的责任，再验证保护”的协议。

nonintrusive `domain.retire(T*, deleter)` 还会分配一个 retire node，不能把 intrusive 路径的成本与异常条件套给它。删除器、存储分配以及 noexcept 边界需要分别检查；本篇不许诺任意回调抛异常后都可恢复，也不把回调的锁或阻塞从完整 retire/cleanup 成本中扣掉。

## 5. cohort 的清场与普通 cleanup 不同

tagged 列表需要额外锁定，以免某个 cohort/tag 的清场遗漏正在转移的对象。常规匹配发现 tagged 对象不再受保护时，不一定当场执行删除器，而可以进入该 cohort 的 safe list。cohort 的 shutdown_and_reclaim 再处理 tagged 清理、安全列表和本地剩余退休列表。[cohort 生命周期](https://github.com/facebook/folly/blob/0660ead5b610047a279cc291129e98e3f2a1fc37/folly/synchronization/HazptrObj.h#L280)

这是为“容器销毁前必须收束属于它的回调”增加的归属机制。它不是 GC 自动判断外部引用失效：同步 cohort 清场仍要求应用已禁止新的访问、结束原有合法访问，不能一边继续读，一边借它的无条件清理路径强制释放节点。

普通 `domain.cleanup()` 调 do_reclamation 并等待并发 bulk reclaim 收束，却不主动让读者撤销保护。如果仍有合法保护，受保护退休对象应继续保留。最终排空需要停止使用者、join、释放保护，再做相应清理；当前仍受保护对象不能因为一次 cleanup 返回就被视为已删除。[cleanup](https://github.com/facebook/folly/blob/0660ead5b610047a279cc291129e98e3f2a1fc37/folly/synchronization/HazptrDomain.h#L182)

归纳这三个可扩展机制的目标：TLS cache 减少保护记录申请交接；退休分片减少所有退休者挤在一个入口；阈值/executor/cohort 改变扫描与回调的批量、执行者和最终归属。它们没有消除长读者、回调阻塞、内存分配或最终清场的前提，也没有在本篇得到“任意线程数下都更快”的测量结论。

## 6. Folly RCU：从旧称谓找到当前提交的真实入口

旧 18 写了 `rcu_reader`、`synchronize_rcu()`。在本篇提交中，实际阅读入口是 `rcu_domain::lock/unlock`，可由 `std::scoped_lock<folly::rcu_domain>` 管理；自由函数叫 `rcu_synchronize`，写侧还有 `rcu_retire` 与 domain.call/retire。搜索时区分注释里的旧名字和真正的函数定义，不能因文件里搜到了字符串就认定存在同名 API。[实际公开接口](https://github.com/facebook/folly/blob/0660ead5b610047a279cc291129e98e3f2a1fc37/folly/synchronization/Rcu.h#L318)

读者 lock 读取全局 version_，交给 `ThreadCachedReaders::increment`；unlock 调 decrement。每线程记录组合 epoch 和嵌套读者数，检查侧用 epochIsClear/waitForZero 验证指定奇偶 epoch 是否还存在读者。第一次建立线程本地记录可能涉及分配，平台 fence 也不同，所以旧文“读侧近零成本”只能作为待测假设，不能变成所有调用的规范保证。[读者记录](https://github.com/facebook/folly/blob/0660ead5b610047a279cc291129e98e3f2a1fc37/folly/synchronization/detail/ThreadCachedReaders.h#L68)

从 `rcu_retire(p, d)` 到 domain.call：建立携带删除器的回调节点，retire 将它放入 q_。`half_sync` 将新工作收集进 queues_[0]，检查一个 epoch 的读者退出后，把经过足够阶段的 queues_[1] 转入 finished，再把 queues_[0] 推到下一阶段，更新 version_ 并通知同步等待者。显式 synchronize 的目标是当前版本之后两个 epoch；非阻塞 half_sync 若读者仍存在就返回，保留回调等待后续推进。[宽限期与双队列](https://github.com/facebook/folly/blob/0660ead5b610047a279cc291129e98e3f2a1fc37/folly/synchronization/Rcu.h#L395)

为什么不只等一次版本变化？进入读侧的线程可能延迟观察 version，退休队列也可能并发加入新工作。源码用两阶段及收集顺序处理这个窗口，不是“版本加一就让全部旧指针过期”。长读者使相应 epoch 无法清空，进而延迟那批回调；RCU 不以每个裸指针逐项登记换取 HP 式精确保护。

## 7. 宽限期结束与删除器完成：这个版本还有一个明确边界

finished 的回调在 syncMutex_ 外交给 executor_->add。默认 executor 与用户提供的异步 executor 可能有不同的实际执行时间；“已允许调用删除器”“已把回调交给 executor”“删除器已经返回”必须分开。

该固定提交的 `rcu_barrier` 仅调用 domain.synchronize，紧邻注释明确承认当前实现有问题。用一个将 add 收到的回调暂存起来的异步 executor 就能在纸上构造反例：宽限期已结束，synchronize/barrier 返回，但 executor 还没执行删除器。因此不能把这个版本的 barrier 名字当成“所有在途删除器都已经结束”的可靠证据。[barrier 与源码注释](https://github.com/facebook/folly/blob/0660ead5b610047a279cc291129e98e3f2a1fc37/folly/synchronization/Rcu.h#L499)

这不是让读者修补第三方库的作业，而是练习识别文档意图与实际路径的差距。课程 RCU 的相应[回调完成协议](../reclamation/05-rcu.md)单独讨论 barrier 责任；换生产库或版本时必须重新核验。使用本篇旧版本也不应假定未标出的其他边界都已证明正确。

此外，持读侧区域调用等待该读者退出的 synchronize，会形成自等待；持有删除器需要的锁跨越同步或回调执行，也可能死锁。裸 executor 指针、domain、回调捕获的资源必须活到相应使用结束。call 可能分配，retire/同步中还有 noexcept 边界，不能把“回调失败了”当作必然回传调用者的 future 异常。

## 8. 与教学实现的契约对照

| 领域 | 教学版 | 本次 Folly 路径 |
|---|---|---|
| 保护发布 | 明确全 SC 与复核 | light/heavy 平台协议加复核，不能只搬一半 fence |
| 记录申请 | 固定数量槽 | 默认域 TLS 记录缓存与 domain 记录池 |
| 退休候选 | 教学 pending 与串行收集 | 分片、阈值、executor、cohort 区分归属和清场 |
| 地址匹配 | 教学完整对象地址约定 | holder/obj 的基类地址规范化，接口义务不同 |
| RCU | 读者、批次与回调完成分开验证 | epoch 读者记录、双阶段队列、executor；该旧版 barrier 有已标注缺口 |
| 进展与容量 | 有限实验与明确停止前提 | 分配、重试、读者停顿、回调和平台系统路径都需计入 |

## 9. 可复验任务与完整答案

先核对 commit，再在已有源码或固定网页上追下面符号：

```powershell
git -C $src rev-parse HEAD
rg -n 'make_hazard_pointer|try_protect|reset_protection|try_put' "$src/folly/synchronization/HazptrHolder.h"
rg -n 'kCapacity|try_get|evict' "$src/folly/synchronization/HazptrThrLocal.h"
rg -n 'push_list|threshold\(|extract_retired_objects|load_hazptr_vals|do_reclamation|cleanup' "$src/folly/synchronization/HazptrDomain.h"
rg -n 'push_obj|shutdown_and_reclaim|set_reclaim' "$src/folly/synchronization/HazptrObj.h"
rg -n 'void lock|void unlock|half_sync|synchronize\(|executor_->add|rcu_barrier' "$src/folly/synchronization/Rcu.h"
```

**任务 A：线程退出时，TLS cache 究竟归还了什么？** 答案：归还的是保护记录，不是直接调用所有退休节点的删除器。holder 先清空保护，cache 析构 evict 记录到 domain；退休对象另按 domain/cohort 的路径处理。画图时把 record 和 object 画成两个方框。

**任务 B：删掉 try_protect 的重读，为什么缓存和分片都救不了？** 答案：读者可能先读旧地址后暂停，写者在公布保护之前完成摘除与扫描；之后登记只是一个过期地址。缓存、分片改变争用位置，不弥补没有成功复核的访问资格。

**任务 C：找到这份提交真正的“可扩展”字段并说出代价。** 答案：TLS cache 的 9 项记录、domain 的 8 份退休列表、随 hcount 调整的阈值、executor 与 cohort 均有真实字段/函数。它们换取本地复用或批量交接，也增加缓存保留、触发延迟、执行资源依赖与清场责任；常数是该提交实现，不是标准要求。

**任务 D：cleanup 返回后为什么可能仍有退休节点？** 答案：保护尚未撤销的候选应被保留；cleanup 等回收工作收束不等于取消所有读者。最终排空先停止/收束使用者并清除保护，再按相应 domain/cohort 协议清理。

**任务 E：画 Folly RCU 三条时线：读者、epoch 推进、executor。barrier 返回在哪条线上？** 答案：读者退出使 epoch 可推进；经过两阶段的回调进入 finished，再提交 executor；此提交的 rcu_barrier 只走 synchronize，未必等异步 executor 完成回调。用源码的 add 与 return 顺序复验，不声称本篇已经运行了反例。

**任务 F：与标准 HP/RCU 及课程接口能否逐名替换？** 答案：不能。这里是固定版本 Folly 扩展，函数名字、记录地址、域/执行器寿命及回调完成保证都要分别对照。标准基准仍为 N5050，滚动库源码既不是规范，也不是课程测试通过的替代证据。
