# 队列旁支 07：Treiber 栈、ABA 与回收的不同职责

栈和队列解决不同的顺序需求。队列是 FIFO，栈是 LIFO；把队列改成栈并不是保持相同契约的性能优化。本篇独立研究 Treiber 栈，因为它用一个根指针就能展示 CAS 重试、节点发布、过期快照和回收的关系，再将这些认识带回链式队列。

实际栈在 [`queue_linked.hpp`](../../exercises/include/concurrency_study/queue_linked.hpp) 的 `treiber_stack<T>`，完整检查见 [G1](../../exercises/G1_treiber_stack/solution.cpp)。ABA 的安全逻辑模型在 [G2/reference.hpp](../../exercises/G2_aba_problem/reference.hpp)，由其 solution.cpp 运行。默认代码不包含 use-after-free，也不需要故意泄漏节点。

## 1. 一个根指针上的 LIFO

push 先构造新节点 N，把 N->next 设成观察到的 head，然后 CAS head 到 N。成功后 N 成为新栈顶；失败时 compare_exchange 会把观察到的值写回 expected，也就是这里的 N->next，再重试。N 尚未发布时只有当前线程能写它，发布成功后 next 不再改变。

```text
原结构: head -> A -> B
候选:   N.next = A
成功:   head -> N -> A -> B
```

pop 的候选是 old=head，要提交的是 old->next。但读取 old->next 之前必须保护 old。本实现每次循环先用 HP protect(head_)，空指针直接返回 false；读 next 后 CAS head 到 next，成功时复制 old->value 到输出并 retire old。CAS 失败写回的 old 不自动获得保护，所以必须回到 protect 重新取得，不能接着解引用。

成功后的复制显式使用 `std::as_const(old->value)`。`is_nothrow_copy_assignable_v<T>` 检查的是 const 源赋值；直接从可变 old->value 赋值可能选到另一个会抛的重载，在根已移除之后跳过 retire。G1 的 `copy_overload_probe` 回归覆盖这种双重载类型，要求可变源重载调用次数为零，并确认活节点、退休节点在退出作用域后全部释放。更完整的表达式与 trait 对照见[预分配篇](02-bounded-and-batch.md)。

成功 push/pop 都在线性化到各自 head CAS 后改变逻辑栈；失败空 pop 在线性化到经过保护协议验证的空 head 观察。值复制赋值不抛异常、析构不抛异常，复制构造或 new 可以在发布前抛。只可移动 T 不在接口范围内。每个节点只能由队列实现创建并发布一次，不开放将已弹出节点直接重新插回的接口。

## 2. 为什么发布不等于寿命保证

Acquire/release 可以解释“观察到了节点，能看见其初始化内容”，却不能阻止另一个线程把节点从根上摘除后 delete。即使 reader 的 acquire 确实读到了 writer 的 release，它随后解引用的对象也可能已经结束生命周期。

G1 使用课程 HP 协议，head 的所有原子访问保持 SC。push 不解引用旧 head，只把它作为 CAS 对比和新链连接值；pop 解引用当前候选，必须先保护。成功 pop 只复制自己的节点值，其他失败消费者读取的是不可变 next；节点不复用到栈中，保护解除前不会由分配器回收复用。

当前全 SC 是回收契约所需，不能按旧版教程机械削弱成“push release、pop acquire 就够”。在另一个拥有完整弱内存 HP 证明的库中可以有别的实现，但不能只改这个文件里的两个内存序而把扫描协议留在原处。

节点摘除后 retire，达到域阈值时可以扫描释放；全部线程 join 后调用 cleanup，析构函数还删除未弹出的活链。G1 Reference 用 3 个生产者与 4 个消费者同时操作 12007 个 ID，逐项检查完整集合；再用带 live 计数的非平凡类型验证运行中清理以及非空栈析构后计数归零。旧版本“总数一致所以不丢不重”和“为教学必须泄漏”两项结论都已替换。

## 3. ABA 可以完全没有未定义行为

CAS 比较当前值与 expected 的值，不记录期间经历过哪些中间值。用固定数组的三个活节点下标表示 A=2、B=1、C=0，初始为 A->B->C。慢线程保存 expected=A、cached_next=B 后暂停。快线程依次 pop A、pop B、把 A 重新接到 C 前并 push A。根又等于 A，但 A->next 已改为 C。

慢线程随后 CAS(A,B) 会成功，把已经被弹出的 B 又接成根。这里 B 仍是数组中的活对象，因此不能把它描述成“已经释放的悬空节点”；错误是将不属于当前栈的旧节点重新接回。其后的 C 也不一定丢失，必须根据具体 next 链检查，而不是套用一句固定灾难描述。

G2 用两个 release/acquire 信号编排：快线程在慢线程完成读取后才改普通 next；慢线程在快线程结束后才执行 CAS。数组始终活着，不做 delete，且最终由作用域自动释放。因此这个反例可以安全地默认运行，它展示的是逻辑错误而不是依赖 UB 的偶然崩溃。若另外研究删节点后的悬空访问，必须单独隔离并启用诊断，不能作为正常 Reference。

## 4. 标签能发现这段历史，但有宽度前提

G2 的第二次运行将 head 打包成一个 `atomic<uint64_t>`：低 32 位是数组下标，高 32 位是版本。初始 (A,0)，实际执行 pop A 到 (B,1)、pop B 到 (C,2)、重新接 A 到 (A,3)。慢线程仍持有 (A,0)，所以它的 CAS 失败。Reference 确认返回值与最终根，不再用“只把版本加三次”冒充完整栈变化。

这不是通用 64 位地址加 32 位版本的压缩指针，而是固定数组的下标模型。不要将任意真实指针截成低 32 位。即使平台恰好支持原子 uint64_t，也必须实测 is_lock_free，不能把类型名当硬件承诺。

有限版本可以回绕。第三个检查把标签缩成两位，实际执行 A->B->A->B->A 四次变化，版本回到零，旧 CAS 再次成功。这说明“版本号根除 ABA”缺少前提。要使用有限标签，必须限制任一旧观察存活期间的更新次数，使版本不会绕回相同值，或使用另一套能防止危险复用的机制。更宽的版本增加余量，不提供对任意长暂停的数学保证。

## 5. HP 和标签各自解决什么

标签使比较的状态更丰富，帮助检测中途变化；它并不会延长节点生命。读者若在比较标签之前就解引用已经释放的节点，错误已经发生。HP 延迟释放和地址复用，保证受保护的对象仍可访问；它也不会阻止应用在对象仍存活时主动改 next 并重新插入同一个节点。

G2 反例中的“活着的 A 被重新接入”正属于后一种情形，所以不能声称“只要登记 hazard，所有 ABA 都消失”。G1 的正确性额外依赖节点 next 发布后不变、退休节点不直接重插、通过安全回收后才允许分配器复用。回收与容器操作规则组合起来，才排除危险的根地址 ABA。

在底层原子无锁与安全节点寿命的假设下，Treiber 的核心 CAS 算法是 lock-free，单线程可能持续重试而非 wait-free。weak 允许伪失败，strong 不允许这种伪失败，但两者都只比较值，换 strong 不会治好 ABA。也不能从 weak 的存在推出它必然比 strong 更快，性能取决于实现与负载。

完整 G1 操作还有 new、HP 槽、退休表 mutex 与扫描删除，不承诺 lock-free。一个 pop 使用一个 HP 槽，和 Q2 的每次出队两个槽共用固定域预算。Reference 输出的 pointer atomic 属性必须与这些完整成本一起解释。

## 6. 运行与自测答案

```powershell
# 在 C08_Concurrency/exercises 下
cmake -S G1_treiber_stack -B build/g1 -G "Visual Studio 18 2026" -A x64
cmake --build build/g1 --config Release
ctest --test-dir build/g1 -C Release --output-on-failure
cmake -S G2_aba_problem -B build/g2 -G "Visual Studio 18 2026" -A x64
cmake --build build/g2 --config Release
ctest --test-dir build/g2 -C Release --output-on-failure
```

**CAS 失败后 expected 是可以马上解引用的新节点吗？** 不是。它是这次比较观察到的值，必须重新通过保护协议取得解引用权，也不保证是墙钟意义的“最新值”。

**SC 能阻止 ABA 吗？** 不能。G2 的所有 head 原子操作本来就是 SC，合法全序仍然可以包含 A->B->A。

**标签失败是否足以说明旧 next 从未被并发修改？** 不足。标签比较只检查状态值；节点字段的并发访问仍必须独立避免数据竞争。G2 通过信号显式排序普通 next 访问。

**为什么 G1 检查集合而不要求消费者返回递减 ID？** 不同生产者并发 push 的顺序不是 ID 大小顺序，多消费者的返回顺序也不同于 CAS 顺序。顺序小测试检查 20 后 10；并发 LIFO 用独立 history test 搜索合法串行解释。

CAS 与对象生命周期规范入口见 [N5050](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf) `[atomics.types.operations]`、`[basic.life]`、`[saferecl.hp]`。上面的 ABA 数组模型和保护边界是本课程可运行推导；链式队列背景可对照 [Michael–Scott 作者说明](https://www.cs.rochester.edu/research/synchronization/pseudocode/queues.html)。
