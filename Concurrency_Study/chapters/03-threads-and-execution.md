# 03 线程与执行方式：把完成责任交给明确的拥有者

第 02 章把结果表示成共享状态，现在只改变一个条件：由另一个线程提供结果。不要同时放弃对象图。创建者要决定输入如何交给 worker，谁拥有线程关联，失败怎样返回，以及销毁输入和输出之前等到哪个边界。

本章由四段递进组成：先管理线程，再搬运异常，再选择 async 策略，最后将任务打包并交给指定执行者。代码分别在 [A1](../exercises/A1_jthread_lifecycle/solution.cpp)、[A3](../exercises/A3_thread_exception/solution.cpp)、[D1](../exercises/D1_promise_future/solution.cpp)、[D2](../exercises/D2_async_policies/solution.cpp)、[D3](../exercises/D3_packaged_task/solution.cpp)。每题都有安全 Starter 和独立 Reference，末尾的 Part 表对应真实函数。默认 C++23；线程和结果基础来自 C++11，jthread/stop_token 来自 C++20。

## 1. 线程函数结束，线程对象还可能 joinable

`std::thread worker(f);` 创建关联一个执行线程的对象。这个对象不是那个线程本身，也不是函数结果。worker 函数可能很快返回，但在拥有者 join 或 detach 之前，线程对象仍然可能 `joinable()`。

A1 `part1_thread_join()` 让 worker 算出 42，再在主线程调用 join。join 成功返回说明关联线程已经结束，并解除关联，所以 `joinable()` 为 false。重要的是它还建立同步关系：线程完成同步于相应 join 的成功返回；worker 的写可以沿此边发布给主线程后面的读。依据见 [thread 成员](https://eel.is/c++draft/thread.thread.member)，固定版为 [N5050](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf) `[thread.thread.member]`。

若 `std::thread` 析构时仍 joinable，标准要求调用 terminate。不能用“它应该已经做完了”替代 join。detach 会解除对象关联，却不等待线程结束；它也不延长线程借用的局部对象寿命。需要脱离创建者寿命的线程时，必须另外设计所有权、完成通知和进程收尾协议；本章不把 detach 作为省去清理的捷径。

第二次显式调用 join 不是无操作，而是错误。A1 在第一次 join 后再次调用并检查 `system_error`。这与 jthread 在已经 join 之后安全析构完全不同：析构会检查当前是否 joinable，已无关联便不再 join。不要把两个情形缩成“join 两次安全”。也不能在多个控制线程上同时 join 同一个线程对象；`joinable()` 检查本身不提供针对该对象的外部互斥。

## 2. jthread 用作用域管理关联，但退出仍需要协议

A1 `part2_scope_and_move()` 创建 jthread，然后把它移动到另一个 jthread。源对象不再关联线程，目的对象承担清理责任。即使函数体已经运行结束，目的对象在 join 前仍有关联，因此这里的 joinable 检查不是“线程是否仍在工作”的测试。

当一个仍 joinable 的 jthread 析构时，它先请求停止，再 join。A1 通过作用域结束后的结果检查验证自动 join；`part3_automatic_stop()` 则让 worker 查询传入的 stop_token，观察停止请求后返回，随后主线程检查保存的 token 与结果标记。

本实验无需 sleep，也不要求 worker 先循环几次。主线程直接到达析构边界，停止请求可能发生在 worker 第一次检查之前，也可能之后，两种交错都合法。循环中的 yield 只是调度提示；退出依据是 stop 状态，结果读取依据是析构内的 join。相关构造与析构规定见 [jthread 构造/析构](https://eel.is/c++draft/thread.jthread.cons)，固定版 `[thread.jthread.cons]`。

“请求停止”不是强制中断。若 worker 做有限工作但完全忽略 token，析构仍可等它自然完成；若 worker 永远不结束，join 也无法结束；若 worker 在一个不支持 stop 的等待上睡着，仅设置 stop 状态不一定能唤醒它。请求与响应之间的检查点、回调线程、可取消等待及资源关闭，继续读 [A2 取消练习](../exercises/A2_stop_token_cancellation/README.md)。本批次不修改 A2。

RAII 还要求声明顺序合理：被 worker 引用的结果和错误槽要比 worker 的收尾活得更久。如果 worker 在等待某个 promise 完成，而 promise 只有在主线程越过 join 后才会析构，自动 join 也救不了这个循环依赖。D1/D2 特别安排释放事件拥有者与线程句柄的析构顺序，使构造中途失败时仍能解除等待。

## 3. 参数在调用方准备，函数体在执行线程调用

把一个普通左值作为 thread/jthread 参数，并不会自动把它变成共享引用。线程启动接口会保存调用对象和参数的衰减后值；C++23 的标准表述使用 `auto(...)` 物化这些值，通常仍用“decay 后保存”解释其效果。引用与顶层 cv 等处理应按实际类型规则判断。构造保存值时的异常发生在调用方；真正执行入口函数时的异常发生在执行线程。依据见 [thread 构造](https://eel.is/c++draft/thread.thread.constr)。

A1 `part4_arguments()` 做三个安全对照：

1. 将整数 10 按值传入，worker 递增参数得到 11，原整数仍为 10。
2. 用 `std::ref(original)` 传给接收 `int&` 的入口，worker 将原整数改为 11；主线程在作用域自动 join 后才读。
3. 用初始化捕获把 unique_ptr 移进闭包，worker 独占输入资源，主线程在 join 后核验结果及原指针为空。

三个版本改变的是所有权契约，不是“哪种写法更先进”。输入小且独立时复制很清楚；需要独占资源时移动；需要共同访问时借用，但必须同时处理寿命与同步。`std::invoke` 描述调用规则，不会为借用对象加锁。

jthread 构造还会根据可调用性选择是否在参数前注入其 stop_token。若能以 token 加保存参数调用，就选择相应形式，否则尝试无 token 形式。普通 lambda 把 stop_token 写为第一个形参是最清楚的教学写法；不是运行时根据参数名字识别 token。

## 4. 异常需要跨越线程边界的明确通道

主线程的 try/catch 包围 thread 构造，只能处理调用方这条调用链上的异常，例如参数准备或线程创建失败。worker 入口内部抛出的异常不会沿创建者的栈返回；若逃逸入口顶层，会调用 terminate。

A3 先使用手动通道：

```cpp
std::exception_ptr captured;
std::jthread worker([&] {
    try { /* 工作；任何业务检查都放在这里 */ }
    catch (...) { captured = std::current_exception(); }
});
worker.join();
if (captured) std::rethrow_exception(captured);
```

这段是 [A3 `part1_manual`](../exercises/A3_thread_exception/solution.cpp) 的结构摘录。真正的代码检查正常结果和指定业务异常类型。只有一个 worker 写 `captured`，主线程第一次读发生在 join 后，因此不用再加一把 mutex。若改成多个 worker 共同赋值同一槽，或者主线程在 join 前轮询它，这份证明便不成立。可以给每个 worker 一个独立槽并在全部 join 后读，或用单独同步保护共享槽。

exception_ptr 保留异常供稍后重抛，不等于普通共享变量可以无同步并发读写。`rethrow_exception` 还要求确实有异常，因此先检查非空。为了防止“打印到任意异常都算成功”，Reference 区分指定业务异常和 `cs::check` 自身可能抛出的失败；意外检查失败必须继续失败。

A3 的第二条通道把 promise 移给 worker：正常路径 `set_value(42)`，异常路径 `set_exception(current_exception())`，主线程 get 在取值处接收结果或重抛。传输机制失败另存到 `transport_error`，主线程必须 join 后才读它。这里 join 的目的除了收尾，还确保手动错误槽完成写入；它不能被一句“future 应该已经 ready”替代。

## 5. 从一次结果扩展到多个消费者

D1 `part1_and_2_transfer()` 专门区分两个边界：结果 wait 成功之后，主线程可以读取设值前完成且之后不再修改的 `published_input`；join 之后，主线程才读取 worker 的最终 `transport_error`。前一条边来自结果状态，后一条边来自线程完成。代码将它们写成两个不同位置，使“结果完成”和“线程完成”不再混淆。

广播实验给三个消费者分别复制 shared_future。每个消费者读取同一整数 100，并写自己的数组元素；每个消费者也有独立 exception_ptr 槽。主线程发布结果后 join 全部消费者，逐项验证输出并重抛其检查失败。这里不是三个线程抢同一个普通 future，也不是一个原子计数恰好等于三就假定所有值正确。

线程批量创建还可能中途失败。D1 将消费者容器声明在 promise 之前：异常展开时，未满足的 promise 先销毁，使等待者得到 broken_promise；随后 jthread 容器清理并 join。若把顺序颠倒，容器可能先 join 仍在等待 promise 的消费者，从而挡住 promise 析构。异常路径上的声明顺序也要画进对象图。

## 6. async 的策略决定执行方式，不承诺立即获得 CPU

`std::async` 同时准备结果状态和安排调用方式。实践中显式选择策略，才能知道在哪个执行上下文推进：

| 策略 | 执行方式 | 什么触发调用 | 本课程如何观察 |
|---|---|---|---|
| `launch::async` | 如同在新的执行线程运行 | 发起调用安排执行，不依赖随后 get | worker 发出 entered 事件，主线程先等待该事件 |
| `launch::deferred` | 在首次非定时等待的线程运行 | 对关联结果对象调用 wait/get 等非定时等待 | 定时查询为 deferred，wait 后调用计数为 1 |
| 省略策略 | 标准默认允许 async/deferred，另需注意实现允许的扩展 | 由所选策略决定 | 只记录本次状态和线程 ID，不断言固定选择 |

“不依赖 get 启动”不等于“函数体在 async 返回前已经执行到第一行”。线程调度没有这样的即时性保证。D2 `part1_policies()` 让 async 任务先满足 entered promise，再等 release 事件。主线程在对后台结果 get 之前收到 entered，因而有证据说本次任务已经执行；release 尚未发生，所以结果查询必须 timeout。这个协议同时把开始与完成分开，无需睡眠猜时序。

对于 deferred，定时等待不触发执行。首次 `wait()` 会在调用线程执行函数，完成后保留 future；之后 get 只交付结果，不重复调用。若把句柄交给另一个线程，并由那个线程首次非定时等待，执行者就变成那个线程，而不是固定为最初创建 future 的主线程。

默认策略不能用于“我发起后只轮询一项由任务写入的标志”的协议，因为实现可能选择 deferred，而该协议始终没执行首次非定时等待。显式 async 也不保证创建成功：资源不足可能使调用方收到异常，不能依赖库偷偷退回 deferred。固定版见 N5050 `[futures.async]`，便捷导航为 [async 条款](https://eel.is/c++draft/futures.async)。

## 7. 最后一个共享状态引用释放，可能成为等待边界

先回忆 P2：普通 future 析构释放的是它对共享状态的关联。不能把所有 future 析构都当成 join。对于普通 promise 或 packaged_task 产生的状态，结果句柄的释放不会为了等状态就绪而阻塞；被管理结果对象自己的析构仍可能有其成本或行为，这与 future 是否等待生产者是两个问题。

async 选择异步策略时，其共享状态还关联执行线程。N5050 `[futures.state]` 说明释放状态的等待例外；`[futures.async]` 进一步规定关联线程完成，与首次成功检测就绪的函数返回或最后释放状态的函数返回之间的同步关系。实用分析方式是：状态是否来自实际采用 async 策略的调用？线程完成是否已经被相应等待观察？当前释放是否最后一个关联？不能只看变量的类型拼写是 future 还是 shared_future。

D2 `part3_last_reference()` 把 async 结果转成 shared_future，再复制一份。任务等待一个门闩。先把第一份赋为空，因为还有另一份关联，这一步不会为任务就绪等待；主线程因此能继续打开门闩。最后把剩余句柄也赋为空，之后读取任务写的普通整数，检查它已为 1。这里展示的是最后释放的完成边界；任务可能在最后赋值开始前就已经完成，实验不要求测出某个正的阻塞时长。

还要注意阻塞点不限于析构：赋值覆盖旧 future/shared_future 也会释放旧关联。把句柄放进容器、移到另一个作用域或复制为 shared_future，都可能改变哪个位置释放最后引用。若在持有 worker 需要的 mutex 时释放最后引用，可能产生等待环，后续锁章节会继续讨论。

仅有 deferred 工作且从未触发等待时，丢弃句柄不会替你执行函数。D2 检查 deferred 调用计数始终为零；还创建普通 promise 的 future，在设值前销毁句柄，再设值，说明普通状态不会在这个析构点等待生产者。

## 8. 为什么临时 future 能把任务串起来

D2 `part2_temporary_and_retained()` 循环执行：

```cpp
(void)std::async(std::launch::async, [&completed] { ++completed; });
```

这个临时结果在完整表达式结束时释放最后关联，于是下一次迭代之前，前一个关联线程已完成。Reference 在每轮之后检查普通整数 completed 的确定值。它是定义良好的语义演示，不是 UB；但它没有保存异常供 get 检查，因此此处任务体只做有界整数递增。正常业务任务应保留句柄并检查异常，析构不会替你处理业务错误。

另一版本将三个 future 存进 vector，每个任务的完成路径都必须经过尚未释放的 gate。主线程完成三次发起后检查所有结果仍未完成，再打开事件并逐项 get。这个版本没有逐任务 entered 握手，因此 timeout 不能证明某个任务已开始或已到达 gate.wait；它证明的是三个已提交任务尚未完成。保留句柄允许多个未完成任务重叠存在，不同硬件、工作量和调度可以得到不同耗时；不要求一定并行执行或一定更快。

异常展开也必须有限收尾。保存 futures 的容器比 release promise 先声明，这样异常时 release 先销毁，gate 的 `wait()` 会因状态成为 ready 而返回，任务随后结束，最后容器释放 async 状态才完成等待。gate 在这里只等待、不 get，因此 broken_promise 也可以作为展开时的放行事件；不要把这个实验门闩直接当成业务结果。

## 9. packaged_task 只打包调用与结果，不选择调度器

D3 `part1_direct_and_thread()` 先构造 packaged_task，查询结果仍 timeout，然后直接调用它，结果线程 ID 等于当前线程。第二次把新任务移动给显式 jthread 调用，结果线程 ID 与主线程不同。差别来自谁调用 `task()`，不是 packaged_task 自己开了线程。

被包装函数正常返回时，结果进入共享状态；被包装函数抛异常时，异常进入共享状态，随后从对应 future.get 重抛。就绪发生在调用产生结果以后，而不是 `task()` 刚开始那一刻。重复调用已经满足的任务、调用无状态任务等协议错误，还可能从 packaged_task 的调用操作本身抛出，需要 worker 顶层的异常兜底。依据见 [packaged_task 成员](https://eel.is/c++draft/futures.task.members)。

不同返回类型的任务怎样放在同一队列？D3 使用真实的只移动封装：

```cpp
auto invoke_task = [t = std::move(task)]() mutable { t(); };
std::packaged_task<void()> envelope(std::move(invoke_task));
```

内层 task 保留自己的 `future<int>` 或 `future<string>`，外层统一为 void()，供队列保存。拥有内层 task 的闭包不能复制，因此不能直接塞进 `std::function<void()>`。C++23 的 `std::move_only_function<void()>` 也能保存只移动闭包，但它只提供调用包装，不自动提供结果通道；本题选 packaged_task<void()> 并把外层 future 也保存下来，以免外层封装截获调用协议错误后无人观察。

这形成两层明确检查：内层 future 处理业务结果/业务异常；外层 future 处理封装调用是否成功；最外侧 worker 的 exception_ptr 处理队列循环本身的异常。它们不能统统 catch 后打印一行“任务结束”就忽略。

## 10. D3 的队列契约：这是关闭批次，不是在线线程池

本题先由主线程提交七个任务，再把访问阶段交给唯一 worker，最后 join。它刻意选择一个更容易验证的场景，和运行中仍允许生产者提交的阻塞队列不是相同契约。

| 项目 | D3 当前契约 |
|---|---|
| 容量 | 动态存储，无固定容量上限，实际受分配限制 |
| 提交 | 仅在 worker 创建前；成功 push 才算接受，分配失败抛异常 |
| 关闭 | 启动 worker 就进入不可再提交阶段，结构上封闭批次，无并发 close API |
| 消费 | 唯一 worker FIFO 取出并逐个调用，队列空即退出 |
| 所有权 | queue 独占每个外层 packaged_task，worker 移出取得任务 |
| 完成 | 六个 int 任务（其中 ID 3 业务失败）加一个 string 任务；逐项检查 |
| 主线程观察 | worker join 后才读 queue、完成计数和错误槽 |
| 进展 | 有限任务体下完成；创建、分配、调用、join 都不宣称无锁或固定时间 |

主线程在 worker 执行期间不碰 queue，因此不需要给 std::queue 添锁。原来 MiniPool 的教学方向由后续线程池项目承接，本题不保留一个未覆盖完整关闭契约的第二份池实现。

为了保留旧题“运行期间投递、关闭后排空”的必做内容，D3 还提供 `part5_online_queue()`，这是明确的场景切换。它先启动一个 worker，再允许主线程 enqueue。在线协议允许 queue 的访问阶段重叠，不能沿用前面仅靠分阶段所有权的证明，所以增加一把 mutex 保护 queue 与 closed。生产者把任务移入队列后解锁并通知；worker 持锁检查 `closed || !queue.empty()`，谓词不成立时由条件变量等待，成立后重新持锁访问队列。移出一个任务就释放锁，再调用任务，避免把用户函数放在队列临界区。

这个版本中，队列为空但尚未关闭不代表结束，因为以后还可能提交；只有关闭且为空时才退出。close 在锁内设置 closed，随后通知等待者；关闭时已有任务仍按 FIFO 排空。关闭后 enqueue 返回 false，未接受任务的参数对象销毁；若它关联读端，会按未执行 provider 放弃的规则产生 broken_promise。插入的存储分配失败则抛异常，不能算成成功接受。

Reference 为关闭测试固定一个交错：worker 发 entered 后，必须经过 gate 才能取任务；主线程收到握手，提交四项并 close，持锁核验队列仍有四项、相应 futures 都未就绪，再释放 gate。因此确定测到了“关闭时有积压”，不是四项碰巧在 close 前都执行完。每个任务进入函数体时记录实际 ID，join 后检查恰为 `0,1,2,3`，再验证 0、10、20 与 ID 3 的指定业务异常。按 future 顺序取值本身不能证明 FIFO。另一个零任务输入检查空关闭能结束，拒收任务则保留 future 并核验 broken_promise。

第三个输入在放行前主动抛出 submission_failure，验证错误路径仍能排空。catch 保存生产者异常并 close；release promise 先离开内层作用域，将未满足 gate 变为带 broken_promise 的 ready 状态，然后才 join。worker 使用 gate.wait，放弃也会解除等待；若在 release 仍存活时先 join，就可能死锁。完成计数、实际执行记录和 worker 异常槽都在 join 后读；预期注入异常单独核验，其他异常继续重抛。这个无固定容量、单生产者/单消费者版本保留阻塞语义，不提供取消或固定延迟；本次固定交错支持关闭契约检查，不代表已经穷尽所有在线历史。

最后，D3 `part4_one_shot_and_abandon()` 验证重复调用抛规定错误，以及未调用的任务被销毁后，其结果 future 得到 broken_promise。想再次使用任务可以研究 reset 创建新共享状态，但旧结果通道不会因此变成可重复设值的消息队列；本题不需要 reset。

## Part 与自测解析

| Part | 对应完整 Reference | 验收与答案 |
|---|---|---|
| 生命周期、移动与参数 | A1 四个 `part` | join 发布写入；移动转交关联；按值/引用/独占输入有不同寿命责任 |
| 手动异常通道 | A3 `part1_manual` | 单 worker 写、join 后读；正常结果和业务异常都检查 |
| 结果通道与广播 | A3 Part 2/3，D1 全部 | 值/异常各完成一次；各消费者独立句柄与输出槽；放弃产生 broken_promise |
| 策略与执行时机 | D2 `part1_policies` | entered 证明开始，release 控制完成；deferred 的非定时等待才执行 |
| 结果句柄寿命 | D2 Part 2/3/4 | 保留 future 后移完成边界；最后共享关联决定等待位置；get 检查异常 |
| 打包、交接与队列 | D3 全部 | 打包不执行；指定调用者；异构任务通过 void() 封装；逐项收集结果 |

运行命令与每个 Part 的操作步骤见各题 README：[A1](../exercises/A1_jthread_lifecycle/README.md)、[A3](../exercises/A3_thread_exception/README.md)、[D1](../exercises/D1_promise_future/README.md)、[D2](../exercises/D2_async_policies/README.md)、[D3](../exercises/D3_packaged_task/README.md)。

**async 是 promise 加 thread 的标准实现吗？** 可以用它们建立直觉类比，但标准规定的是可观察语义，不强制实现内部必须实例化某个 promise 对象。不要把接口类比写成源码事实。

**get_future 必须在 move 之前调用吗？** 本题选择先取得句柄再提交，避免所有权交出后无法访问 task。移动后的目的 task 若仍由你独占管理，也能取得其 future；“先取后投”是本课程的所有权协议，不是标准禁止所有相反顺序。

**七个 future 按提交顺序 get，是否能推出多 worker 也按该顺序执行？** 不能。结果槽身份、执行顺序、完成顺序、取值顺序是四件事。本题单 worker FIFO 才提供当前调用顺序；换成多 worker 必须重新声明契约。

**为什么不要求输出先打印 worker 再打印 main？** 只有同步规定的事件关系才必须遵守。主线程“已经创建对象”的输出和 worker 的第一行之间通常没有我们想当然的先后；验证用结果、门闩和 join，日志只是解释检查后的事实。

本章规范索引使用 N5050 的稳定条款名；eel 页面是滚动导航，可能已包含 C++29 修改。运行通过只说明本机实现与本次检查一致，不代表已经测过全部标准库，也不代表经过独立作者审查。
