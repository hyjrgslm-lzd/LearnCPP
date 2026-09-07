# 04 共享状态与锁：先保护一个完整事实

一个线程把配置版本改成 12，另一个线程同时读取配置。我们真正希望读到的不是某个整数“没撕裂”，而是一个自洽的版本：版本号、由版本派生的值、相关容器内容应该属于同一次更新。锁解决的问题由这些关系决定，不能从“给每行代码加一个 mutex”开始。

本章对应 [B1 完整 Reference](../exercises/B1_mutex_family/solution.cpp)、[B2 完整 Reference](../exercises/B2_deadlock_scoped_lock/solution.cpp) 和 [B3 完整 Reference](../exercises/B3_call_once/solution.cpp)。先阅读代码中的 `config::snapshot`、`transfer` 和 `local_resource`，再沿下面的推导解释它们为何成立。所有默认路径均安全；错误诊断只在编译选项和命令行双重开启后执行。

## 1. 数据竞争不等于“结果偶尔少一点”

考虑两个线程对同一个普通 `int` 做 `++counter`。它们对同一内存位置进行冲突访问，至少一个写入，且没有建立所需的 happens-before 关系。这属于数据竞争，程序行为未定义。是否在墙上时钟的同一纳秒访问、CPU 是否恰好生成一条自增指令、最后数字是否等于预期，都不能改变这个判断。标准规则按抽象机的访问和同步关系表述，见 [N5050 的 `[intro.races]`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf)。

“两个线程都读到 0，然后都写 1”是帮助理解丢失更新的一个交错模型，但它不是对 UB 结果的约束。不能断言最终值一定小于等于正确值，更不能把它放进性能表当作“无锁快版本”。本章 B1 默认只运行正确实现。选做的 `--unsafe-race` 用于诊断工具，不设置期望数字，不以输出正常证明安全。

正确计数的关键边界在完整的读改写外面：

```cpp
std::lock_guard lock(mutex);
++counter;
```

同一把 mutex 把所有自增排成互斥的临界区；join 或 future 完成后的主线程再读取最终值。B1 的 `counter_and_config()` 用四个实际异步任务各自增 2000 次，以 Release 下仍生效的 `cs::check(counter == 8000, ...)` 验收。锁并不自动附着在 counter 上：任何绕过它的访问都会破坏论证。

## 2. 从单个变量到跨字段不变量

B1 的 `config` 有两个字段：`revision` 和 `twice_revision`。不变量是 `twice_revision == 2 * revision`。写者在一次独占临界区内更新两者；读者在一次共享临界区内把两者复制成一份快照。

如果把两个字段各改成 atomic，仅能得到两个各自合法的原子读写。读者仍可能在新 revision 写完、twice_revision 尚未更新时看到混合版本。消除数据竞争与实现复合操作的原子性是两项要求。这里的快照操作必须作为整体与更新互斥。

如果 getter 返回字段引用，持锁读取引用本身并没有把保护带出去。调用者使用引用时，锁已经释放，写者可能正在改它；容器还可能让引用失效。Reference 因此返回 `std::pair<int,int>`，把锁保护的共享对象转成调用者拥有的值。换成大型配置时，可以研究不可变快照的共享所有权，但不能省略寿命协议。

`shared_mutex` 允许多个共享持有者；独占持有者与其他共享、独占持有者互斥。它不会判断操作在逻辑上是不是“读”。一个 `const` 成员函数若更新可变缓存，仍然需要保护写入。反过来，读者用独占锁是正确的，只是读者之间也会串行化，并不是编译错误或数据竞争。规范入口为 [`[thread.sharedmutex.requirements]`](https://eel.is/c++draft/thread.sharedmutex.requirements)；该链接是滚动草案，版本判定以 N5050 为准。

不要由“读占 99%”推导必然加速。共享锁需要内部簿记，实际成本取决于临界区长度、竞争、实现和硬件；标准也不保证写者优先、公平或无饥饿。B1 验证每次快照自洽及最终 `{2000,4000}`，没有设置“必须更快”“必须出现多个同时读者”这样的调度假设。

## 3. 锁对象为何需要寿命与所有权

手工 `lock(); work(); unlock();` 在 `work()` 抛异常时漏掉解锁。RAII 把“谁负责释放”绑定到局部对象的寿命。`lock_guard` 构造时获取锁，离开作用域时释放；`std::lock_guard<std::mutex>{mutex};` 这样的无名临时对象在分号处已经销毁，不能保护下一句。

`unique_lock` 额外记录它关联哪把 mutex、当前是否拥有锁。因此可以延迟加锁、提前解锁、重新加锁，并移动所有权。B1 `ownership()` 依次检查 defer 状态、移动后的源/目标状态，然后保存本地快照并解锁，让另一个线程修改共享值。异步写者的 `get()` 返回之后，快照仍是 42，新共享值已是 7。若忘记解锁，写者无法结束，外部 CTest 超时会暴露问题。

提前解锁之后不能继续使用受保护对象的引用，也不能假定先前条件仍为真。比如“解锁计算折扣，再把结果写回”可能覆盖其他人的更新；此时要在重新加锁后检查版本，或者把整个操作重新设计为原子业务步骤。缩短临界区必须先确定哪些内容已独立拥有。

移动 `unique_lock` 不等于可以把普通 mutex 的解锁义务搬到另一个线程。普通 mutex 仍要求由拥有它的线程解锁。`adopt_lock` 则只接管已经拥有的锁，根本不执行加锁；未持锁时使用它违反前提。相关接口见 [`[thread.lock.unique]`](https://eel.is/c++draft/thread.lock.unique) 与 [`[thread.lock.guard]`](https://eel.is/c++draft/thread.lock.guard)。

`recursive_mutex` 允许同一线程重复获取，每次获取都必须配一次释放。它只解决重入获取的机制，不能修复“递归调用看到了尚未恢复的不变量”。若 public 方法相互调用，可考虑 public 负责加锁、private helper 要求调用者持锁；这往往使边界更清楚。`recursive_timed_mutex` 同时支持递归和定时获取。B1 运行一次嵌套 RAII 获取，默认不执行普通 mutex 自重入反例。

`timed_mutex` 的 `try_lock_for` 和 `try_lock_until` 提供放弃本次获取的选择。它们可能虚假失败，超时也不是实时调度保证，不能规定“恰好 50ms 返回”。B1 的 `timed_and_recursive()` 由主线程持锁，异步尝试结束前主线程不释放，因此能确定本次尝试不可能成功；随后用普通阻塞获取验证锁仍可用。顺序来自所有权和 future，完全不靠 sleep。`shared_timed_mutex` 为共享与独占获取都增加定时接口；选择它需要真实的期限需求。

## 4. 两把锁：业务原子性与获取算法是两回事

账户 A 向 B 转账时，只锁 A 扣钱、解锁后再锁 B 加钱，会让并发观察者看到总额短暂变少；如果第二步失败，还可能留下永久不一致。B2 的 `transfer` 同时保护两个账户，在更新前检查金额与余额，然后一次完成扣减和加款。观察两个账户的总额也必须遵守同样的锁集合，否则观察者不能要求原子快照。

朴素地“先锁付款方，再锁收款方”有问题：A→B 线程持有 A 等 B，B→A 线程持有 B 等 A，形成等待环。传统资源模型把互斥、持有并等待、不可抢占、循环等待列为死锁的必要条件。这能帮助画等待图，但不能取代对程序其他依赖的检查，例如持锁等待一个也需要该锁的 future。

```cpp
if (&from == &to) return;
std::scoped_lock lock(from.mutex, to.mutex);
// 检查余额，然后同时维护两个账户的不变量。
```

这里必须先处理别名。同一个非递归 mutex 不能作为两把不同资源重复交给多锁算法。

`scoped_lock` 正常构造返回时拥有这些锁，多锁情形使用 `std::lock` 的死锁避免算法。标准允许适当组合 lock、try_lock、unlock，具体顺序未指定。它并没有把多把 mutex 变成一条硬件原子指令；获取途中可能暂时拥有其中一部分，失败回退也可能涉及重试。应表述为“避免这些锁在获取过程中的死锁”，不能宣称“从不持有部分锁”“永不阻塞”或“全程序不会死锁”。见 [`[thread.lock.algorithm]`](https://eel.is/c++draft/thread.lock.algorithm)。

比如调用前已拿着第三把锁，任务完成前又等待另一个线程，而对方需要第三把锁，循环仍在多锁算法之外。锁获取没有截止时间或公平保证，也不能回滚锁内任意业务操作。B2 之所以有简单异常保证，是验证发生在修改前，随后整数操作在本题输入范围内不抛异常且不溢出。

另一种方法是制定全局锁顺序。B2 的第三个版本用 `std::less<const void*>` 对账户地址建立一致严格全序，不能用不相关对象指针的内建 `<` 冒充可移植总序。固定且唯一的账户 ID 也可用。所有调用路径必须遵守同一排序，资源寿命必须覆盖获取过程。层级锁是在运行时检查这种纪律的工程手段，不会自动发现所有 future、I/O 或回调依赖。

B2 对 scoped_lock、`std::lock + adopt_lock`、固定全序三个版本分别做 1000 次双向转账，检查最终余额恰好为 11000 和 9000，并检查自转账无变化。错误诊断 `--unsafe-deadlock` 用 barrier 确定两个线程都持有第一把锁，再请求另一把；一旦显式开启，必须由外部超时结束它。

## 5. 只初始化一次，不等于对象永远线程安全

B3 的 `once_flag` 表示一次初始化协议。一次进入初始化函数的调用称为 active；抛异常的是 exceptional；成功返回的是 returning；不运行初始化体的调用是 passive。同一 flag 至多有一次 returning，失败尝试可以有多次。active 调用之间有标准规定的全序和同步关系，成功初始化与 passive 返回之间也有同步关系，因此其他调用者在 call_once 返回后可以看到完整结果。见 [`[thread.once.callonce]`](https://eel.is/c++draft/thread.once.callonce)。

Reference 故意让第一次尝试抛异常，第二次把参数 123 存入 `unique_ptr<int>`。八个调用者各自通过 call_once 后检查值，最后检查 attempts 为 2、失败数为 1。attempts 只在串行化的 active 调用里写，主线程在全部 future 完成后读，因此无须为它额外添加 atomic；失败计数由不同异常处理线程更新，所以使用 atomic。

call_once 不做事务回滚。若初始化先发出外部消息再抛异常，下一次可能重复发送；应先在局部构造完整资源，成功时才发布。也不要让同一初始化函数递归等待同一个 flag 完成，那会把自己变成自己的依赖。

函数内 `static` 的初始化也有并发等待和失败后重试语义，B3 的 `local_resource()` 用它构造只读整数并验证只构造一次。它适合该函数拥有的长寿命对象；once_flag 适合嵌在某个对象里，保护由该对象寿命管理的初始化。两者都不保护初始化完成后的任意修改，不替你解决静态析构期间其他线程访问的问题。局部静态初始化中递归进入同一声明也有专门的未定义行为规则，见 [`[stmt.dcl]`](https://eel.is/c++draft/stmt.dcl)。

错误的 double-checked locking 在锁外读取普通指针，另一个线程可能在锁内写它，锁外读没有参与同步。即使地址看起来已写入，也不能据此推导对象已安全发布。用标准初始化协议可直接消除这个需要额外证明的分支；发布机制的进一步推导见后续内存模型章节。

## 6. 实验与自测答案

从 `Concurrency_Study/exercises` 运行，替换题名可检查 B2、B3：

```powershell
cmake -S B1_mutex_family -B build/b1 -G "Visual Studio 18 2026" -A x64 -DBUILD_TESTING=ON -DCONCURRENCY_STUDY_TEST_STARTERS=OFF
cmake --build build/b1 --config Release
ctest --test-dir build/b1 -C Release -R '^B1_mutex_family_reference$' --no-tests=error --output-on-failure
```

上面只运行 Reference，改 main.cpp 不会改变该目标的结果。每题 main.cpp 现在提供独立学生函数、逐 Part 中文提示和调用这些函数的检查；例如 B1 的 student_config 必须维持同一快照不变量，B2 的三种 student_transfer 接受同一组转账输入。按 TODO 完成后把对应 part*_done 改为 true，再运行学生入口：

```powershell
cmake -S B1_mutex_family -B build/student-B1_mutex_family -G "Visual Studio 18 2026" -A x64 -DBUILD_TESTING=ON -DCONCURRENCY_STUDY_TEST_STARTERS=ON
cmake --build build/student-B1_mutex_family --config Release --target B1_mutex_family
ctest --test-dir build/student-B1_mutex_family -C Release -R '^B1_mutex_family_student$' --no-tests=error --output-on-failure
```

未完成 Starter 在创建任何线程前输出 INCOMPLETE 并返回 1，学生测试失败是预期，不纳入默认 Reference 门禁。完成标记不代替实际检查，错误实现依然失败或触发 30 秒超时。每题 README 都列有它自己的两套命令。危险参数只由 Reference 可执行文件处理：独立诊断时开启 `CONCURRENCY_STUDY_ENABLE_UNSAFE_DEMOS`，显式传 `--unsafe-race` 或 `--unsafe-deadlock` 并设置外部超时；Starter 不执行它们。

**为什么检查“不崩溃”不够？** 配置可能偶尔读到 `{12,22}`，既没崩溃也都在合理范围。必须检查跨字段关系；本次全部通过仍不能替代锁协议推导。

**为什么不能逐字段加锁后拼快照？** 两次临界区之间允许一次完整更新，拼出来的字段不一定属于同一版本。把快照定义成一次操作，锁住整个读取过程。

**unique_lock 一定更慢吗？** 接口维护了更多状态，但对象大小、内联优化和运行耗时是实现与上下文问题。本章没有测量结论，不能凭接口大小排序性能。

**scoped_lock 的“原子性”在哪里？** 业务更新对遵守相同锁协议的观察者不可拆开；多把锁的获取本身是避免死锁的算法，两个概念不能混用。

**call_once 是否保证初始化体执行恰好一次？** 至多一次成功返回；失败可以重试。如果从来没人调用，或所有调用都失败，成功次数也可以是零。

**为什么 shared_lock 在 C++14 已有、shared_mutex 是 C++17？** C++14 已有 `shared_timed_mutex` 与共享持锁包装器；不带定时接口的 `shared_mutex` 后来补充。这是版本知识，不构成任何特定实现性能承诺。

下一章把“拿不到业务条件就等待”加入共享状态协议；锁负责保护状态，条件变量负责协调何时重新检查它。
