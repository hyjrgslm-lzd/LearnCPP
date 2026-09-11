# 17：从现象、证据和源码找到原因

并发程序出错时，现象经常离原因很远：一条旧指针可能在另一个线程稍后才被解引用；队列只丢了一个元素，最终表现却是所有消费者一直等不到目标计数；一次日志恰好增加了同步，错误在打开日志后消失。

这一章把前面已经用过的检查组织成一条诊断路径。先明确违反了什么契约，再缩小到可以解释的状态变化，最后选择能提供对应证据的工具。完整程序和已知检查分别在 [Q0](../exercises/Q0_queue_baseline/README.md)、[队列历史测试](../exercises/runtime_tests/queue_history_test.cpp)、[回收测试](../exercises/runtime_tests/reclamation_test.cpp)和[数值测试](../exercises/runtime_tests/numeric_test.cpp)。

## 1. 先给“错”一个准确名字

考虑三个看上去都像“结果不对”的情形：

- 两个线程无同步地修改同一个普通整数，属于数据竞争，程序不满足语言规则。
- 两个线程用原子 load、计算、store 更新计数，各次访问都原子，但会覆盖别人的更新；这是协议的逻辑错误，不需要发生数据竞争。
- 一个合法并行归约改变了浮点结合顺序，与顺序参考值略有差异；需要先看数值契约和误差预算。

相同的“最后结果不相等”，需要三种不同的论证。第一种应消除非法访问；第二种应调整整个更新协议；第三种需要判断是否超出约定的误差。不能看到 atomic 就排除所有逻辑问题，也不能看到浮点差异就断定内存序错误。

先写一句失败描述，例如：“生产者成功提交的 ID 17 没有出现在最终集合”“读者离开保护区前对象已经析构”“关闭后仍返回一个永远不能就绪的 future”。这种描述比“偶尔卡死”更容易连接到具体不变量。

## 2. 缩小到一条能解释的执行

假设一个有界队列偶尔不能排空。先把容量缩小到 2，把生产者与消费者减少到一两名，给元素分配唯一 ID。在发生问题之前放置受控门闩：生产者已经取得某个票据，但尚未发布槽位；消费者此时尝试取出。

你要分别记录以下事件：

```text
P：进入 enqueue → 取得票据 → 写入 payload → 发布 sequence → 返回
C：进入 dequeue → 观察 sequence → 取得 payload → 释放槽位 → 返回
```

“取得票据”和“可以读取 payload”是两个不同的时刻。暂停在这两者之间，可以解释队首停顿，而不是期待压力测试碰巧制造一次足够明显的停顿。

记录历史时，还要知道调用的开始和返回区间。两个并发调用可能重叠，操作可以在各自区间内选择合法的生效位置。[历史检查器](../exercises/include/concurrency_study/queue_checks.hpp)对小规模记录搜索符合该实现契约的顺序。它不是无限状态模型检查器，也不能消除记录动作本身对调度的影响。

先为检查器构造一个应接受的历史和一个应拒绝的历史。否则，一个永远返回 true 的“检查器”也可能给出漂亮的测试通过结果。

## 3. 日志为什么会改变问题

课程的 [log.hpp](../exercises/include/concurrency_study/log.hpp)用 osyncstream 组织完整输出。这样可以避免同一条消息的片段与其他合作输出混在一起，但并不让输出时刻成为业务操作的生效时刻。

构造字符串、分配缓冲、输出和串行化提交都会消耗时间，也可能增加同步关系。如果在本来用于观察内存序的关键窗口内插入日志，你研究的就已经是另一个程序。日志消失或错误消失都不能自动证明算法被修好了。

对短协议实验，优先在每个线程自己的记录区保存少量事件，join 后汇总。若需要一个全局事件序号，明确说明这个原子记录动作会影响程序；用它重建小历史，不把该版本的耗时作为原始算法吞吐。

时间戳同样有边界：steady_clock 适合测持续时间，但时间先后不是语言层的 happens-before。一个线程“打印得早”不能代替对共享数据的发布协议。

## 4. 挂起时先画等待图

下面是一个常见的关闭死锁：

```text
main：析构 future，等待 worker 结束
worker：等待 release 信号
main：负责 release 的对象，要等 future 析构之后才销毁
```

这不是“future 不够智能”，而是析构顺序与信号所有权形成了环。修复时，要让异常展开也先放行 worker，再等待它退出。正常路径里显式 set_value 一次，只能覆盖正常路径，不能覆盖它前面的分配失败。

回收专题的失败注入检查会区分“worker 在发送 started 前失败”和“worker 已停住后 main 失败”。观察主线程最终能否取得原始异常、是否收束所有线程，以及仍发布或已退休的对象由谁负责清理。相关记录见[回收验证说明](../topics/reclamation/06-validation.md)。

锁死锁也按同样方式画图：线程 A 持有 X 等 Y，线程 B 持有 Y 等 X。多锁协议、同池 worker 的嵌套等待、关闭过程中的 join 都可以放入这张图。超时只能观察到截至某个期限未完成；是否形成等待环仍应从代码关系解释。

## 5. 用外部进程边界限制诊断

死锁线程可能无法响应自己的停止标志，析构中的 join 也可能让程序不能执行到内部超时检查。因此，实验运行器要在进程外设置上限。

普通 Reference 的 CTest 已有超时。需要手动运行某个诊断时，可在 `C08_Concurrency/exercises` 使用：

```powershell
python tools/run_diagnostic.py --timeout 5 -- ./build/verify-core/B2_deadlock_scoped_lock/Release/B2_deadlock_scoped_lock_reference.exe --unsafe-deadlock
```

在默认构建中，这条命令应报告危险路径未启用。只有明确选择独立诊断构建时才开启：

```powershell
cmake -S B2_deadlock_scoped_lock -B build/diagnostic-B2 -DCONCURRENCY_STUDY_ENABLE_UNSAFE_DEMOS=ON
cmake --build build/diagnostic-B2 --config Release
python tools/run_diagnostic.py --timeout 5 -- ./build/diagnostic-B2/Release/B2_deadlock_scoped_lock_reference.exe --unsafe-deadlock
```

预期的诊断现象是外部运行上限到达后终止该进程；这仍记录为一次失败运行，而不是一般正确性测试通过。不要直接在正常测试套件里调用这个永久等待的路径。

真实数据竞争或释放后访问属于未定义行为，不能承诺每次都能观察到某种输出。安全的状态模型、明确的反例推导和受控交错应该先于真实 UB 的诊断；后者还需要适用工具与隔离边界。

## 6. 工具各自能给出什么

| 工具或检查 | 主要证据 | 重要限制 |
|---|---|---|
| Release 有效断言 | 当前运行是否维持检查的不变量 | 只覆盖实际输入和交错 |
| ASan | 已执行路径的部分地址访问与释放错误 | 不等于数据竞争检测或完整泄漏证明 |
| TSan | 支持平台上被执行路径的数据竞争报告 | 未报告不证明任意协议或进展保证 |
| 小规模历史检查 | 当前历史能否满足声明的顺序契约 | 有限规模，记录会扰动执行 |
| 编译器向量化诊断与汇编 | 某个具体构建生成了什么 | 不自动解释真实运行的瓶颈 |
| profiler/硬件计数器 | 采样或计数口径下的热点与事件 | 依赖硬件、权限、采样和归因条件 |

本机实际运行过的工具与范围应写入本机验证记录。ASan 和 TSan 的构建应分别安排；Windows 上的某个 ASan 成功记录不能替代 Linux/其他平台的 TSan 记录。工具使用与限制以 [Clang ASan](https://clang.llvm.org/docs/AddressSanitizer.html)、[Clang TSan](https://clang.llvm.org/docs/ThreadSanitizer.html)、[MSVC ASan](https://learn.microsoft.com/en-us/cpp/sanitizers/asan)等官方说明为准。

## 7. 检查“标量”代码是否已经向量化

SIMD 专题提供[诊断源](../topics/simd/vectorization_probe.cpp)，包装函数调用与测量相同的内核。可以对这个源生成诊断与汇编，观察普通循环是否已经出现 packed 指令，再理解显式 SIMD 的增量作用。

在已初始化 MSVC x64 开发环境的仓库根目录，按[数值验证脚本](../topics/performance/verify-numeric.ps1)生成相应构建与诊断。脚本和[专题解释](../topics/simd/04-diagnostics-and-bandwidth.md)记录了实际选项、结果及限制。

函数名叫 scalar 只描述源码的写法，不保证生成的机器代码没有 SIMD。类似地，使用 par 表示允许采用某种执行策略，不证明本次运行实际使用了某个线程数量。源代码意图、生成代码和运行观察应逐层核实。

## 8. 带着一个问题读生产实现

源码导读现在有六篇完整正文。每篇从公开入口追到真实关键函数和字段，说明状态/所有权变化、成功与失败出口、释放或复用条件，并提供可操作搜索任务和完整答案。它们覆盖旧 18 的阅读对象，也补上生产级并发哈希表、分段队列、分配器与可扩展回收方案族；深度是读懂生产源码及其契约，不是要求读者手写这些产品。

**版本边界：所选版本是为可复验选择的源码快照，不声称上游最新版本。** 例如 Folly 采用 2024 年快照，是为了让函数、字段与已知边界可以复核；“现代 C++ 学习路线”不意味着下面每个库都是 latest，也不构成部署版本推荐。各篇记录完整 commit 和 2026-09-08 的只读核查范围，未因阅读源码而安装运行依赖、执行生产库或生成性能结果。课程既有库实验的证据仍归各自实验记录，不能算成本系列的新实测。

当前 C++26 规范状态继续从[标准索引](../references/standards-and-implementations.md)进入。标准、TS、第三方扩展与具体提交是不同维度：GCC experimental SIMD 不自动等于 N5050 std::simd，Folly 的回收接口不自动等于标准 HP/RCU，stdexec 的 static_thread_pool 也不因实现了 scheduler 就成为同名标准类。

| 连续正文 | 明确追踪的主路径 | 读完应能交付的证据 |
|---|---|---|
| [01 原子与 SIMD 后端](../topics/source-reading/01-atomic-and-simd-backends.md) | libstdc++ atomic → GCC 内建展开 → 目标/运行时回退；GCC SIMD ABI/掩码；xsimd batch/kernel/dispatch | 指出实际含锁的条件分支、CAS 两个出口，以及编译期 ABI 与运行时 ISA 分派的区别 |
| [02 票据与分段队列](../topics/source-reading/02-ticket-and-segmented-queues.md) | Folly MPMCQueue ticket/turn；moodycamel producer/token → block 索引 → 完成标记 → 块复用/FreeList | 区分 ready/promised、producer 内顺序与全局 FIFO、显式/隐式 producer 的回收出口 |
| [03 并发哈希表](../topics/source-reading/03-concurrent-hash-map.md) | oneTBB concurrent_hash_map → bucket/node 两层锁 → accessor → 分段扩容/按桶迁移 → erase/delete_node | 画出访问资格与摘除/释放的交接，并列出 mask race、候选节点与扩容失败分支 |
| [04 分配器所有权](../topics/source-reading/04-allocator-ownership.md) | mimalloc 本地分配 → 远端 free → page/heap 两级归还 → 页退休 → abandoned/reclaim | 说明应用对象回收、块复用、页面缓存与系统存储归还是不同阶段 |
| [05 可扩展回收](../topics/source-reading/05-scalable-reclamation.md) | Folly holder/TLS cache → protect 复核 → domain 分片/扫描/cohort；RCU 读者记录 → 两阶段宽限期 → executor | 区分保护记录与退休对象，指出平台 fence 配对和该快照 rcu_barrier 的回调完成边界 |
| [06 sender 与执行链](../topics/source-reading/06-senders-and-execution.md) | stdexec schedule/connect/start → operation state → then/when_all 完成 → sync_wait 与资源退出 | 追到状态保存、错误/停止传播、子操作全部到达和外层完成的具体代码 |

### 三轮阅读，逐步增加证据

第一轮看形状：先列公开接口、返回语义、模板分支和关键字段，画出对象关系。不要用“一个 MPMC 库”“一个 RCU 库”替代明确类型和配置。本系列为每条主线选定了分支，例如 Folly 固定容量、oneTBB 默认锁、mimalloc 普通小块；其他分支是边界，不假装已全部覆盖。

第二轮补协议：选一个元素、节点、参数包或回调，从进入接口开始一直追到返回及最终释放。给每个动作标明执行者、原子序/锁、所有权和失败出口。每篇的阅读任务要求查实际函数体；搜索出符号后，还要读完整相关分支，不能把注释里的旧 API 名当作真实定义。

第三轮解释取舍：回到课程的对应实现和规范，回答生产版本比教学基线多承担了什么、改变了哪些承诺。借助固定提交的设计注释与原始资料解释原因，但把源码事实、自己的协议推导和运行实测分开。旧快照中已经标注的问题也应保留，不能为了把生产库写成“标准答案”而略去。

### 不安装依赖也能复验

每篇都有带完整 SHA 的一手文件链接，可以直接在网页搜索指定函数与字段。若已经有相应仓库，先 `git -C $src rev-parse HEAD` 核对快照，再执行文中的 rg 搜索；HEAD 不同时，使用已有对象库中的 `git show <完整SHA>:<路径>` 查看指定版本，或回到固定网页。没有对应提交对象时不把当前 HEAD 的结果冒充该快照，也无需为本作业拉取一套新的运行依赖。

提交阅读答案时保留五项：完整 SHA 与文件、入口到出口的函数链、关键状态图、失败/回收分支、与教学契约的一条具体差异。源码阅读没有运行测试时，就写“只读核查/推导”；如果进一步研究性能、机器码或并发历史，另列实际环境和产物。不能因为库头文件里出现 lock-free、SIMD 或 scheduler 就给整个调用增加未经验证的保证。

原有的扩展方向仍可继续：本机 STL 的 future/线程等待、所用并行算法后端、Concurrency Kit epoch/队列与 Userspace RCU。它们的公共入口保留在标准索引和相应回收/调度专题中；本节六篇的完成声明只覆盖各篇明确绑定快照并逐路径核查的范围，不把扩展库名列表算作又完成了一篇源码导读。

## 自测与答案解析

**为什么计数相等还可能丢数据？** 遗漏和重复可以相互抵消。逐项 ID、内容和按契约检查顺序，才能发现这些不同的失败；即便这些检查都通过，也只描述本次运行。

**关闭后没有新提交，为什么仍可能挂住？** 已存在的等待者可能没有被唤醒，或者异常路径形成了“等待 worker—等待 main 放行”的环。需要检查关闭生效位置、所有等待谓词和析构顺序。

**两个版本的耗时不同，何时可以讨论原因？** 先确认输入、容量、线程角色、保证和计时范围可比，再检查差异是否稳定并收集相应热点证据。否则，结果只能说明两次执行耗时不同。

**为什么没有多节点硬件时允许 SKIP？** 实验要求验证真实 CPU 与页面节点；不存在的硬件条件不能用虚构数值补齐。代码、协议和探测仍完整提供，运行报告清楚保留这个边界。

**一次源码阅读怎样算完成？** 给出你追踪的版本、入口、关键状态与退出路径，画出一条完整数据或控制链，再说明一个具体设计选择的收益、代价和前提。只记住文件名和几个 API 名称还没有完成这个过程。
