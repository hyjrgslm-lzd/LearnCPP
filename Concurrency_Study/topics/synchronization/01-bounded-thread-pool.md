# 同步专题 01：把已接受任务的责任交给一个有界线程池

线程池的目的不是把同步函数换个名字。调用 submit 的线程应该把任务交给常驻 worker，并拿到一个稍后才就绪的结果通道。任务可以返回值，也可以失败；队列满时提交者需要等待；关闭后不再接收，而先前接受的任务仍然有执行义务。这些行为共同构成这个项目，不能用“submit 当场调用函数”通过几个结果相等检查就宣布完成。

实际实现是 [`cs::thread_pool`](../../exercises/include/concurrency_study/thread_pool.hpp)，存储来自同一份 [`cs::bounded_channel`](../../exercises/include/concurrency_study/bounded_channel.hpp)。[Capstone1 Reference](../../exercises/Capstone1_thread_pool/solution.cpp) 与 [runtime_tests/thread_pool_test.cpp](../../exercises/runtime_tests/thread_pool_test.cpp) 都用本题 [checks.hpp](../../exercises/Capstone1_thread_pool/checks.hpp) 的类型参数化检查验证公共实现。独立 [Starter](../../exercises/Capstone1_thread_pool/main.cpp) 则将 student_pool 传给同一组检查，没有包含公共池答案；复用已学的通道是允许的。生产消费者和调度专题仍应直接使用公共头，学生练习实现不是它们的依赖。

前置正文是 [共享状态与锁](../../chapters/04-shared-state-and-locks.md)、[等待与通道](../../chapters/05-waiting-and-channels.md)、[取消与关闭](../../chapters/06-cancellation-and-shutdown.md)。本专题继续同一条 drain 契约；取消尚未执行任务、工作窃取和 sender 调度属于需要另外说明契约的后续分支。

## 1. 先明确 submit 在哪一步形成承诺

一个任务经历四个阶段：提交线程构造可调用对象及其参数；任务成功进入队列；某个 worker 把它从队列取出；worker 执行并让 future 保存结果或异常。

只要任务尚未成功入队，分配失败、构造失败或关闭拒收都可以让 submit 抛异常。成功入队后，它就属于 accepted 任务，池负责在正常 drain 关闭中把它执行一次。submit 可能尚未返回，worker 就已经完成了这个任务；返回 future 的函数耗时与任务执行区间可以重叠。

容量限制的是等待在队列中的任务数，不包括 worker 已经取出的任务。因此 N 个 worker、容量 K 的池，最多可有 K 个排队任务和 N 个执行中任务；同时等待 submit 的外部线程还各自持有准备好的任务对象与参数。这个池对排队数量有界，不等于对整个进程的任务内存有严格上界。若必须限制所有未完成请求，还需由上游限制并发提交者，这属于更外层的背压协议。

## 2. 公共 API 和不变量

```cpp
cs::thread_pool pool(3, 8);
auto answer = pool.submit([](int x) { return x * x; }, 7);
int value = answer.get();
pool.shutdown();
```

| 项目 | 教学实现选择 |
|---|---|
| 构造 | worker_count 与 capacity 都必须大于零，否则 invalid_argument |
| submit | 外部线程可并发调用；队列满则等待；关闭拒收时 runtime_error |
| 返回 | future<R>，R 由衰减后存储类型按右值调用推导 |
| 元素 | move_only_function<void()> 包装 packaged_task；支持 move-only 函数和参数 |
| 顺序 | 队列按成功入队位置 FIFO；不保证开始、结束或 future 就绪的全局顺序 |
| shutdown | 幂等，可由多个外部线程并发调用；关闭、广播、排空、join |
| worker 限制 | 本池 worker 对同池 submit/shutdown 立即抛 logic_error |
| 对象寿命 | 池不可拷贝、不可移动；所有 API 调用者必须在析构前结束访问 |
| 自销毁 | worker 销毁所属池属于违反寿命契约，析构 fail-fast terminate |
| 进展 | 阻塞实现；任务必须能结束，无无锁、无饥饿、实时期限承诺 |

队列的不变量由 bounded_channel 维护：容量不越界，关闭后不再接受，成功 pop 转移独占任务所有权。线程池追加两个不变量：每个 worker 从队列取出的 packaged_task 只执行一次；共享任务通道的寿命覆盖所有 worker 的访问。shutdown_mutex 只串行化 join，不用来保护任务执行或队列操作。

## 3. 为什么任务要先变成无参数 void()

不同任务可能返回 int、string、void，参数也不同。队列需要统一的存储类型，但结果通道不必丢掉类型。`packaged_task<R()>` 把“执行某个无参函数”和“保存一个 R 或异常”绑定在一起；worker 只需调用它，调用方持有 `future<R>`。

实现先把 f 和参数捕获到一个闭包，包装成 packaged_task，先取出 future，再把 packaged_task 移入 `move_only_function<void()>`。这里选择 C++23 的 move_only_function，是因为 packaged_task 不可复制，不必为了满足 std::function 的可复制要求额外套一层 shared_ptr。两种表示都可以正确实现，但本题默认工具链已是 C++23。

必须在任务移入队列前取得 future，否则 worker 可能已经执行并销毁队列中那个任务对象。future 的共享结果状态可以活得比任务、甚至比线程池更久；持有 future 不会反过来延长被 lambda 引用捕获的任意对象寿命。标准机制见 [`[futures.task]`](https://eel.is/c++draft/futures.task)、[`[futures.state]`](https://eel.is/c++draft/futures.state) 和 [`[func.wrap.move]`](https://eel.is/c++draft/func.wrap.move)。

用户函数抛异常时，packaged_task 将异常存入共享状态，future.get 重新抛出；worker 可以继续处理下一项。Reference 在异常任务之后再投递一个成功任务，检查 worker 没有因用户异常退出。不能只 catch 异常打印后继续，却让原 future 永远不就绪。

## 4. 类型推导必须与实际存储和调用一致

submit 的转发引用 F&&、A&& 描述调用表达式，不一定描述稍后执行时的对象类型。本实现把函数和参数按值保存，存储相当于 `decay_t<F>` 与 `decay_t<A>...`，执行时把它们当右值使用。因此签名采用：

```cpp
std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<A>...>>
```

实际调用对应 `std::invoke(std::move(fn), std::move(values)...)`。这个一致性不是形式细节。假设一个函数对象的 `operator()() &` 返回 int，而 `operator()() &&` 返回 string；调用者传入它的左值，线程池复制保存后按右值调用，应得到 future<string>。如果用原始 F 推导出 future<int>，接口声明与函数体就会冲突。runtime test 的 `ref_qualified` 同时做编译期类型检查和运行值检查。

默认按值存储也意味着一个接收 `int&` 的函数，不能直接靠传入左值 int 就期待线程池借用它；使用 `std::ref(value)` 明确借用。Reference 检查借用函数对象调用其 `&` 重载，以及 void 任务通过 ref 修改外部整数。`std::ref` 不管理寿命，调用方必须保证 get 或 shutdown 完成之前对象仍存在，且所有并发访问遵守同样同步协议。

对 unique_ptr 参数和移动捕获 lambda，参数被搬进任务，再在 worker 上消费，这就是 move-only 支持。成员函数指针通过 std::invoke 统一调用。若函数返回引用，future 可以承载引用，但调用方仍须保证被引用对象的寿命；尤其不要返回仅由任务闭包拥有、调用结束后即销毁的对象引用。本池不把引用自动变成拥有型快照。

## 5. worker 必须先出队解锁，再调用用户代码

每个 worker 循环只有两件事：`tasks_.pop()`，有任务就 `(*task)()`。pop 在内部锁下移动元素、释放槽位并通知生产者，返回时已经解锁。

若在队列锁内运行 task，长任务会阻止其他 worker 取任务，也阻止外部线程提交；任务再访问池或等待其他依赖，还会产生锁等待环。把执行留在锁外既是并发度要求，也是可组合的锁协议要求。它不会保证任务本身是线程安全的：两个任务若修改同一个引用捕获的普通对象，仍须自行同步。

worker 的入口还有 exception_ptr 记录，用于把用户 packaged_task 机制之外的基础设施错误交给 shutdown 报告。每个 worker 写自己的错误槽位，shutdown 在 join 后读取。正常用户异常不走这条分支。

这里的排空保证以基础设施正常运行为前提：若底层互斥/线程运行设施发生不可恢复错误，不能承诺所有 accepted 任务仍可执行。显式 shutdown 会在完成可行的 join 后重新抛出已记录错误；noexcept 析构遇到错误则 terminate。实际测试覆盖用户任务异常，没有注入操作系统 mutex 失败，不应把两类失败混为一谈。

## 6. shutdown 的线性化位置与幂等性

shutdown 首先禁止本池 worker 调用，然后调用任务通道 close。close 在队列锁内把 closed 置为 true，再广播 not_full 和 not_empty。这个锁内写入就是停止接受任务的边界。

与 close 并发的 submit，若已经完成入队，就是 accepted，必须继续排空；若尚在满队列等待，醒来后看到 closed，抛拒收异常；即使关闭后有剩余空间，也不能再入队。不能用函数调用的墙上时钟先后来判断竞态结果，必须看队列锁内的生效顺序。

worker 看到“关闭但仍有任务”继续 pop，直到“关闭且空”才退出。随后 shutdown 获取 shutdown_mutex，逐个 join 尚可 join 的 worker。其他 shutdown 调用者也会先 close，再在同一个 join 互斥量上等待；轮到它们时线程已不可 join，所以重复调用不重复 join。

只串行化 join 很关键。等待任务空间的 submit 不持有 shutdown_mutex；worker 执行任务也不持有它，所以关闭线程可以在生产者满等时发出广播。runtime test 把唯一 worker 卡在可控 gate、唯一队列槽位放入第二个任务，再发起第三个 submit。shutdown 后，第三个提交必须在 worker 放行之前拒收返回；之后再放行 worker，检查前两项都完成。

这个 gate 实验确立了“没有 worker/容量可推进第三个任务”的因果关系。它没有通过 sleep 断言提交线程已经在某个内部等待点停住；早进入 wait 和晚看到 closed 都是合法实现路径。唤醒活性仍需配合协议推导和外部超时，而不是从某一次调度推演所有未来历史。

## 7. 构造失败时谁关闭已经启动的 worker

构造 N 个线程可能在第 k 个失败。此时 `thread_pool` 对象构造没有完成，类自己的析构函数不会执行；已经构造的成员会逆序销毁。若只依赖 vector<jthread> 析构，它会请求 stop 并 join，但 worker 正在不带 stop_token 的通道等待上，可能永远不醒。

本实现捕获线程创建阶段的异常，先 tasks_.close，再 join 已经创建的 worker，最后重新抛出原异常。关闭空通道使 worker 的 pop 返回 nullopt，于是它们能结束。worker vector 在创建之前 reserve；队列存储和错误槽位也在启动线程前准备好。若这些准备分配失败，还没有任何 worker 需要回收。

成员声明顺序也参与寿命证明：任务通道在 worker 容器之前声明，逆序析构时 worker 先结束、通道后销毁。正常析构先调用 shutdown，提供同样的关闭与 join 协议。不能把共享条件变量放到比自动 join 成员更早析构的位置，再期望 RAII 自动修好顺序。

资源耗尽造成“第 k 个线程创建失败”的注入尚未在本机执行；代码路径已按上述规则实现，但不能用 worker_count 为零的输入检查冒充部分创建失败测试。可在独立故障注入环境拦截线程创建或施加资源限制验证，不能在普通全量课程运行中试图耗尽系统线程。

## 8. 有界队列与 nested 等待的死锁边界

最小死锁一：一个 worker 的池执行父任务，父任务提交子任务后等待其 future，子任务在队列里等唯一 worker，父任务占着它。最小死锁二：所有 worker 都在执行父任务，队列已满，各父任务都阻塞于 submit，没人能出队释放空间。即使不用 future.get，单是有界 submit 就足以形成循环。

本课程选择一个清晰且可检查的限制：同池 worker 不允许 submit，也不允许 shutdown。thread_local 记录当前 worker 所属池，在操作前抛 logic_error，runtime test 验证异常经任务 future 传回，而不是卡死。外部线程并发提交仍然支持。

这个限制不等于任意任务依赖都安全。外部线程可以把某个同池任务的 future 交给另一任务，后者等待它，仍可能造成资源饥饿；两个池的任务互相等待也可以形成跨池环。不能依靠“加几个 worker”证明死锁消失。需要把依赖表达成完成后续接，或者使用支持协作执行/结构化任务调度的专门实现，并重新定义背压与退出语义。

worker 调用自身 shutdown 会尝试 join 自己；更隐蔽的是外部线程已经拿着 join 锁并等待该 worker，而 worker 再进入 shutdown 等同一把锁。所以拒绝检查必须在任何 shutdown 锁之前。worker 自销毁更严重：抛异常或 detach 都不能让它稍后访问已销毁通道变安全。析构显式检测并 terminate，文档把它列为禁止的所有权用法，而不是声称池能自动收拾。

不要把池的最后一份 shared_ptr 所有权交给它自己的任务，并允许那个任务结束时触发池析构。由外部拥有者维持池寿命，先结束所有使用它的线程，再销毁。shutdown 与 submit 可以并发，并不授权析构与任意 API 调用并发。

## 9. 检查应验证责任，而不只验证一个求和

`results_and_types()` 检查 worker ID 不同于提交线程、move-only 参数与函数、衰减类型推导、ref 借用、成员调用、void future 和异常后继续执行。随后给 300 个任务分配互不重叠的 ID，分别计数执行次数并保留各自 future。shutdown 完成后，每项结果都须等于 i*i、每个 ID 的执行次数都须为一。仅比较总和会掩盖重复和遗漏互相抵消。

`close_full_pool()` 检查容量 1 下关闭穿透等待提交者，并排空已接受任务。`ownership_and_shutdown()` 检查零参数拒绝、同池 worker 禁止操作、两个外部线程并发 shutdown、空闲 worker 退出，以及 future 在池析构后仍可获取结果。所有异步测试异常经 future.get 传播；gate 在展开时也会释放，以便池能够排空。

```powershell
# 在 Concurrency_Study/exercises 下
cmake -S Capstone1_thread_pool -B build/pool -G "Visual Studio 18 2026" -A x64 -DBUILD_TESTING=ON -DCONCURRENCY_STUDY_TEST_STARTERS=OFF
cmake --build build/pool --config Release
ctest --test-dir build/pool -C Release -R '^Capstone1_thread_pool_reference$' --no-tests=error --output-on-failure

# 同一份检查也能由 runtime_tests 的公共接线注册
cmake -S runtime_tests -B build/runtime -G "Visual Studio 18 2026" -A x64
cmake --build build/runtime --config Release --target runtime_thread_pool_test
ctest --test-dir build/runtime -C Release -R '^runtime_thread_pool_test$' --output-on-failure
```

上述两种目标验证的都是公共答案，不能验证 main.cpp 的修改。Starter 为构造、worker_loop、submit、shutdown、析构留有逐 Part 提示和成员状态，运行 pool_checks::run<student_pool>() 才实际调用学生实现。完成七个 Part 并设置对应 part*_done 后，运行：

```powershell
cmake -S Capstone1_thread_pool -B build/student-Capstone1_thread_pool -G "Visual Studio 18 2026" -A x64 -DBUILD_TESTING=ON -DCONCURRENCY_STUDY_TEST_STARTERS=ON
cmake --build build/student-Capstone1_thread_pool --config Release --target Capstone1_thread_pool
ctest --test-dir build/student-Capstone1_thread_pool -C Release -R '^Capstone1_thread_pool_student$' --no-tests=error --output-on-failure
```

未完成时先输出 INCOMPLETE 并返回 1，不构造池、不启动 worker；student CTest 失败是预期，不加入默认 Reference 门禁。只改完成标记也不能通过：构造/submit 的 TODO 和类型参数化检查会报告失败，错误退出协议会被 30 秒超时限制。两种池共享契约检查，不共享实现。

不要求特定加速比；线程池管理、包装、分配和同步都有代价，极短任务甚至可能更慢。正确性检查与性能计时应分开，未来基准需要固定任务内容、数量、队列容量、线程数、计时是否包含创建/关闭，以及所有任务确实完成的证据。

## 自测与完整答案

**submit 返回前，任务能完成吗？** 能。任务入队之后 worker 就可以取走执行，submit 返回的是共享结果状态句柄，不是启动执行的第二次命令。

**FIFO 是否要求 future 按提交顺序就绪？** 不要求。先出队的 worker 可能调度较晚，任务本身也可能耗时更长。FIFO 只约束队列线性化的取出次序。

**任务异常会让 shutdown 抛同一个异常吗？** 用户任务异常保存在该任务 future；shutdown 正常 drain 不从所有 future 中收集用户异常。基础设施错误另经 worker 错误槽报告，两条通道不要混淆。

**为什么不用 stop_token 替代 closed？** token 是停止请求机制，closed 表示拒收及数据流结束；若收到请求就退出，accepted 任务可能遗漏。本实现用 close/drain 完整表达责任。

**close 之后再 get_future 可以吗？** get_future 应在包装任务入队前完成，与 close 无关。future.get 可以在 shutdown 后调用，因为结果共享状态仍由 future 持有。

**能不能在析构时把 worker detach 防止卡住？** 不能。worker 还在访问池成员，detach 会把可见的等待问题变成对象寿命错误；不能当作正确退出策略。

**shutdown 是否保证有限时间返回？** 以所有已接受任务能结束、依赖无死锁、调度与基础设施正常为前提。一个永不返回的用户任务会让 drain 永远等下去；本池没有强杀任意 C++ 线程的能力。

**如何增加 cancel-pending 模式？** 必须另定契约：关闭拒收、待执行任务的 future 获得何种终态、已经出队的任务是否协作取消、丢弃任务在哪个线程析构。销毁 packaged_task 可能形成 broken_promise，但这不等于设计了有意义的业务取消结果。当前公开 API 只提供 drain，未暗藏丢弃策略。
