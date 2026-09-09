# 共享回收专题：从借用证据到回调完成

本专题是第 10 章“发布与生命周期”的深入正文。读者应已理解对象生命期、原子操作、happens-before 和基本 CAS；不要求已经实现过回收器。默认 C++23，cs 命名空间中的实现均为教学协议。

建议按以下问题顺序阅读；这是不同契约的分支，没有预设性能优劣链：

| 问题 | 正文 | 可运行 reference |
|---|---|---|
| 发布后为何还会访问死亡对象？引用计数和标签分别解决什么？ | [生命期与所有权](01-lifetime-and-ownership.md) | [延迟/ABA 模型、atomic shared_ptr](lifetime_reference.cpp) |
| 如何精确保护一个节点，且不漏掉登记窗口？ | [HP](02-hazard-pointers.md) | [I2](../../exercises/I2_hazard_pointer/solution.cpp) |
| 如何用一个读区保护多个借用？ | [EBR](03-epoch-reclamation.md) | [R1](../../exercises/R1_epoch_reclamation/solution.cpp) |
| 应用什么时候可以主动报告静默？ | [QSBR](04-qsbr.md) | [R2](../../exercises/R2_qsbr/solution.cpp) |
| 版本替换、宽限期和回调结束如何区分？ | [RCU](05-rcu.md) | [I3](../../exercises/I3_rcu/solution.cpp) |
| 如何复现、检查哪些性质、怎样集成？ | [验证说明](06-validation.md) | [runtime checks](../../exercises/runtime_tests/reclamation_test.cpp) |

这里有三份实际头文件：[hazard_pointer.hpp](../../exercises/include/concurrency_study/hazard_pointer.hpp)、[epoch.hpp](../../exercises/include/concurrency_study/epoch.hpp)、[rcu.hpp](../../exercises/include/concurrency_study/rcu.hpp)。EBR/QSBR/RCU 共用分代核心；区别在应用何时登记、何时承诺结束旧借用，以及何时要求回调结束。

所有普通实验安全且有限，不执行“先 delete 再解引用”的 UB。模型反例给出可达错误状态后停止，正确算法的测试使用真实线程、实际删除器、逐项检查和有上限的退休批次。正文区分协议推导、本机观测及尚未验证的条件。

## 共同客户端约束

源指针所有原子访问使用 SC；构造完成后才发布，发布后载荷字段不可变；摘除之后只退休一次，不重新发布。结构链接若仍会变化，必须使用结构自身已证明正确的原子协议；本题 HP 栈的 next 则发布后不变。每个对象的读取与退休遵循同一个协议/域。读者不得将裸借用带出保护边界。数据结构仍要自己证明 CAS、元素访问及线性化行为；生命期协议不会修正逻辑 ABA 或字段数据竞争。

HP 句柄可通过同步后的移动转交，但同一时刻只有一个使用者。epoch_guard、rcu_reader、qsbr_participant 绑定创建线程，不可移动。域须长于所有参与者、回收调用和回调；局部域是相互独立的，默认域有静态生命期但不支持静态销毁后的迟到调用。

所有回收实现包含短锁、分配和用户回调，完整操作不承诺 lock-free。退休队列无硬容量限制，具体实验主动限制对象数量/载荷字节并实施背压。长读区/漏报静默可能无限拖延回收，达到内存预算不能成为提前 delete 的理由。

## 正常清场

HP：停止生产 → join 使用者 → 撤销全部保护 → `cs::hazard_pointer_cleanup()`。返回 size_t，含义为本次完成的回调数；它不等待读者解除保护，同线程回调重入返回 0，外层会处理有限的派生退休。

EBR/QSBR/RCU：停止生产 → 全部读区结束、QSBR 下线 → join → barrier。若回调本身会退休更多对象，外部活动已停止后使用 `while (domain.pending() != 0) domain.barrier();`，并要求派生链有限。普通 I3/R1/R2 没有派生退休。synchronize 只等老读区，不执行回调；join 只等线程，不排空退休队列。

retire 函数体内部的分配/删除器移动失败及删除回调抛异常会终止程序；实参进入函数前的构造异常另论。guard 登记可抛异常；阻塞等待检测到同域读区自等待，或在回收回调内等待任何 epoch/RCU 域，会抛 logic_error。析构域时仍有参与者/回调属于契约违反并 terminate。回调不得形成跨线程锁/等待环。

## 规范与一手来源

固定规范是 [WG21 N5050](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf)，阅读 `[util.smartptr.atomic.shared]`、`[saferecl.hp]`、`[saferecl.rcu]`。cs API 的命名对应概念，不代表实现符合全部标准语义；尤其 SC 限制、异常、域构造和扩展方法需要按正文重新核对。

工业实现阅读入口为 [CK epoch 头](https://github.com/concurrencykit/ck/blob/master/include/ck_epoch.h)、[CK epoch 源码](https://github.com/concurrencykit/ck/blob/master/src/ck_epoch.c)、[liburcu API](https://github.com/urcu/userspace-rcu/blob/master/doc/rcu-api.md)、[liburcu QSBR 源码](https://github.com/urcu/userspace-rcu/blob/master/include/urcu/static/urcu-qsbr.h)。查阅日期为 2026-09-08，链接指向上游分支，可能继续变化；本文没有复制其平台算法。外部经验文章、一次压测和“没有崩溃”都不能替代协议保证。

四题的学生路径是 main.cpp → checks.hpp → student.hpp；只编辑 student.hpp 的 TODO。只读答案仍为 solution.cpp → reference.hpp。主入口先实际预检全部学生步骤，有未完成项在线程启动前返回 1，完成后才运行并发检查。题型和范围详见各题 README，不通过改写共享 reference 完成作业。

协议实现及此前的异常路径修复已获独立 review APPROVED；本次学生路径隔离仍待原 reviewer 复验。空 starter 与 Reference 的预期退出码不同，独立编译命令及本次证据见 [验证说明](06-validation.md)。
