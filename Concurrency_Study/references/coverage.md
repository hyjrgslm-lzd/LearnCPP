# 知识覆盖、练习与迁移

本表用于核对课程知识是否实际落到正文、程序及检查。每个练习链接进入其 Part、完整解析及复现命令；Reference 目标为该 ID 加 `_reference`。实际构建与独立审查状态统一见[质量报告](quality-report.md)，本表不把“存在文件”当作“已经验证”。

全局归属：本课主讲 C08，CPU 性能/SIMD/NUMA 作为 C13 的已有实验资产；通用编译、链接、ABI、CMake 和能力探测由 [C01](../../Engineering_Study/README.md)主讲。C++29 线程属性与 HP batches 的新增索引和未实施状态见[版本索引](standards-and-implementations.md#c29-增量的独立状态)，没有改变以下50题的完成含义。

## 1. 逐练习覆盖

| 练习 | 主要知识 | 正文入口 |
|---|---|---|
| [P1_object_lifetime](../exercises/P1_object_lifetime/README.md) | 对象、捕获、移动、tuple/tie/ref/apply | [00](../chapters/00-execution-and-objects.md) |
| [P2_result_states](../exercises/P2_result_states/README.md) | valid/ready、等待与消费、共享结果 | [02](../chapters/02-result-channels.md) |
| [A1_jthread_lifecycle](../exercises/A1_jthread_lifecycle/README.md) | 线程拥有者、join、参数与引用寿命 | [03](../chapters/03-threads-and-execution.md) |
| [A2_stop_token_cancellation](../exercises/A2_stop_token_cancellation/README.md) | 共享停止状态、回调、注销与收束 | [06](../chapters/06-cancellation-and-shutdown.md) |
| [A3_thread_exception](../exercises/A3_thread_exception/README.md) | 异常捕获、exception_ptr、结果通道 | [03](../chapters/03-threads-and-execution.md) |
| [B1_mutex_family](../exercises/B1_mutex_family/README.md) | 复合不变量、RAII锁、读写锁和边界 | [04](../chapters/04-shared-state-and-locks.md) |
| [B2_deadlock_scoped_lock](../exercises/B2_deadlock_scoped_lock/README.md) | 等待环、多锁协议、固定顺序 | [04](../chapters/04-shared-state-and-locks.md) |
| [B3_call_once](../exercises/B3_call_once/README.md) | 初始化、异常重试与发布 | [04](../chapters/04-shared-state-and-locks.md) |
| [C1_condvar_predicate](../exercises/C1_condvar_predicate/README.md) | 谓词、通知先后与重新检查 | [05](../chapters/05-waiting-and-channels.md) |
| [C2_bounded_queue_condvar](../exercises/C2_bounded_queue_condvar/README.md) | 容量、背压、关闭与排空 | [05](../chapters/05-waiting-and-channels.md) |
| [C3_interruptible_wait](../exercises/C3_interruptible_wait/README.md) | 可中断等待、期限与退出谓词 | [06](../chapters/06-cancellation-and-shutdown.md) |
| [D1_promise_future](../exercises/D1_promise_future/README.md) | 值、异常、broken_promise、共享消费 | [02](../chapters/02-result-channels.md) |
| [D2_async_policies](../exercises/D2_async_policies/README.md) | 执行策略、gate和最后引用 | [03](../chapters/03-threads-and-execution.md) |
| [D3_packaged_task](../exercises/D3_packaged_task/README.md) | 可调用任务、在线队列、关闭时积压 | [03](../chapters/03-threads-and-execution.md) |
| [Capstone1_thread_pool](../exercises/Capstone1_thread_pool/README.md) | 真实任务投递、move-only、背压、shutdown | [06](../chapters/06-cancellation-and-shutdown.md) |
| [E1_atomic_basics](../exercises/E1_atomic_basics/README.md) | 原子性、RMW、转换和无锁能力 | [08](../chapters/08-atomics.md) |
| [E2_atomic_ref](../exercises/E2_atomic_ref/README.md) | 既有对象、对齐、访问阶段、const目标 | [08](../chapters/08-atomics.md) |
| [E3_cas_loop](../exercises/E3_cas_loop/README.md) | expected、重算、溢出、min/max版本 | [08](../chapters/08-atomics.md) |
| [F1_release_acquire](../exercises/F1_release_acquire/README.md) | 发布读取与明确同步边 | [09](../chapters/09-memory-model.md) |
| [F2_relaxed_counter](../exercises/F2_relaxed_counter/README.md) | 单对象原子性与最终完成同步 | [09](../chapters/09-memory-model.md) |
| [F3_seqcst_fence](../exercises/F3_seqcst_fence/README.md) | SC约束、允许结果与fence | [09](../chapters/09-memory-model.md) |
| [F4_publish_pattern](../exercises/F4_publish_pattern/README.md) | 双向确认、槽位复用与后续写 | [10](../chapters/10-publication-and-lifetime.md) |
| [G1_treiber_stack](../exercises/G1_treiber_stack/README.md) | LIFO、CAS、节点保护与退休 | [11](../chapters/11-concurrent-structures.md) |
| [G2_aba_problem](../exercises/G2_aba_problem/README.md) | 逻辑ABA、地址复用、标签与寿命 | [11](../chapters/11-concurrent-structures.md) |
| [G3_spsc_ringbuffer](../exercises/G3_spsc_ringbuffer/README.md) | 独立槽位、双向交接、缓存位置 | [11](../chapters/11-concurrent-structures.md) |
| [H1_latch_barrier](../exercises/H1_latch_barrier/README.md) | 一次汇合、重复阶段、completion边界 | [07](../chapters/07-coordination.md) |
| [H2_semaphore](../exercises/H2_semaphore/README.md) | 资源上限、无所有权信号与许可 | [07](../chapters/07-coordination.md) |
| [H3_atomic_wait_notify](../exercises/H3_atomic_wait_notify/README.md) | 值比较、通知、ABA遗漏与代号 | [08](../chapters/08-atomics.md) |
| [I1_atomic_shared_ptr](../exercises/I1_atomic_shared_ptr/README.md) | 不可变快照、所有权、多写者重算 | [10](../chapters/10-publication-and-lifetime.md) |
| [I2_hazard_pointer](../exercises/I2_hazard_pointer/README.md) | 保护校验、退休、回收与退出 | [10](../chapters/10-publication-and-lifetime.md) |
| [I3_rcu](../exercises/I3_rcu/README.md) | 读复制发布、宽限期、回调完成 | [10](../chapters/10-publication-and-lifetime.md) |
| [Capstone2_lockfree_queue](../exercises/Capstone2_lockfree_queue/README.md) | 有界MPMC、失败契约、进展与回绕 | [11](../chapters/11-concurrent-structures.md) |
| [J1_false_sharing](../exercises/J1_false_sharing/README.md) | 共享、伪共享、分离与批量对照 | [13](../chapters/13-cache-and-layout.md) |
| [J2_interference_size](../exercises/J2_interference_size/README.md) | 尺寸常量、对齐与实际布局 | [13](../chapters/13-cache-and-layout.md) |
| [J3_numa_concept](../exercises/J3_numa_concept/README.md) | 真实拓扑、允许CPU与亲和验证 | [16](../chapters/16-scheduling.md) |
| [K1_simd_basics](../exercises/K1_simd_basics/README.md) | 标量与显式后端、lane和能力 | [15](../chapters/15-simd.md) |
| [K2_simd_dotproduct](../exercises/K2_simd_dotproduct/README.md) | 归约、精度预算、尾部与后端 | [15](../chapters/15-simd.md) |
| [K3_simd_where_select](../exercises/K3_simd_where_select/README.md) | 掩码、索引、gather/scatter边界 | [15](../chapters/15-simd.md) |
| [L1_par_algorithms](../exercises/L1_par_algorithms/README.md) | policy合同、异常与比较条件 | [14](../chapters/14-parallel-algorithms.md) |
| [L2_parallel_reduce_scan](../exercises/L2_parallel_reduce_scan/README.md) | 归约、扫描、结合顺序与误差 | [14](../chapters/14-parallel-algorithms.md) |
| [L3_par_vs_seq_bench](../exercises/L3_par_vs_seq_bench/README.md) | 成功量、计时边界、未测线程数 | [12](../chapters/12-measurement.md) |
| [M1_work_stealing_pool](../exercises/M1_work_stealing_pool/README.md) | 分块、动态调度、窃取与依赖 | [16](../chapters/16-scheduling.md) |
| [M2_execution_bridge](../exercises/M2_execution_bridge/README.md) | 固定stdexec、三通道与收束 | [16](../chapters/16-scheduling.md) |
| [Capstone3_parallel_compute](../exercises/Capstone3_parallel_compute/README.md) | GEMM、归约、排序逐版校验 | [14](../chapters/14-parallel-algorithms.md) |
| [Q0_queue_baseline](../exercises/Q0_queue_baseline/README.md) | 锁内检查修改、容量和真实FIFO | [11](../chapters/11-concurrent-structures.md) |
| [Q1_mpsc_queue](../exercises/Q1_mpsc_queue/README.md) | 单消费者分支、预订发布与停顿 | [11](../chapters/11-concurrent-structures.md) |
| [Q2_ms_queue](../exercises/Q2_ms_queue/README.md) | 链式MPMC、帮助、后继保护与回收 | [11](../chapters/11-concurrent-structures.md) |
| [R1_epoch_reclamation](../exercises/R1_epoch_reclamation/README.md) | 注册代号、长读者、积压与失败 | [10](../chapters/10-publication-and-lifetime.md) |
| [R2_qsbr](../exercises/R2_qsbr/README.md) | 静止点、上下线、调用者义务 | [10](../chapters/10-publication-and-lifetime.md) |
| [N1_numa_placement](../exercises/N1_numa_placement/README.md) | 页面位置、first-touch、局部/远端/分片 | [16](../chapters/16-scheduling.md) |

## 2. 复杂主题的实现与检查

| 主题 | 演进与分支 | 主要检查 |
|---|---|---|
| 并发队列 | mutex、预分配环、批量、SPSC及缓存、MPSC、有界MPMC、Michael–Scott | 容量、逐项ID、历史搜索、预订停顿、回绕、类型重载、bool独立存储 |
| 栈与ABA | Treiber、逻辑ABA、标签与生命周期分离 | LIFO、受控状态模型、保护窗口、实际析构计数 |
| 回收 | 引用计数、HP、EBR、QSBR、RCU | 保护/退休/允许释放、线程退出、嵌套域、回调重入与结束、启动/收尾异常 |
| 同步与关闭 | 复合操作、CV、停止回调、latch/barrier/semaphore、线程池 | 谓词、广播、限额、关闭竞态、排空、异常future、同池调用边界 |
| 内存模型 | 原子更新、发布、重复使用、SC、fence、atomic_ref | 状态与同步边推导、小模型、实际运行、操作和版本前提 |
| SIMD与数值 | 标量、布局、自动向量化、SSE2/xsimd/原生分支、掩码、归约 | 独立oracle、误差界、特殊值、对齐/尾部/索引、ISA选择 |
| NUMA与调度 | 拓扑、绑核、放置、初始化、分片、静态/动态/窃取、sender | 实际CPU/页面证据、SKIP条件、唯一任务ID、关闭、递归及三通道 |

运行时检查入口包括 [atomic_protocol_test](../exercises/runtime_tests/atomic_protocol_test.cpp)、[queue_history_test](../exercises/runtime_tests/queue_history_test.cpp)、[reclamation_test](../exercises/runtime_tests/reclamation_test.cpp)、[thread_pool_test](../exercises/runtime_tests/thread_pool_test.cpp)、[numeric_test](../exercises/runtime_tests/numeric_test.cpp)、[scheduling_test](../exercises/runtime_tests/scheduling_test.cpp)。它们与相应 Reference 共同覆盖具体程序，协议证明仍在正文。

### 按核心问题追到契约、代码和证据

下列路线补足表格中的先修与契约入口。正文是推导和答案，练习 README 逐 Part 定位 Reference；各专题验证记录是作者证据，独立批准及最终集成结果以[质量报告](quality-report.md)为准，不能将二者混为一谈。

- **如何把结果交给另一个执行者，同时保持对象存活？** 先完成 00 的对象、捕获和参数打包，再读 02 的有效/就绪/消费状态；03 对照直接调用、thread、async 和 packaged_task。D1–D3 将值、错误、共享状态和任务排空分开检查，P1/P2 提供最小前置实验。深度：自行扩展观察基线，完整参考实现及协议解析。
- **怎样等待、取消并可靠关闭一组工作？** 先修 03–05 的线程拥有者、锁与等待谓词，再沿 [有界线程池](../topics/synchronization/01-bounded-thread-pool.md)追踪接受、拒绝、排空和 join 契约。C2、A2/C3、H1/H2、Capstone1 的独立学生路径与 Reference 分别验证；[同步记录](../topics/synchronization/VALIDATION.md)说明受控顺序、故障边界和运行结果。深度：实现与协议试验，不声称通用结构化并发框架。
- **一次原子访问何时能发布其他数据？** 先修 04 的复合不变量和 08 的合法操作，再沿 [同步边](../topics/atomics/04-happens-before.md)、[fence](../topics/atomics/06-fences.md)、[重复发布](../topics/atomics/07-publication-and-lifetime.md)比较单次发布、确认复用和快照所有权。E/F/H3/I1 的 Reference 与 atomic_protocol_test 对应合法结果及受控反例；规范和机器观察分别记录。深度：完整语义推导与最小协议实验。
- **何时可以称入队或出队已经发生？** 先修 05 的锁内检查修改、09 的同步边和 10 的生命期，再从 [mutex 基线](../topics/queues/01-mutex-baseline.md)进入各线程拓扑分支；[MPMC](../topics/queues/05-vyukov-mpmc.md)与 [Michael–Scott](../topics/queues/06-michael-scott.md)分别声明暂时失败、容量、顺序和完整操作进展。G/Q/Capstone2、历史测试及 queue_bench 复用实现；[队列记录](../topics/queues/VALIDATION.md)区分旧样本与修复版本。SPMC 通过多消费者配置检查，不冒充专用最优实现。深度：可运行教学结构、回收和有限历史验证。
- **对象被摘除后，什么时候才能真正释放？** 先修 00 的借用和 09 的同步关系，从 [生命周期基线](../topics/reclamation/01-lifetime-and-ownership.md)区分引用计数、标签和保护，再分别进入 HP、EBR、QSBR、RCU 的应用义务。I2/I3/R1/R2 与 reclamation_test 验证保护、退休、长读者、线程退出及删除器完成；[回收记录](../topics/reclamation/06-validation.md)保存启动/异常展开等检查。深度：最小可证明教学协议，不宣称标准实现兼容或完整操作无锁。
- **数据布局和执行策略的收益来自哪里？** 先修 12 的公平比较，再从 [缓存布局](../topics/performance/01-cache-layout.md)、[策略合同](../topics/performance/02-execution-policies.md)走到 [归约扫描](../topics/performance/03-reduce-scan.md)和 [计算项目](../topics/performance/04-parallel-compute.md)。J1/J2、L1–L3、Capstone3、numeric_test 及 layout_bench 共同检查；不同规模和轻重负载用于识别启动成本与可分摊工作，不以固定加速比验收。深度：真实计算内核、独立数值检查和实验解释。
- **向量化怎样保持访问合法与数值可接受？** 先修 13–14 的布局、依赖和结合顺序，再从 [标量与布局](../topics/simd/01-scalar-and-layout.md)进入显式后端、掩码、尾部和 [精度](../topics/simd/03-reductions-and-precision.md)。K1–K3、numeric_test、simd_bench 复用内核；[性能记录](../topics/performance/VALIDATION.md)分开保存严格、有限输入、实际 ISA 和缺能力路径。深度：标量、SSE2、固定 xsimd 实测；原生标准路径以真实探测为边界。
- **任务和页面是否真的位于预期位置？** 先修 06 的收束和 12–13 的测量，再从 [拓扑](../topics/numa/01-topology.md)、[亲和](../topics/numa/02-affinity.md)、[放置](../topics/numa/03-placement.md)进入分片；[静态/动态调度](../topics/scheduling/01-static-dynamic.md)、窃取和任务组合是相关但不同的分支。J3/N1 验证实际 CPU/页，M1/M2 检查调度和完成通道；[调度记录](../topics/scheduling/verification.md)明确单节点、平台及依赖限制。深度：教学实现与探测，未运行的远端条件不写成通过。

## 3. 原有内容怎样迁移

- 原 A/D 重排为执行背景、结果通道和线程使用，补足参数打包与对象所有权；A2 与 C3 进入取消关闭主线。
- 原 B/C/H1/H2 在使用阶段集中讲解；H3 与原子操作及内存模型衔接。
- 原 E/F 进入 08–10 及原子专题，纠正版本混用和过度保证；atomic_ref、fence 有完整专项。
- 原 G/I 与项目2扩充为结构及回收方案族，保留不同保证的分支，并补 MPSC、Michael–Scott、EBR/QSBR。
- 原 L3 的测量方法提前；原 J1/J2 合并布局推导但保留两个练习入口；NUMA 扩充到真实放置和分片实验。
- 原 K/L/M 分别进入 SIMD、并行算法与调度任务组合，项目3保留矩阵乘、归约和排序的实际对照。
- 原18扩展为诊断与源码路线。旧讲义文件保留迁移链接，避免旧链接突然失效。

删除的是重复任务清单、重复复盘和没有依据的固定性能保证；已有核心知识及其改进路线均有明确去向。专题可以有不同阅读优先级，但选定内容的正文、代码、答案和独立审查都属于交付范围。

### 原源码路线与新增生产实现导读

下面均为固定版本的源码导读深度，不表示本仓库重新实现或实测了这些生产库。每篇给出真实符号路径、所有权或状态变化、成功/失败出口、与教学协议的差异及阅读任务答案；版本与一手链接在正文中，独立审查状态仍由质量报告统一维护。

| 来源与问题 | 先修和契约重点 | 实际去向与迁移理由 |
|---|---|---|
| 原 18：Folly MPMC 的 ticket/turn | 有界 MPMC、预订与发布；ready/promised 操作的失败和等待不能混用 | [票据队列导读](../topics/source-reading/02-ticket-and-segmented-queues.md)：把库名清单展开为单槽跨轮次的构造、读取、析构与重用路径 |
| 原 18：moodycamel 的子队列与 token；新增分段队列深度 | SPSC/MPMC 的顺序与元素寿命；每 producer FIFO 不等于全局线性化 FIFO | [分段子队列导读](../topics/source-reading/02-ticket-and-segmented-queues.md)：分开显式/隐式 producer、block 完成和空闲链复用，解释改变的场景与保证 |
| 原 18：libstdc++ atomic 到编译器内建 | 原子操作和内存模型；规范语义不等于固定机器指令 | [原子与 SIMD 后端导读](../topics/source-reading/01-atomic-and-simd-backends.md)：追标准库包装、内建展开及目标/运行库回退，不以平台实现替代语言推导 |
| 原 18：标准库 SIMD 与 xsimd 后端 | SIMD 输入域、掩码、ABI 与 ISA；实验接口不等于标准新接口 | [原子与 SIMD 后端导读](../topics/source-reading/01-atomic-and-simd-backends.md)：追 GCC experimental SIMD、固定 xsimd 的 batch/kernel/dispatch，区分编译期布局和运行时选择 |
| 原 18：生产级 HP/RCU；新增可扩展回收深度 | 保护、退休、宽限期与回调完成；缓存和分片不取消应用生命期义务 | [可扩展回收导读](../topics/source-reading/05-scalable-reclamation.md)：追保护记录缓存、退休分片、线程退出与 RCU 域同步，区分教学域和生产域接口 |
| 最终计划新增：生产级并发哈希表 | 锁、节点所有权与结构变化；accessor 访问资格、扩容和擦除各有边界 | [oneTBB 哈希表导读](../topics/source-reading/03-concurrent-hash-map.md)：追 find/insert/erase、桶与节点锁的衔接、段发布和惰性迁移 |
| 最终计划新增：分配器与完整操作成本 | 对象生命期、链式队列与回收；应用对象安全不由分配器代为证明 | [mimalloc 导读](../topics/source-reading/04-allocator-ownership.md)：追本地分配、远端释放、页归属及遗弃接管，补全 CAS 外部的分配和最终释放责任 |
| 原调度/源码路线：任务组合的实际执行链 | 结果通道、线程池、sender 的三种完成通道；operation state 不等于执行资源 | [sender 与执行链导读](../topics/source-reading/06-senders-and-execution.md)：追固定 stdexec 的 schedule/connect/start、then/when_all、sync_wait 与资源退出，按真实命名空间和分支核查 |

## 4. “覆盖”怎样验收

每个主题都要能回答：前一个方案满足什么契约、为什么出现新问题、改动哪条协议、产生什么新成本、实验怎样证伪预测、哪些结论仍未验证。只列类型名、引用链接或输出一个成功数字，不能据此标记完整覆盖。

学生程序与 Reference 分开运行；代码、实验及答案的版本对应见各题 README 与[构建指南](../exercises/BUILD_GUIDE.md)。无多节点或原生库能力时，保留完整解释、代码和明确 SKIP，不填造性能结果。生产级库的源码导读属于另一种学习深度，必须说明阅读问题与实际关键路径。
