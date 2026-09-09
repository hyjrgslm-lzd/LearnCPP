# 07 计数与阶段同步：区分完成数、阶段和资源许可

四个线程各准备一份输入，主线程等它们全部就绪；三个线程每轮各算一块，所有块都写完才能汇总；八个任务争用三份外部资源。这三个问题都含有“计数”，但计数的含义完全不同：第一种是尚未完成的事件数，第二种属于当前阶段，第三种是仍可领取的资源数。

`latch`、`barrier`、`semaphore` 分别为这些协议提供了标准表达。它们是 C++20 设施，本课程默认以 C++23 编译，规范核对以 [C++26 N5050](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf) 为固定基准。完整实验在 [H1 solution.cpp](../exercises/H1_latch_barrier/solution.cpp) 和 [H2 solution.cpp](../exercises/H2_semaphore/solution.cpp)。原 H3 的 atomic wait/notify 迁往 [08 原子操作](08-atomics.md)，它等待的是原子值变化，不能直接等同于本章的计数协议。

## 1. latch 统计一次性的完成义务

`latch ready(4)` 的 4 表示还有四次完成义务。每个 worker 准备好自己的结果后调用 count_down，主线程 wait，直到计数变为零。latch 不检查线程身份，所以同一线程完成多个独立工作单元后也可以减多次；反过来，同一个工作单元错误减两次，会提前放行并破坏正确性。

H1 的 `latch_start()` 用 `array<int,4>` 存储准备结果。每个 worker 只写自己的数组元素，再 count_down；主线程 ready.wait 返回以后读取全部元素。它不需要把每个 int 都做成 atomic，因为不同线程写不同元素，而 latch 的同步关系使等待完成后的读取可以看到这些写入。这里使用的是普通 int 数组，不是可能共享位存储的 `vector<bool>`。

只用 ready 还不能让 worker 等主线程下令：最后一个 worker 报到之后，其他 worker 也许早就继续执行了。因此 Reference 增加 `go(1)`。worker 报到后等 go；主线程确认所有准备完成且 started 仍为 0，再对 go 减一放行。

这保证“发令之前不能开始”，不保证四个线程在同一时刻运行。机器可能只有一个可用核心，调度器也可能依次恢复线程；不应该以日志时间戳几乎相同作为验收条件。最终检查四个 worker 都开始即可。

## 2. latch 的边界比“倒计数”更具体

构造计数应在 `[0,max()]` 内；count_down 的更新量非负，且不能超过当前计数。计数归零后可以继续 wait，它仍是开放状态，但不能再减一，也没有重置回 4 的成员函数。若需下一批独立完成事件，应创建新 latch，且旧对象的所有使用者已经结束；若是同一组参与者反复汇合，考虑 barrier。

`arrive_and_wait(update)` 组合减少计数与等待。`try_wait()` 允许极低概率返回 false，即使计数已是零；所以程序不能把一次 false 当作肯定还未完成，也不应该写“归零后单次 try_wait 必须 true”的断言。H1 只检查正计数时不能报告完成，确定等待使用 wait。详见 [`[thread.latch.class]`](https://eel.is/c++draft/thread.latch.class)，N5050 32.9.2.3 也保留了这一虚假失败许可。

创建线程失败时，不能让已创建线程永久等一个永远不会到齐的门。H1 的 latch 示例创建失败就放开 go，使已创建任务能退出；此时不进入 ready.wait。原语对象声明在 worker 容器之前，因此异常展开时先等待 worker 结束，再销毁原语。把 latch 计数预设为四，不会自动为第四个线程创建失败补上 count_down。

## 3. barrier 把每次到达归入某个 phase

假设每轮有三个 worker，各写一个部分结果。只有本轮所有写入结束，才能汇总并允许覆盖下一轮的槽位。单个 latch 用完即不能重置，而手工重置布尔值会遇到“快线程把下一轮信号混进上一轮”的代际问题。

barrier 明确维护 phase。每轮的 expected count 由 arrive 或 arrive_and_drop 减少；本轮完成步骤结束后，为下一轮建立新的 expected count。`arrive_and_wait()` 等价于 `wait(arrive())`，在本轮报到并等待本轮的完成步骤。

H1 的 `barrier_phases()` 分配三个普通 int 槽位。第 r 轮第 i 个 worker 写 `(r+1)*10+i`，再 arrive 并 wait。completion 汇总三个槽位，写入预先分配的 totals 数组，推进 phase。四轮总和应该依次为 33、63、93、123。

为什么读写不会重叠？每个 worker 本轮写入在 arrive 之前，arrive 的同步保证把它置于本轮 completion 开始之前；completion 完成后，解除本轮等待的调用才能继续。下一轮覆盖发生在 worker 的 wait 返回之后，因此汇总不会读到半个下一轮。各 worker 写的是不同元素，也不互相竞争。这个协议依赖所有共享槽位的访问都位于相应边界，不能在 arrive 后又随手修改本轮槽位。

## 4. completion 的精确条件

不能笼统说“最后到达的线程一定执行 completion”。N5050 `[thread.barrier.class]` 规定：expected count 到零之后，由某线程在其 arrive、arrive_and_drop 或 wait 调用期间执行一次完成步骤；如果没有任何线程调用 wait，该步骤是否执行是实现定义的。完成步骤调用 completion function，解除本阶段的等待，随后开始下一阶段。其结束 strongly happens before 被该步骤解除阻塞的调用返回。

因此本课程各轮都实际调用 wait，可以检查每轮恰好一次 completion；却不能把同一检查直接搬给一个“所有人只 arrive 后走掉”的程序。也不能通过线程 ID 判断哪个线程“应该”负责汇总。固定版本出处为 [N5050 第 2259 页（PDF 第 2270 页）](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf#page=2270)，便捷条款入口为 [`[thread.barrier.class]`](https://eel.is/c++draft/thread.barrier.class)。

completion 类型需要满足相应移动构造和析构要求，且 `is_nothrow_invocable_v<CompletionFunction&>` 为 true。普通 lambda 的调用默认不满足无抛出要求，所以 Reference 显式写 noexcept。更重要的是函数体真的不应抛出：本例只做固定数组读取、整数累加和写入，cs::check 留到汇合后的主线程。把可能分配或抛异常的日志函数塞进 noexcept lambda，不能靠 noexcept 消除风险，逃逸异常仍会导致 terminate。

对自定义 completion 类型，完成步骤进行时，其他线程调用该 barrier 除 wait 之外的成员有未定义行为限制。这意味着不能把 arrive 当成“我先进入下一轮了”，在本轮 completion 尚未结束时再次 arrive。H1 一律先完成本轮 wait 再开始下一轮，遵守这一前提。

## 5. 拆开到达、等待，以及正式减员

`arrive()` 返回的 arrival_token 关联某个 phase 的同步点。你可以先报到，做不依赖本轮汇总、也不碰汇总使用数据的本地工作，稍后把 token 移入 wait。token 必须属于同一个 barrier 的当前或紧邻的前一 phase；不能缓存很多轮以后才拿来等待。H1 明确展示 `auto arrival = barrier.arrive(); barrier.wait(std::move(arrival));`，便于在这两个位置间检查数据访问边界。

某个 worker 提前永久退出，不能简单 return。其余人还欠这个 worker 的每轮到达，之后会一直等待。`arrive_and_drop()` 同时减少本轮的剩余 expected count，以及以后各轮的初始 expected count。它不等本轮完成，调用者要另行保证自己不再访问由其他阶段管理的数据。

H1 用 `shrink(2, completion)`，一个异步任务 arrive_and_drop，主线程 arrive_and_wait 完成本轮；下一轮主线程单独 arrive_and_wait，验证两个阶段都完成。减员不是强行取消其他参与者，不会自动解决“某人抛异常后没到达”的协议漏洞。构造 expected 为零的 barrier 只能销毁，不能把它当作一个总是开放的 latch。

Reference 在启动多轮 worker 前有一个共享 launch future。若其中一个线程创建失败，主线程发布 false，让已创建线程在接触 barrier 之前退出；全部创建成功才发布 true。这样不把“对象构造成功”误当成“所有阶段参与者已经存在”。如果业务计算可能抛异常，还应设计失败后如何填充结果、drop 或取消整组；原语本身不替应用收尾。本例计算仅固定整数操作，检查均在 worker 结束后进行。

## 6. semaphore 统计可领取资源，不统计已完成阶段

`counting_semaphore<3> permits(3)` 表示初始有三份许可。每次成功 acquire 消耗一份，为零时等待；release 归还许可并使等待者有机会前进。H2 `resource_limit()` 让八个异步任务各进入资源区 100 次，在持有许可期间记录 active，并检查它不超过三。

上限来自业务账本：初始三份、每次成功领取减一、每次结束恰好归还一份。模板参数 3 不是一个自动帮你纠错的“最多并发三人”配置。如果多归还一次，你可能放进第四个人。少归还一次，系统最终可能所有人都等不到资源。

H2 在 acquire 后建立局部 lease，离开资源区时自动减少 active 并 release。即使 cs::check 抛异常，许可也能归还，异常由 async future 带回主线程。最终检查 completed 为 800、active 为零、峰值在 `[1,3]`，不要求恰好达到三。调度完全串行也满足正确限流协议；没有峰值三不代表信号量失效。

许可不等于某个具体资源的引用。如果三个线程要取出连接池中的三条连接，仍需保护连接容器的 pop/push 或分配槽位。三份许可允许三个线程同时进入，它不会让这三个线程对同一普通计数器的写入自动互斥。本例 active 和 peak 用 atomic，业务 payload 的独占来源另行定义。

## 7. least_max_value 与 max() 不要混淆

`counting_semaphore<least_max_value>` 的模板参数要求实现支持至少这么大的计数；实际最大计数由 `max()` 给出，可能更大。构造初始值必须非负且不超过 max；release 的 update 必须非负，并且加上后不超过 max。不能写“超过模板参数就必定 UB”，也不能写“实现会替你把计数饱和到模板参数”。规范入口为 [`[thread.sema.cnt]`](https://eel.is/c++draft/thread.sema.cnt)。

可移植业务通常把自己使用的最大值控制在模板要求的范围内。`binary_semaphore` 是 `counting_semaphore<1>` 的别名；本章按最多一个未消费信号的协议使用它，不依赖实现的 max 是否恰好为一。不能对同一未消费信号无节制 release，期待它像可合并事件那样自动折叠。

`try_acquire()` 可以虚假失败，因此 false 只表示本次没有获得许可，不能当成一份精确的资源数量快照。try_acquire_for/until 给出等待期限，但仍受调度影响，实际返回可能晚于期限。H2 先用普通 acquire 领完三份许可，在保证无人会归还时检查 try 和 timed try 失败，随后 release(3) 恢复。这里没有要求“有一份就必须一次 try 成功”。

## 8. 无所有权使跨线程交棒成为合法协议

mutex 由获取它的线程负责解锁；semaphore 没有这种线程所有权，所以 A 线程 release、B 线程 acquire 是正常用法。H2 `handoff()` 用 ready 与 ack 两个二值信号量，让主线程写 payload，通知消费者读取，再等待消费者确认后才覆盖下一条值。

```cpp
payload = i;
ready.release();
ack.acquire();
```

消费者对应 `ready.acquire(); read(payload); ack.release();`。ready 的交接把写入置于消费者读之前，ack 的反向交接把这次读置于下一次覆盖之前，因此 payload 是普通 int 也可以安全使用。只保留 ready、不等 ack，生产者可能在消费者还没读完时覆盖，就破坏了寿命与访问顺序。

消费者不能在发 ack 之前直接抛出断言异常，否则主线程还在等一份永远不会到来的许可。Reference 先在本地记录正确性，每一轮继续完成握手，全部结束后把 bool 交给主线程检查。100 次 ready/ack 是本例的有限协议，不用 sleep 猜测读取完成。

同理，用 semaphore 代替 latch 时需要 `release(N)` 发出 N 份许可，每个等待者领取一份；这与 latch 归零后所有未来 wait 都通过的状态不同。用两个 semaphore 计空位/满位可以构造有界缓冲，但还需要槽位所有权、失败归还和 close 唤醒的协议；上一章的 bounded_channel 已经明确这些行为，不应仅把 CV 替换成两个计数就宣称等价。

## 实验与自测答案

```powershell
# 在 C08_Concurrency/exercises 下，H2 可替换同样的题名与目录
cmake -S H1_latch_barrier -B build/h1 -G "Visual Studio 18 2026" -A x64 -DBUILD_TESTING=ON -DCONCURRENCY_STUDY_TEST_STARTERS=OFF
cmake --build build/h1 --config Release
ctest --test-dir build/h1 -C Release -R '^H1_latch_barrier_reference$' --no-tests=error --output-on-failure
```

这些是 Reference 的验收结果。H1 的独立 Starter 要求完成 ready/go 交接、completion、arrival_token 等待、drop、启动门五个局部接口；H2 要求完成资源许可范围、定时获取与发送/接收交棒。每个接口都有真实调用它的检查，H2 还注入内容不匹配检查“错误也必须 ack”的路径。完成 TODO 并设置各 part*_done 后，运行学生入口：

```powershell
cmake -S H1_latch_barrier -B build/student-H1_latch_barrier -G "Visual Studio 18 2026" -A x64 -DBUILD_TESTING=ON -DCONCURRENCY_STUDY_TEST_STARTERS=ON
cmake --build build/student-H1_latch_barrier --config Release --target H1_latch_barrier
ctest --test-dir build/student-H1_latch_barrier -C Release -R '^H1_latch_barrier_student$' --no-tests=error --output-on-failure
```

原样 Starter 在启动并发前返回 1，student 测试预期失败，不进入默认 Reference 门禁。H2 的两套具体命令见其 README。验收检查准备先于放行、四轮汇总、drop 后计数、资源上限及握手结果，不要求物理同时性或特定加速。worker 结果通过 future.get 回传；错误协议的无限等待由 CTest 30 秒超时限制。

**“等待三件事”为什么不一定是三个线程？** latch 统计完成义务，同一线程可以顺序完成三件事；关键是计数与实际业务义务一一对应。

**barrier 能否当一次性的门用？** 可以构造一个只使用一轮的协议，但 latch 更直接。barrier expected 为零不能充当开放门。

**为什么每轮只需要一个 barrier 就能汇总 slots？** 汇总发生在 completion 内，所有本轮 arrive 都在它之前，下一轮写在 wait 返回之后。如果把汇总移到某个 worker 的 wait 后，而其他 worker 立即写下一轮，就需要第二个阶段或双缓冲协议。

**completion 抛异常应如何传播？** 本例不允许。把业务错误作为结果数据交给不抛出的 completion，或者在阶段之外设计异常通道；不能从 noexcept completion 抛到主线程。

**一个 worker 不再 arrive 会怎样？** 若没有按协议 drop，本轮或下一轮还欠一次到达，其他参与者可能一直等待。自动 join 不会补计数。

**semaphore 可以用于互斥吗？** 初始一份且严格配对时可以建立互斥，但没有 mutex 的线程所有权约束和标准 lock_guard 的直接配合。跨线程信号与多份资源是 semaphore 更直接的用途。

**为什么峰值小于三不算失败？** 三是上界，调度不承诺所有可用许可都同时被使用。验收上界与性能利用率是两项不同问题。
