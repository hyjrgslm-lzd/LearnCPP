# 06 取消与关闭：请求停止之后，谁还欠谁工作

“把线程停掉”听起来像一个动作，实际至少包含三个问题：是否还接受新工作，已经接受的工作如何处理，执行线程何时不再访问共享对象。把它们混成一个 stop 布尔值，往往会出现丢任务、漏唤醒或析构访问已释放资源的问题。

本章的 [A2 Reference](../exercises/A2_stop_token_cancellation/solution.cpp) 从 stop_source 的共享状态讲起，[C3 Reference](../exercises/C3_interruptible_wait/solution.cpp) 把取消接到真实等待点，最后与 [有界线程池专题](../topics/synchronization/01-bounded-thread-pool.md) 衔接。C++23 是基线；C++26 inplace stop 类型仅在 `CS_HAS_INPLACE_STOP_TOKEN` 探测成功时编译，规范固定为 [N5050](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf)，不能把滚动 eel 页面自动视作固定 C++26 文本。

## 1. 协作式取消首先是一条请求

`request_stop()` 把一个停止状态从“未请求”改成“已请求”。它不会强制跳出目标线程的循环，也不会替线程释放业务 mutex、关闭文件、撤回已经发生的外部操作。worker 必须在约定的检查点观察请求，完成必要清理后自行返回。

A2 `polling()` 的 worker 收到 jthread 注入的 token，在循环条件中检查 `stop_requested()`。主线程用 promise 确认 worker 已开始，再调用 request_stop，最后显式 join。worker 退出后记录确实观察到了 stop。验证依据是请求、观察、join 的协议，不是“打印了八行以后程序就退出”。这里 yield 只是轮询演示减少独占执行资源的提示，既不是同步原语，也不保证别的线程立刻运行。

轮询适用于能经常到达检查点的计算。如果一次不可中断 I/O 或第三方调用持续很久，token 不会神奇地缩短它。取消响应时间受到当前步骤、清理工作和调度共同限制；为每个潜在阻塞点设计退出方式，才是实际工程中的取消支持。

jthread 析构对可 join 的线程请求停止并 join，但目标线程未必观察它自己的 token。例如线程体捕获了另一个外部 source 的 token，析构所请求的不是那份状态；又例如 worker 卡在普通 condition_variable 的裸 wait，单纯 request_stop 不会为那个 wait 自动增加停止条件。RAII 能保障执行既定动作，不能替你补全业务协议。

## 2. source、token 和 callback 共享的是什么

普通 `std::stop_source` 拥有可请求停止的共享状态；复制 source 得到另一份控制同一状态的句柄，不是一个独立开关。`stop_token` 用于查询及关联回调，也参与共享状态寿命管理，但它自身不能发起停止请求。

第一次成功请求返回 true，之后重复请求返回 false。状态是单向的，不能 reset 回“未停止”；下一轮独立操作应创建新的 source。A2 `sources_and_callbacks()` 复制 source，经副本请求，检查原 source 与 token 都看到停止，再检查重复请求没有再次触发回调。

默认构造的 token 没有关联状态，`stop_possible()` 为 false。一个从未请求停止、且所有 source 都已销毁的状态，即使仍有 token 持有，也已不可能再被请求；A2 的 orphan 情形检查这一点。已经请求停止的状态则不会因 source 消失而变回 false。`stop_possible` 不是“现在应该退出”，`stop_requested` 才是停止请求的事实。规范入口为 [`[stoptoken]`](https://eel.is/c++draft/stoptoken) 与 [`[stopsource]`](https://eel.is/c++draft/stopsource)。

外部 source 可以把同一取消域传给多个普通线程、多个等待点或几个异步任务。共享的是取消信号，不是资源所有权；一个 token 不会延长捕获的业务对象寿命。不要因为每个线程都拿到了 token，就认为析构谁都安全。

## 3. 回调可能就在当前调用栈里执行

`stop_callback` 是一个 RAII 注册对象。它的寿命决定注册有效期，而不是仅靠 token 的寿命。如果构造时状态已经停止，回调会在构造它的线程上同步执行；否则它被登记到共享状态，成功请求停止的一方会同步执行已登记的回调。注册与请求竞态由库协议处理，不要求用户用“先检查再注册”来弥补。不要假定不同回调的调用顺序，也不要假定它们一定运行在 worker 线程上。详见 [`[stoptoken.concepts]`](https://eel.is/c++draft/stoptoken.concepts) 和 [`[stopcallback.cons]`](https://eel.is/c++draft/stopcallback.cons)。

A2 记录 request_stop 的调用线程 ID，在请求返回后检查回调已完成且线程 ID 相同；随后注册一个 late callback，紧接构造语句检查计数已增加。这比“看日志时间戳差不多”更直接，也解释了为什么下面的设计危险：持有业务锁 M 时 request_stop，而某个回调也要获取 M。同步调用就会等待自己尚未释放的锁。

回调对象析构也可能等待。如果回调正在另一个线程执行，注销必须等待那次调用结束，才允许销毁回调对象。若回调在当前线程执行，析构不能阻塞等待自身完成；这也不意味着回调可以继续访问已销毁的捕获成员。应该把回调设计为短小、无抛出、易于分析的通知操作，避免复杂自注销协议。

A2 的 `callback_destruction()` 用两个 promise 建立“回调已经进入”与“允许回调返回”的顺序。在另一线程注销回调后，检查它看到了 completed。测试不以短暂停顿证明析构已经阻塞在某条内部指令；标准条款给出等待保证，运行验证完成后的可见结果。

不要在持有回调需要的锁时销毁 callback，否则析构者等回调、回调等锁，形成等待环。回调抛出并离开调用边界会导致 terminate；Reference 因此不在 callback 内调用可能抛出的 cs::check，而是记录普通状态并在主线程检查。这个区别不是允许吞掉 worker 错误：普通任务异常仍要通过 future 或 exception_ptr 回传，只是 callback 的接口本身有不同约束。

## 4. 让等待点真正接受取消

普通 `condition_variable` 没有 stop_token 参数的等待重载。C++20 起，`condition_variable_any` 提供可中断等待：

```cpp
std::unique_lock lock(mutex_);
if (!cv_.wait(lock, token, [&] { return slot_.has_value(); }))
    return std::nullopt;
return std::exchange(slot_, std::nullopt);
```

这是 C3 `mailbox::take()` 的实际实现。等待期间，库把这个条件变量登记到 token 的停止请求通知机制中；stop 到来时等待者会被唤醒，返回前仍需重新取得用户锁并检查谓词。它不只是把 `stop_requested()` 放进一个 while：自行手写轮询条件而没有对应登记，仍会漏掉检查与等待之间的取消。

返回值告诉你的是最终谓词是否满足，不是“究竟由谁唤醒”。对于无期限重载，false 表示停止时谓词仍不满足；true 则表示本次检查条件满足，即便 token 也已经停止。因此 C3 选择“数据优先”：已经存入信箱的数据可以在停止状态下被取走。这个选择来自我们使用返回值的方式，而不是 stop_token 强制规定所有应用都采用排空策略。规范算法见 [`[thread.condvarany.intwait]`](https://eel.is/c++draft/thread.condvarany.intwait)。

若业务要“取消优先”，可以在持锁返回后检查 token，再决定放弃取值。但该检查也有线性化边界：刚检查为 false 后请求可能才到达；不能宣称从墙上时钟某一瞬间开始绝不再处理任何工作。还必须说明留在 slot 中的数据由谁收走或丢弃。

定时可中断 wait_until 的 false 可以由期限到达或取消造成。即使随后检查 token 为 true，也不总能重建哪个事件先导致唤醒；如果需要区分明确的业务终态，应在自己的状态机里持锁记录原因及优先级。C3 专门在没有停止请求、谓词 false、期限已到的情况下检查 false，并检查 source 未停止，证明“false 就是取消”这个解释不成立。

`condition_variable_any` 支持满足 BasicLockable 要求的锁类型，本例仍使用 `unique_lock<mutex>`，没有为了演示而引入自定义锁。无论何种锁，等到返回并持锁的那一刻，才有权依据谓词访问受保护对象。

## 5. 单槽信箱不能默默覆盖旧消息

C3 的 mailbox 是一个有意限定角色的例子：一个消费者，单槽存储，put 发现已有值就返回 false。它没有 close，也没有承诺多个消费者获得广播式的数据流结束；取消的是这次 take 的等待。若需要持续生产消费且支持背压，请直接使用上一章的 bounded_channel。

旧式演示常通过 `put(42); sleep; put(7);` 猜测第一个数据已被消费，但调度可能让第二次写覆盖第一条。C3 用 future.get 确认 42 已取到，再启动下一次空等待并 request_stop。随后预先取消空信箱，检查立即返回 nullopt；再存入 7，第二次 put(8) 应被拒绝，停止 token 下 take 仍取到 7。每一步都有明确协议和真实检查。

## 6. close、cancel、join 不能互相替代

上一章的 close 是全局数据流终点：不再接受新元素，已接受元素继续允许 pop，直到关闭且空时返回 nullopt。它不要求某个消费者尽快放弃，也不主动 join。C3 用同一个共享 bounded_channel 检查 close 后仍返回 1、2，再返回 nullopt，避免另写一个名称相似、契约却不一致的队列。

取消是调用者说“我不想继续这项操作”。它可以只取消一个等待者，让其他消费者继续 drain；也可以成为更大的关闭协议的输入。若策略是丢弃待执行任务，就必须定义这些任务的 future 如何结束、已开始任务是否继续、资源析构在哪个线程执行，以及多个取消请求如何去重。

线程池的 graceful shutdown 在本课程中选择 drain：先关闭任务通道并唤醒所有阻塞提交者及 worker，worker 继续取出 accepted 任务，在队列关闭且空时退出，最后 join。这里不靠 jthread 的 token 提前跳出取任务循环，因为那可能丢弃已经承诺执行的队列项。完整代码和边界见 [线程池专题](../topics/synchronization/01-bounded-thread-pool.md)。

把“停止请求发生”当作“共享对象现在可以销毁”是最危险的混淆。仍有线程执行回调、重新取得锁、从 wait 返回、向 future 写结果。拥有者必须明确等待这些访问者结束；对池来说是 shutdown 完成并确保外部 API 调用者也已返回，对通道来说是由外部 join 全部参与线程。

## 7. C++26 inplace stop：省去共享所有权，也把寿命责任交给你

N5050 的 `inplace_stop_source` 把停止状态放在 source 对象内部，不能拷贝或移动；token 关联那个 source，不能像普通共享状态 token 一样延长它的寿命。相关 worker、回调和查询必须在 source 仍有效时完成。尤其 `stop_requested()` 的调用需要满足与关联 source 析构开始之间的寿命前提，不能留下一个 token 后在 source 销毁以后继续查询。见 N5050 的 `[stoptoken.inplace]`、`[stopsource.inplace]`、`[stopcallback.inplace]`。

A2 的可选段把 source 声明在 token 和 callback 之前，按逆序析构先注销 callback，再离开 source 的作用域。它只验证 source/token/callback 的基本关系，不把 inplace token 强行传给仍以普通 stop_token 为签名的 condition_variable_any 重载。标准版本名称和库实现能力是两件事，必须以实际能力探测决定是否编译。

```cpp
#if CS_HAS_INPLACE_STOP_TOKEN
std::inplace_stop_source source;
auto token = source.get_token();
// callback 与所有使用者必须先于 source 结束。
#endif
```

本机默认 C++23 且该宏为 0 时，程序打印专项 SKIP，但仍执行并检查所有普通 stop_token 基线，不让整个 A2 冒充通过 C++26 检查。不要手动把宏改成 1 代替编译器/标准库能力验证。

## 实验、答案与检查边界

```powershell
# 从 Concurrency_Study/exercises 运行
cmake -S A2_stop_token_cancellation -B build/a2 -G "Visual Studio 18 2026" -A x64 -DBUILD_TESTING=ON -DCONCURRENCY_STUDY_TEST_STARTERS=OFF
cmake --build build/a2 --config Release
ctest --test-dir build/a2 -C Release -R '^A2_stop_token_cancellation_reference$' --no-tests=error --output-on-failure
```

上面运行完整答案。A2 main.cpp 独立暴露 student_poll、student_copy_source、student_register、student_unregister 及能力宏内的 student_inplace；C3 暴露 student_mailbox 与 student_expired_wait。它们都有调用学生函数的契约检查。完成各 TODO 后设置对应 part*_done，再运行学生目标，不能只跑 Reference：

```powershell
cmake -S A2_stop_token_cancellation -B build/student-A2_stop_token_cancellation -G "Visual Studio 18 2026" -A x64 -DBUILD_TESTING=ON -DCONCURRENCY_STUDY_TEST_STARTERS=ON
cmake --build build/student-A2_stop_token_cancellation --config Release --target A2_stop_token_cancellation
ctest --test-dir build/student-A2_stop_token_cancellation -C Release -R '^A2_stop_token_cancellation_student$' --no-tests=error --output-on-failure
```

未完成 Starter 在创建任何线程前返回 1，此学生测试失败是预期；不进入默认 Reference 门禁。能力宏为 0 时 A2 不要求 inplace Part 的完成标记。把题名替换为 C3_interruptible_wait 可检查信箱与取消等待，每题 README 也提供完整命令。worker 错误通过 future 或 exception_ptr 回传；join 后才读取非原子记录。错误等待由 CTest 30 秒超时限制，不以 sleep 排序。

**source 复制后谁的 request_stop 才有效？** 两者都控制同一个状态，先成功改变状态的那次返回 true。不同 source 对象也可能共享同一取消域。

**request_stop 返回 false 是否说明没有取消？** 不说明，最常见的是之前已请求过；无状态 source 也可能返回 false。应检查协议含义，不能把返回值当作目标线程存活状态。

**注册回调之前是否必须先检查 stop_requested？** 不必。库注册会处理已停止状态；自己先检查一次也无法消除检查后才取消的竞态。

**callback 析构为什么会死锁？** 析构可以等另一线程中该回调结束。若析构者持有回调所需资源，就形成等待环；回调析构不是无条件的常数时间操作。

**所有 worker 都被 request_stop 后，accepted 任务会怎样？** 取决于 worker 循环与队列协议。stop_token 不保存任务义务，必须另行定义 drain 或 cancel。本课程线程池只承诺 drain。

**数据与 stop 同时可见时，C3 返回什么？** 返回数据，因为 wait 的最终谓词为真；取完后下一次在空信箱上等待，已停止 token 会让它返回 nullopt。

**没有 sanitizer 报告，能否证明回调寿命正确？** 不能。检查支持已运行的交错，寿命正确性仍依赖谁持有对象、谁等待谁以及析构次序。C++26 未编译分支与部分资源故障注入必须明确标注未验证。
