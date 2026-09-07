# 02 结果通道：先有“结果状态”，再考虑线程

一个函数直接 `return 42` 时，结果沿调用链交给调用者。但如果准备阶段先交出一个“以后来取”的句柄，我们就需要把结果存放在独立于这次函数返回的位置。这里有三个自然问题：结果还没提供时怎样表示？计算失败时怎样表示？谁能取、能取几次？

`std::promise` 与 `std::future` 给这几个问题提供了标准库协议。先不把它们叫成“线程间传值工具”：它们可以完全在单线程中使用。[P2 Starter](../exercises/P2_result_states/main.cpp)展示十来行的基本流程，[P2 Reference](../exercises/P2_result_states/solution.cpp)完整检查状态转换。只有把这份协议学清楚，下一章把生产者换成 worker 时才不会同时猜测线程和结果两件事。

## 1. 先做一个不会阻塞的单线程实验

P2 的 `part1_states()` 创建 promise，从它取得 future，然后检查等待状态：

```cpp
std::promise<int> provider;
auto first = provider.get_future();
```

`provider` 是结果提供端，`first` 是结果读取端。两者关联到一个共享状态，状态里可以尚无结果，也可以有值或异常。这里的“共享”描述这些对象共同关联的状态，不意味着此时已经有两个线程。

此刻 `first.valid()` 为 true，因为它确实关联共享状态；`first.wait_for(0s)` 返回 `timeout`，因为还没有谁提供结果。Reference 把 future 移动到 `result`，检查源 future 无效、目的 future 有效，再由 `provider.set_value(42)` 完成状态。

随后 `wait_for(0s)` 返回 `ready`，重复 `wait()` 不消费结果，`get()` 得到 42 并使普通 future 无效。日志里的 `valid/pending -> ready -> get -> invalid` 是对象与状态的可观察演进，不是线程调度轨迹。基本规则见 [future 条款](https://eel.is/c++draft/futures.unique.future)，固定版为 [N5050](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf) `[futures.unique.future]`。

如果反过来，在唯一生产者还没有 `set_value` 前就在同一线程调用 `get()`，调用会等待共享状态就绪，但唯一能使它就绪的代码正在等待之后。这是协议死锁，不是 future 需要“多等一会儿”。本课程不把这个程序加入默认运行；Part 用零时长查询观察未就绪，之后先设值再取值。

## 2. 把句柄状态与结果状态分成两层

“future 有效”和“共享状态就绪”是两个维度。默认构造的 future 无效；promise 创建的 future 可以有效但尚未就绪；装有异常的状态也可以就绪。`valid()` 不告诉你成功与否，更不告诉你有没有后台线程。

| 观察点 | 句柄 valid | 共享状态 | 合适的下一步 |
|---|---|---|---|
| 默认 future | false | 无关联 | 赋入合法 future，或销毁 |
| get_future 之后、设值之前 | true | 未就绪 | 零时长查询；有执行者时可等待 |
| set_value 之后 | true | 就绪，持有值 | wait 或 get |
| set_exception 之后 | true | 就绪，持有异常 | wait 返回；get 重抛 |
| 普通 future.get 之后 | false | 此句柄已释放关联 | 不再用它等待或取值 |
| future.share 之后 | 原句柄 false | 由 shared_future 继续关联 | 用新的共享句柄观察 |

普通 `future<T>::get()` 将状态中的值用于构造返回结果，标准以 `std::move` 描述该返回；对于只移动类型，这允许把资源交给调用者。`future<T&>` 返回被存储的引用，`future<void>` 只表示完成，不返回对象。不能把三个版本都解释成“必然复制一个值”。本章可运行实例使用值类型和 void；引用结果仍要额外证明外部对象寿命，不把引用保存在状态里当成拥有它。

普通 future 的 get 即使重抛了存储异常，也会使句柄失效。不能在 catch 之后“再 get 一次确认”。对无效 future 再调用 get、wait 等不满足接口使用规则，不能把某个平台抛 `no_state` 当成可移植的必然结果。P2 只检查 `valid()==false`，不实际执行这类错误调用。

## 3. wait 只等，get 还负责交付结果

`wait()` 等待就绪，但保留当前句柄关联。`get()` 包含等待，并取得值或重抛状态中的异常；普通 future 随之失效。一个 future 可以先被多次 wait，再 get 一次。这适合把“等所有工作结束”与“逐项处理结果”分成两个阶段，但不要凭此假设任意多个线程可以并发操作同一个 future 对象。

`wait_for`/`wait_until` 是定时等待，返回 `ready`、`timeout` 或 `deferred`。其中 deferred 属于下一章的惰性 async 状态，不会由本章普通 promise 产生。`wait_for(0s)` 常用于立即查询，但它不是硬实时上界，也不是 lock-free 接口承诺。零时长查询得出 timeout 只说明这次观察尚未取得就绪状态，不能推出后台任务尚未开始。

P2 中设值和查询在同一线程有明确顺序，所以结果可以严格预测。跨线程时，若没有门闩，刚创建任务后查询可能看到 timeout，也可能看到 ready。第 03 章的 D2 会明确阻止任务完成，再检查 timeout，避免把正常调度差异当作失败。

## 4. 异常是另一种完成结果

P2 的 `part2_exception()` 在 try 中抛出业务异常，catch 中取得当前异常并保存到 promise：

```cpp
try { throw std::runtime_error("calculation failed"); }
catch (...) { provider.set_exception(std::current_exception()); }
```

这一操作把异常保存到通道，并不会在消费方立即执行 handler。等 `result.get()` 时，异常才在调用 get 的线程重新抛出。`result.wait()` 不负责重抛该存储异常，所以可以正常返回；Reference 依次检查“ready、wait 返回、get 重抛、future 无效”。

`set_exception` 自己仍可能因通道协议错误抛 `future_error`，例如状态已经满足。它不等于一个永远不失败的发送函数。线程版例子会将业务失败与结果通道操作失败分层处理：先尝试把业务异常写入通道；如果传送过程本身失败，再由单 worker 独占的 exception_ptr 槽回传，主线程 join 后检查。

`std::current_exception()` 取得当前正在处理的异常；在没有活动异常处理的正常路径上返回空指针。把调用写在 catch 内最清楚，但从 handler 调用的辅助函数也可能仍处于处理当前异常的动态范围。不能把“源码必须位于 catch 的大括号内”当成语法限制。也不能把空指针送给 `rethrow_exception` 或 `set_exception`；本课程先建立确实有异常的分支。

## 5. 一次性意味着值和异常合计完成一次

`set_value` 与 `set_exception` 写入的是同一个完成位置，并非两个相互独立的槽。已经写了值后不能再补一条异常；已经写了异常后也不能改写为成功。P2 `part3_provider_errors()` 检查重复设值抛出的错误码为 `promise_already_satisfied`，并确认第一次的结果仍为 1。

`get_future()` 也只能从同一个 provider 的相应状态取得一次。再次调用有规定的 `future_already_retrieved` 错误。这里特意对比两类误用：重复调用 provider 的这两个接口有指定异常；对无效 future 调 get 则不能依赖实现兜底。合法错误通道可以运行测试，未定义行为不能拿来作为可移植验收。

如果提供者还没完成结果就放弃状态，标准用 `broken_promise` 形成一个异常结果，使原本等待的消费者可以结束等待。P2 在内层作用域销毁未设值 promise，离开作用域后先检查状态 ready，再 get 并核验错误码。这条路径不会凭空保留业务异常；业务错误如果没有被 `set_exception` 保存，消费者看到的只是“结果提供责任被放弃”。规则见 [共享状态](https://eel.is/c++draft/futures.state)和固定版 `[futures.state]`。

“线程崩溃会自动收到 broken_promise”是危险概括。只有实际执行了相应放弃与析构流程才有这个结果。若异常逃逸 thread/jthread 入口导致 terminate，整个进程可能终止，不能依赖主线程继续 get。普通提前 return、正常作用域析构和进程异常终止必须区别对待。

另一个常见挂起来源是：worker 没完成结果就返回，但 promise 仍由等待它的主线程持有。主线程在 promise 析构之前先 get，于是放弃动作还没发生。把生产端按值移动给真正承担完成责任的人，可以让所有权与失败路径更一致；它不是对任意挂起程序的自动补救。

## 6. shared_future 共享的是结果状态，不是任意可变数据的安全性

如果三个消费者都需要同一份配置，不能让它们轮流消费同一个普通 future。`share()` 将关联交给可复制的 shared_future，原 future 失效；每个消费者可以持有自己的 shared_future 副本，并多次 get。

P2 的 `part4_shared_and_void()` 创建两个共享句柄，检查两次 get 指向同一个字符串结果。对普通值类型 `shared_future<T>::get()` 返回 `const T&`，不会为每个消费者自动复制一份 T。Reference 用 `static_assert` 固定返回类型，并显示如何显式复制为独立拥有的字符串。

这条引用只有在结果对象仍存活时才可使用。最后一个相关拥有者释放共享状态之后，先前存下的结果引用不能再用。若调用者需要脱离句柄寿命，就按类型能力复制结果或另行转移/共享资源。对于 `shared_future<T&>`，状态里只是外部对象引用；保留共享状态仍不延长外部对象寿命。依据见 [shared_future](https://eel.is/c++draft/futures.shared.future)，固定版 `[futures.shared.future]`。

更深一层，如果 T 是智能指针，const 引用并不必然使其指向对象不可变。多个消费者拿到同一个指针后并发修改对象，仍需要那个对象的同步协议。future 保障结果交付，不替所有后续访问兜底。D1 的广播只读取整数，每个消费者写自己的输出槽，主线程 join 后检查，刻意让这两种责任分开。

## 7. void 结果与跨线程发布

有时需要通知“这一步完成了”，不需要携带数值。`promise<void>::set_value()` 让 `future<void>` 就绪；`get()` 返回 void，仍会消费普通 future，也仍能重抛异常。P2 Part 4 检查这个完成事件，D1 Part 4 将其移到 worker。

结果通道的同步关系还能发布设值之前的普通数据。D1 的 `part1_and_2_transfer()` 让 worker 先写 `published_input=21`，再完成 promise；主线程 wait 成功返回后读取这份输入。该写之后没有后续修改，因此可以沿“写入→完成状态→成功等待→读取”证明访问有序。

但是这条边只覆盖相应完成之前的动作。worker 在设值之后仍可能执行清理、访问其他对象，普通 promise 的 ready 不等于整个线程结束。要读 worker 最后写下的手动错误槽，应先 join；要销毁仍被 worker 借用的数据，也必须等最后一次访问真正结束。下一章会把这两个等待边界同时画出来。

## 8. Part 与完整答案

运行方式见 [P2 README](../exercises/P2_result_states/README.md)。四个必做 Part 都在独立的 `solution.cpp` 中，Starter 通过不等于已经完成全部题目。

**Part 1：画句柄与结果状态。** 默认无关联；get_future 后有关联但未就绪；move 转移关联；set_value 使状态就绪；wait 保留关联；get 取得 42 并释放关联。不要在最终状态再次 get。

**Part 2：把值换成异常。** ready 仍为真；wait 不重抛存储异常；get 才重抛指定类型与消息，之后 valid 为 false。Reference 检查异常确实出现，不能用一个 catch 打印任意异常就算成功。

**Part 3：区分重复完成与放弃。** 重复 get_future、重复 set_value 分别有相应 future_error；销毁未完成 provider 产生 broken_promise，就绪不代表成功。三个情形都能在单线程有限运行，无需 sleep。

**Part 4：多个读取者与无值事件。** share 后原 future 无效；两个 shared_future 可反复读同一 const 字符串引用；独立保存需要显式复制。void 事件没有数值，但仍有一次完成和异常语义。

**自测：get 已返回，能立即析构 worker 引用的全部对象吗？** 对普通 promise 不一定。get 仅确认结果状态已经完成。还要检查 worker 是否继续使用它们；本课程通常以 join 或 jthread 作用域结束界定最后访问。

**自测：为什么不先写 mutex+ready+exception_ptr？** 如果需求恰好是一次性结果与异常交付，标准库已经提供完整状态协议。多条消息、队列关闭、取消、流式消费是不同需求，应进入对应章节，不能把一个 promise 反复设值冒充队列。

继续 [03 线程与执行方式](03-threads-and-execution.md)，把已经熟悉的结果通道连接到明确拥有的执行线程。
