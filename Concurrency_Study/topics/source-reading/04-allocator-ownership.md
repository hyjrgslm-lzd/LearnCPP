# 源码导读 04：mimalloc 的本地分配、远端释放和页面归属

动态队列的一次 push 可以只有一个 CAS，却仍要先分配节点；一次 pop 摘除节点以后，回收器最终调用的 delete 又会进入分配器。若忽略这两段，就无法解释完整操作的锁、内存占用和延迟。本篇读取 mimalloc，重点看“哪个线程可以修改哪条空闲链”，不把它当作应用对象生命周期的替代品。

固定版本为 mimalloc `v2.1.7`，commit [`8c532c32c3c96e5ba1f2283e032f69ead8add00f`](https://github.com/microsoft/mimalloc/tree/8c532c32c3c96e5ba1f2283e032f69ead8add00f)，2026-09-08 只读核查，未构建、运行、替换课程分配器。以下主路径限定普通 small block；超大对象、特殊对齐、安全/调试配置另有分支，不把 v2 的布局套到 v3。

真实入口为 [src/alloc.c](https://github.com/microsoft/mimalloc/blob/8c532c32c3c96e5ba1f2283e032f69ead8add00f/src/alloc.c)、[src/free.c](https://github.com/microsoft/mimalloc/blob/8c532c32c3c96e5ba1f2283e032f69ead8add00f/src/free.c)、[src/page.c](https://github.com/microsoft/mimalloc/blob/8c532c32c3c96e5ba1f2283e032f69ead8add00f/src/page.c)，布局在 [include/mimalloc/types.h](https://github.com/microsoft/mimalloc/blob/8c532c32c3c96e5ba1f2283e032f69ead8add00f/include/mimalloc/types.h)。与课程 [Michael–Scott 队列](../queues/06-michael-scott.md)、[HP](../reclamation/02-hazard-pointers.md)及 [缓存布局](../../chapters/13-cache-and-layout.md)对照即可，不新增分配器练习代码。

## 1. heap、page、block 分别管理什么

这里的 page 是分配器组织同类大小块的内部页，不应直接等同于一个 OS 页面。一个 page 内有多个 block；heap 维护按大小分类的页面队列，以及可快速索引的小对象页面；更大的 segment 管理底层存储与线程归属。

先在 `mi_page_t` 找三条空闲路径：`free` 供本地分配立即取块，`local_free` 接收本线程归还，`xthread_free` 接收其他线程的归还并编码 delayed-free 状态。`used` 记录尚未计入可复用的占用；`xheap` 标识所属 heap。heap 另有原子的 `thread_delayed_free` 链，负责把某些远端归还提交给拥有者处理。

这不是三份同义 free list。让远端线程直接修改本地 `free` 或 `used`，就会破坏分配快路径依赖的线程归属；让所有线程每次都竞争同一条全局 free list，又失去了本地分片的目的。源码通过不同归还通道把常见本地操作和跨线程协调分开。[布局与 delayed 标志](https://github.com/microsoft/mimalloc/blob/8c532c32c3c96e5ba1f2283e032f69ead8add00f/include/mimalloc/types.h#L240)

## 2. 从 mi_malloc 到一个交给调用者的块

沿 `mi_malloc`、`mi_heap_malloc`、`_mi_heap_malloc_zero_ex` 查找小对象路径，会进入 `mi_heap_malloc_small_zero`，选定大小类别对应 page，再调用 `_mi_page_malloc_zero`。它取 `page->free` 的头块，将 free 前移，增加 used，然后处理按配置启用的清零、统计和填充检查，最后返回 block 地址。[本地快路径](https://github.com/microsoft/mimalloc/blob/8c532c32c3c96e5ba1f2283e032f69ead8add00f/src/alloc.c#L31)

此时发生的是原始存储使用权交给调用者。若调用者随后用 placement new 构造 C++ 节点，那个对象的构造、发布和异常清理由调用者或其 RAII 包装负责；mi_malloc 本身不调用节点构造函数。若该块仍有别的线程通过过期指针访问，分配器也不能使这种访问重新合法。

当 free 为空，进入 `_mi_malloc_generic`。它可能初始化 heap，运行 deferred-free hook，处理部分其他线程的 delayed frees，然后寻找或分配合适 page；第一次找不到时强制 collect 并再试一次，仍失败则记录 ENOMEM 并返回 NULL。这是 C 分配接口的失败出口，不要写成“总会抛 bad_alloc”；C++ new 包装另有语义。[慢路径与失败](https://github.com/microsoft/mimalloc/blob/8c532c32c3c96e5ba1f2283e032f69ead8add00f/src/page.c#L895)

由这条路径能推出：一个看起来短小的分配调用可能顺带承担回收、页面管理和系统存储获取。是否走快路径取决于当前状态，不取决于调用者把函数命名为 try_push。没有本机运行数据，不给出“几条指令”“固定纳秒数”或 lock-free 的完整操作保证。

## 3. 本地释放先进入 local_free

`mi_free` 先检查指针并找到 segment/page，再比较当前线程 ID 与 segment 的归属。普通本地块走 `mi_free_block_local`：把块放到 local_free，递减 used；若 used 变为零，进入 `_mi_page_retire`，否则必要时将原来 full 的页移回可用队列。[释放入口](https://github.com/microsoft/mimalloc/blob/8c532c32c3c96e5ba1f2283e032f69ead8add00f/src/free.c#L138)

这里还没有要求将每个刚释放的块立刻插回分配用 free 链。`_mi_page_free_collect` 稍后把 local_free 合并或交给 free；当 free 已空时，可以直接接管 local_free 的整条链。批量切换减少了每次分配路径要处理的情况，但也意味着“应用已经 free”和“这块已经回到某条立即分配链”不是一个时刻。

调试路径中的 double-free 或 padding 检查是诊断措施，不改变调用契约：对已释放指针再次 free、对不属于该分配器的地址 free 都不是本篇允许的输入。关闭这些检查也不会使非法输入变安全。

## 4. 远端线程为什么不能直接递减 used

假设 A 分配一个节点，节点经安全的队列和回收协议交给 B 删除。B 的 mi_free 发现它不是所属线程，进入 `mi_free_generic_mt`、`mi_free_block_mt`，普通情况最终到 `mi_free_block_delayed_mt`。这条路径通过 `page->xthread_free` 的原子 CAS 发布归还，不直接修改所属线程使用的 local_free 与 used。[远端提交](https://github.com/microsoft/mimalloc/blob/8c532c32c3c96e5ba1f2283e032f69ead8add00f/src/free.c#L193)

多数情况下，block 的 next 指向旧的远端空闲链头，release CAS 把它放进 xthread_free。CAS 失败会用更新的观察重试，尚未成功链接的 block 仍由这个释放操作负责，不能提前结束后让别人继续使用它。

拥有者执行 `_mi_page_thread_free_collect` 时，用 acq_rel CAS 摘走当前远端链，数出归还块数，把链接入 local_free，再减少 used。之后 `_mi_page_free_collect` 才可能将这些块变成分配快路径的 free。远端发布与所属线程接管分开，使普通字段的修改保持在相应归属协议内。[收集与计数](https://github.com/microsoft/mimalloc/blob/8c532c32c3c96e5ba1f2283e032f69ead8add00f/src/page.c#L181)

从应用视角，B 已经 free 的块不再属于应用；从分配器视角，所属页的 used 可能要等拥有者收集远端链才体现这次释放。这解释了为什么不能用一次远端 free 返回，直接推断该页已经可释放到 OS。

## 5. delayed-free 状态是在保护哪次交接

如果 xthread_free 的标志是 `MI_USE_DELAYED_FREE`，该释放者先 CAS 成 `MI_DELAYED_FREEING`，然后取得 `page->xheap`，把 block 放入 heap 的 `thread_delayed_free` 链，最后将状态改为 `MI_NO_DELAYED_FREE`。这个中间状态保护“正在向某个 heap 交接归还”的窗口，涉及 heap 删除或迁移时的生命期；不是一个只为优化统计而存在的 bool。

所属线程从 heap delayed 链取到 block 后，调用 `_mi_free_delayed_block`。它先调用 `_mi_page_try_use_delayed_free` 尝试让 page 重新使用 delayed-free 通知；这次尝试成功，才收集该 page 的其他远端释放，最后用本地释放流程处理手上这个 block。源码特别说明成功路径的顺序：若先收集、后恢复通知，新远端释放可能只落入 page 链，却没有留下促使拥有者再处理该页的 heap 通知。[delayed block 接收](https://github.com/microsoft/mimalloc/blob/8c532c32c3c96e5ba1f2283e032f69ead8add00f/src/free.c#L163)

尝试也有正常失败出口：若 `_mi_page_try_use_delayed_free` 仍观察到另一个释放者的 `MI_DELAYED_FREEING`，会有限次 yield；该提交在四次让出后仍遇到此状态便返回 false，`_mi_free_delayed_block` 随即返回 false，不进入后续 collect 和本地释放。这里只是对等待该中间状态的尝试作限制，不能推出整个 CAS 重试过程有固定总步数。[有限让出与失败](https://github.com/microsoft/mimalloc/blob/8c532c32c3c96e5ba1f2283e032f69ead8add00f/src/page.c#L146)

这时 block 不会丢失，也不退回应用所有。调用者 `_mi_heap_delayed_free_partial` 已从 heap delayed 链接管一批块，先保存当前块的 next；遇到 false 就将 all_freed 设为 false，用 release CAS 把该 block 重新挂回 `heap->thread_delayed_free`，再继续处理原批次的其他块。后续 partial 调用会重新尝试；`_mi_heap_delayed_free_all` 则在 partial 返回 false 时 yield 并继续调用，直到该循环的收集条件满足。责任始终在分配器的本次批次或 heap delayed 链之间交接，不能把函数返回 false 解释为该块已经完成本地 free。[重新入链与再次收集](https://github.com/microsoft/mimalloc/blob/8c532c32c3c96e5ba1f2283e032f69ead8add00f/src/page.c#L313)

这是值得练习的源码阅读方式：不要只抄四个枚举名，而要追“谁把哪个指针放进哪条链”“中间状态何时结束”“改变这两步顺序会错过什么工作”。`MI_NEVER_DELAYED_FREE` 又属于无所属 heap 的 abandoned 页面等路径，与暂时的 NO_DELAYED 状态不同，不能合并为同一“关闭”。

## 6. 块释放、页退休、segment 归还各有出口

`_mi_page_retire` 的前提是该页所有块都已可释放，但它仍可能保留当前大小类别的唯一页面，设置 retire_expire，等待后续 collect 决定是否真正移除。这里的 retire 是分配器缓存策略，不是“应用节点仍有 HP 读者所以不得析构”的 retire。二者名字相同，前提相反：分配器此时处理的应用块已经归还。[页退休](https://github.com/microsoft/mimalloc/blob/8c532c32c3c96e5ba1f2283e032f69ead8add00f/src/page.c#L409)

真正 `_mi_page_free` 会把 page 从 heap 队列移除、清除 heap 归属，进入 `_mi_segment_page_free`。继续到 [segment.c](https://github.com/microsoft/mimalloc/blob/8c532c32c3c96e5ba1f2283e032f69ead8add00f/src/segment.c#L1025)，会看到页面清理、segment 的 used/abandoned 条件，以及 segment 最终归还底层存储的路径。因 arena、缓存、purge/decommit 等条件不同，不能把这些函数每次返回都解释成一次 OS unmap。

线程或独立 heap 结束也不应丢失尚未由应用释放的块。`mi_heap_delete` 可以把页面吸收到 backing heap；backing heap 退出则走 `_mi_heap_collect_abandon`：先将相应页面设为不再向旧 heap 投 delayed 通知，处理已有通知，再将仍使用的页面/segment 标为 abandoned。其他有存储需求的线程可通过 `mi_segment_try_reclaim`/`mi_segment_reclaim` 接管它们，更新线程归属并继续收集归还。[heap 迁移与删除](https://github.com/microsoft/mimalloc/blob/8c532c32c3c96e5ba1f2283e032f69ead8add00f/src/heap.c#L409)

不要把 mi_heap_delete 与 mi_heap_destroy 混同：后者有批量销毁 heap 存储的语义及前提。更不能以为“分配线程退出了”就授权应用继续访问已经释放的节点。分配器解决存储最终由谁管理；HP/RCU/锁解决应用何时允许结束对象生命期，先后责任不能颠倒。

## 7. 与教学实现的契约对照

| 观察问题 | 教学中的相应边界 | mimalloc 源码增加的责任 |
|---|---|---|
| push 看起来只有 CAS | 完整 push 还包括 new | 小块快/慢路径、页补给、OOM 与 deferred free |
| retire 后何时可 delete | HP/RCU 证明没有合法读者 | delete 之后进入本地或远端归还通道，二者不要混成一次回收 |
| 一次 free 是否立刻减内存 | 应用不再拥有对象 | remote collect、页缓存、segment/arena 处理可能继续持有存储 |
| 线程结束后剩余记录 | 教学回收保留退休记录归属 | heap 吸收、页面 abandoned 与另一线程 reclaim |
| 性能成本 | 计时边界包含分配/回收 | 必须区分快路径、批量收集与系统路径，不能用单次观察概括 |

本篇没有把 mimalloc 接入课程，不比较它与本机默认 allocator 的速度。若将来做那项实验，先保持队列、输入、分配/释放线程角色和计时边界一致，再声明是更换 allocator 的同契约实验。

## 8. 阅读任务与完整答案

在已有只读检出中核对 commit，使用以下搜索追同一个 block；没有检出时用固定网页定位相同函数即可：

```powershell
git -C $src rev-parse HEAD
rg -n 'mi_malloc\(|_mi_heap_malloc_zero_ex|mi_heap_malloc_small_zero|_mi_page_malloc_zero' "$src/src/alloc.c"
rg -n 'mi_free\(|mi_free_block_local|mi_free_block_delayed_mt|_mi_free_delayed_block' "$src/src/free.c"
rg -n '_mi_malloc_generic|_mi_page_thread_free_collect|_mi_page_free_collect|_mi_page_retire' "$src/src/page.c"
rg -n '_mi_page_try_use_delayed_free|yield_count|_mi_heap_delayed_free_partial|_mi_heap_delayed_free_all|all_freed' "$src/src/page.c"
rg -n 'mi_heap_delete|mi_heap_absorb|MI_NEVER_DELAYED_FREE' "$src/src/heap.c"
rg -n '_mi_segment_page_free|mi_segment_abandon|mi_segment_reclaim' "$src/src/segment.c"
```

**任务 A：从一次普通小块分配列出两个出口。** 答案：free 非空则摘首块、used++ 并返回；free 为空则进入 generic，可能收集/找页/分配，重试仍无页时返回 NULL。前者的短路径不能替整个 mi_malloc 作进展或延迟承诺。

**任务 B：A 分配、B 释放，哪一次修改 used？** 答案：B 普通远端释放先发布到远端链；所属线程收集这条链后按块数递减 used。若走 heap delayed 分支，所属线程还会用本地释放处理那一个通知块。按这两条路径分别画，避免漏算或重复计数。

**任务 C：为什么恢复 delayed-free 标志要先于收集 page 远端链？如果恢复尝试失败，由谁接管？** 答案：成功恢复使后来的远端归还能产生必要的 heap 级通知；顺序反过来可能只剩 page 链中的新块，拥有者没有相应 delayed 工作可见。若遇 MI_DELAYED_FREEING 的有限让出尝试耗尽，helper 和 _mi_free_delayed_block 返回 false，当前块没有进入本地释放；partial 调用者保存原 next、将块 release CAS 回 heap delayed 链，标记尚未全部处理，再继续其他块。下一次 partial 或 all 的循环重试负责接续这项工作。复验需顺着 free.c 的 false 返回，跳到 page.c L313 起找重新入链和循环，不能在返回 false 处截断所有权图。

**任务 D：为何页已经没有应用使用块，仍不马上归还系统？** 答案：_mi_page_retire 可以保留大小类别的唯一页，后续 collect 才移除；移除页后还要看 segment 和底层 arena。把 API free、页从队列删除、OS 存储归还标成三段事件。

**任务 E：换成 mimalloc 能否消除 Treiber 栈的 ABA/UAF？** 答案：不能。地址仍可能复用，分配器也不知道哪个裸指针代表合法读者。必须先由结构算法和回收协议确认可以释放，再调用分配器；更快地复用地址甚至可能改变错误复现的概率，而不是修复错误。

本篇的完成证据是固定源码中的分支与所有权链。无生产库运行、分配失败注入或内存驻留实测，相关结论一律不冒充实验结果。
