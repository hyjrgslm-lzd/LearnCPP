# 05 等待与有界通道：让状态决定何时前进

上一章的 mutex 能让一次队列操作保持完整，却不能告诉消费者“什么时候有下一条数据”。在 [Q0 基线](../topics/queues/01-mutex-baseline.md) 中，空队列返回 false，调用者可以选择重试。若任务很稀疏，反复拿锁检查会制造大量无用工作；如果先睡一段时间再检查，又增加了响应延迟，而且 sleep 不建立任何同步关系。

本章先用 [C1 Reference](../exercises/C1_condvar_predicate/solution.cpp) 解释等待协议，再推导 [共享 `bounded_channel.hpp`](../exercises/include/concurrency_study/bounded_channel.hpp)，最后用 [C2 Reference](../exercises/C2_bounded_queue_condvar/solution.cpp) 检查 FIFO、容量、关闭和多生产者多消费者。相对 Q0，这是一条明确的契约演进：从“满/空立即失败”切换为“满/空等待”，并增加数据流结束状态。不能把两者未经说明地当成同契约性能排名。

## 1. 为什么“检查后睡觉”会错过事件

假设消费者在锁内看到 `ready == false`，解锁后准备睡觉。生产者恰好在这个空隙把 ready 设为 true 并发送通知。等消费者真正开始睡时，通知已经过去；如果没有下一次通知，它可能一直不再检查状态。

这里不是“通知速度太快”，而是协议留下了一个没有登记等待者的间隙。条件变量的 wait 把释放 mutex 和进入等待联系为一个原子部分；唤醒后还会重新获取锁，再把控制权交回调用者。同一条件变量的等待各部分与通知有标准规定的总序，但通知不会变成一个可供未来调用消费的持久令牌。详见 [`[thread.condition]`](https://eel.is/c++draft/thread.condition) 和 [`[thread.condition.condvar]`](https://eel.is/c++draft/thread.condition.condvar)，规范版本以 [N5050](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/n5050.pdf) 为准。

```cpp
std::unique_lock lock(mutex);
cv.wait(lock, [&] { return ready; });
// 此时持锁，且本次谓词检查得到 true。
```

这个重载的核心是 `while (!ready) cv.wait(lock);`。它先检查，再决定是否等待；被唤醒并重新持锁后，再检查一次。生产者按相同 mutex 修改 ready，然后通知。若 ready 早已变成 true，消费者根本不必进入等待，所以即使“通知早于 wait”也不会丢掉业务状态。

仅仅让 ready 成为 atomic 不能随意去掉这个配套协议。如果生产者绕过 mutex 更新，消费者仍可能在检查 false 与实际等待之间错过通知；“变量没有数据竞争”不等于“等待不会永远挂起”。本章统一由同一把锁保护谓词的检查和修改，避免把两个独立协议拼在一起。

## 2. 醒了为什么还要检查

第一种原因是虚假唤醒：没有一次与你需要的事件对应的通知，wait 也允许返回。第二种原因更常见：多个消费者醒来后抢同一个元素，最先持锁的消费者取走它，后一个拿到锁时队列又空了。第三种原因是同一个条件变量可能有并不满足你当前业务条件的通知。

因此 wait 返回只意味着可以重新观察，而不是获得一张“必定有数据”的票。用 `if` 检查一次就无条件消费，会把等待机制误当作状态存储。真正的记忆是受保护的 ready、队列内容、closed 等字段。

C1 先把 ready 设好、发送通知，再执行谓词等待，验证 payload 为 42。第二段启动三个等待者，各自在同一 mutex 下报到；主线程获得该锁并看到报到数为 3，能够确认这些线程已通过 wait 释放过锁。随后发送一次 ready 仍为 false 的无关通知，等谓词总检查数至少达到六次，再发布 99 并广播。检查所有返回值为 99。检查计数证明了 false 条件下发生过重新检查，但不要求人为制造实现层面的虚假唤醒，也不假定每个线程恰好检查两次；正确性来自循环谓词。

选做 `--unsafe-notify` 把“先 notify、后裸 wait”写成带 5ms 期限的诊断。它允许观察 timeout 或虚假唤醒，不把其中某个结果写死为断言。若希望演示永久遗漏唤醒，应在单独进程运行无期限版本并由外部终止；默认课程路径不这样做。

## 3. notify_one 与 notify_all 由状态变化决定

当队列只增加一个元素时，至多一个消费者可以成功取得它；把一个等待消费者唤醒通常足够。一次 pop 只释放一个槽位，通常也只需唤醒一个生产者。它们是同类等待者，谁先成功并不影响队列契约。

如果 ready 是所有等待者都应该通过的一次广播，或者 close 让所有等待者都应该退出，用 notify_all。只通知一个可能让其他已经睡下的线程永远没有机会重查已为真的状态。不能说“其余线程一定永远睡着”：它们可能尚未睡下或虚假唤醒；准确结论是单次 notify_one 不提供全部前进的保证。

若不同谓词共用一个 CV，notify_one 可能选中不能前进的那类线程，而真正可以前进的线程仍在等待。C2 因此给生产者和消费者分配 `not_full_`、`not_empty_` 两个 CV，分别表达不同的等待原因。它们共享同一 mutex，因为容量与元素数来自同一个状态机。

通知可以发生在解锁前，也可以在解锁后。持锁通知本身合法，只是被唤醒者还要等 mutex；解锁后通知常能减少这种竞争。它绝不允许对象在解锁与通知之间被销毁。Reference 的拥有者保证所有 push/pop/close 调用返回、参与线程 join 后，才析构通道。

## 4. 期限等待不能每醒一次就重新计时

需求是“最多等待一段预算”，不是“每一次内部等待都允许再用完整预算”。下面这种思路会因无关通知不断延长总等待：反复调用 `wait_for(lock, 100ms)`，返回后发现条件仍假，就再等 100ms。

应在进入循环前用 `steady_clock` 计算一次绝对 deadline，再让每次重试使用同一期限；或者直接使用标准的谓词版 `wait_for`，由其内部维护相对期限对应的等待协议。

```cpp
const auto deadline = std::chrono::steady_clock::now() + budget;
bool ready_now = cv.wait_until(lock, deadline, [&] { return ready; });
```

返回 bool 表示最终谓词值。即使时间已经到达，重新拿到锁后发现 ready 为真，也可能返回 true；裸等待的 `cv_status` 不是业务条件的答案。返回 false 时，应在当前锁保护下处理“本次观察条件仍不满足”的分支，不要解锁后又使用旧条件。

mutex 竞争、线程调度、时钟粒度会让实际返回晚于 deadline，这不是硬实时接口。C1 检查 false 谓词到期返回 false，以及过期期限下已经为真的谓词仍返回 true，不断言耗时恰好为 2ms。C3 进一步演示可中断定时等待的 false 还可能来自取消，因此不能仅凭返回值确定原因。规范入口为 [`[thread.req.timing]`](https://eel.is/c++draft/thread.req.timing) 与 [`[thread.condvarany.intwait]`](https://eel.is/c++draft/thread.condvarany.intwait)。

## 5. 给有界通道写出完整契约

`cs::bounded_channel<T>` 是教学用 MPMC 阻塞 FIFO，不是 C++ 标准库提供的类。它的协议如下：

| 项目 | 本实现承诺 |
|---|---|
| 容量 | 构造时固定且大于零；零容量抛 invalid_argument |
| 线程角色 | 多生产者、多消费者；允许与 close 并发 |
| push(T) | 满则等待；关闭后返回 false；成功返回 true |
| pop() | 空且开放时等待；返回 optional<T>；关闭且排空才为 nullopt |
| 顺序 | 入队与出队按持锁生效位置解释为全局 FIFO；不保证各线程返回先后 |
| close() | 幂等，拒绝新入队，保留已接受数据并广播两类等待者 |
| 元素 | 无抛出移动构造、无抛出析构；其操作不得重入本通道 |
| 所有权 | push 按值接收；pop 返回独立拥有的值；不暴露内部引用 |
| 进展 | 获取 mutex、等待空间/数据均可阻塞；没有公平或无饥饿保证 |
| 寿命 | 拥有者停止并 join 所有调用线程以后才能析构；close 本身不 join |

按值 push 的细节很重要：调用者传入右值 unique_ptr 时，在进入函数前就可能已经转移所有权。即使因为关闭而返回 false，调用者也不能假设原值还在。它只承诺本次没有进入通道，不承诺把参数还给原对象。需要失败后保留资源的接口，应另外设计返回被拒绝元素的结果类型，这属于契约变更。

## 6. 从不变量推导环形存储

共享实现用 `vector<optional<T>>` 在构造时一次分配槽位，不要求 T 默认构造。`head_` 指向下一次取值位置，`tail_` 指向下一次写入位置，`size_` 区分满与空。容量 1 时 head 与 tail 始终都是 0，但 size 在 0 和 1 之间变化，因此同样有效。

需要维持三个关系：`0 <= size_ <= capacity`；从 head 开始连续 size 个逻辑位置含有有效 T；closed 只会由 false 变成 true。所有槽位和计数访问都在同一 mutex 下，所以不需要再把这些字段做成 atomic。

push 先等 `closed_ || size_ < slots_.size()`。关闭优先检查：即使还有空间，关闭后也不能接受。然后在 tail 构造元素，tail 环绕，size 加一，解锁并通知消费者。成功入队的线性化位置可以选择 size 增加完成处；失败位置是锁内观察到 closed 的位置。

pop 等 `closed_ || size_ != 0`。如果 size 为零，此时必然 closed，返回 nullopt；否则先移动出队首，再 reset 槽位、推进 head、减少 size，解锁并通知生产者。成功出队的线性化位置选在 size 减少完成处。close 在锁内设置 closed 的时刻生效，因此并发 push 要么已被接受并属于排空责任，要么观察到关闭并失败。

无抛出移动和析构限制让“取出后更新索引”没有元素异常留下的半完成状态。构造 vector 的分配仍可能失败，但此时对象尚未发布。函数实参的复制、移动表达式也可能在进入函数前失败；通道还没改变。虽然存储不在 push 中分配，T 自己的移动可能有成本甚至阻塞，所以“内部锁算法短”不能推出完整操作具有无锁进展保证。

## 7. close 是数据流终点，不是销毁命令

生产者全部结束后，拥有者 close，消费者循环 pop 直到 nullopt，然后 join 消费者，最后销毁通道。这是一个完整的退出协议。

如果 close 只通知消费者，满队列上的生产者可能永远等不到空间；如果只通知生产者，空队列上的消费者可能永远等不到元素；如果等待谓词没加入 closed，就算广播，它们醒来仍可能继续睡。因此“修改终止状态、在每条等待路径检查它、唤醒所有受影响等待者”缺一不可。

不能在 close 后立即 delete 通道：已唤醒线程仍需访问 mutex、CV 和存储，正在返回的生产者还可能调用 notify。close 只改变业务状态，join 才回收执行者。取消某个消费者也不等于关闭全局数据流；下一章会明确这两个契约。

## 8. 怎样检查没有遗漏和顺序错误

C2 [checks.hpp](../exercises/C2_bounded_queue_condvar/checks.hpp) 的 `sequential<Channel>()` 先验证零容量异常、unique_ptr 元素、环形槽位复用、关闭后拒收以及关闭后仍按 20、30 排空。`concurrent<Channel>(1)` 让三个生产者各生产 200 个不重叠 ID，由单消费者保留真实取出序列；检查每个生产者内部递增顺序，再排序检查所有 ID 恰好一次。每次操作调用传入的 Channel 类型：solution.cpp 传公共答案 cs::bounded_channel，main.cpp 传待完成的 student_channel。检查头本身不包含答案实现。

`concurrent(2)` 使用两个消费者。各消费者记录自己的返回序列，最后合并检查 ID 集合。这里不能把合并后的向量当作全局 pop 顺序：消费者 A 可能先取出、后记录，消费者 B 先记录了后取出的元素。FIFO 论证来自临界区，逐项检查支持本次运行；完整并发历史线性化验证需要记录调用/返回区间并进行合法序列搜索，本题没有冒充完成那种证明。

`close_waiters()` 在满队列上发起多个 push、空队列上发起多个 pop，再关闭并等待全部 future。合法调度可能让部分调用在 close 前进入等待，另一些在 close 后直接看到终态；测试覆盖关闭竞态及终态检查，但不能仅凭线程启动断言“所有线程已经停在内核等待”。所有必需等待都由 CTest 的 30 秒进程超时兜住，测试不以 sleep 证明顺序。

```powershell
# 在 C08_Concurrency/exercises 下
cmake -S C2_bounded_queue_condvar -B build/c2 -G "Visual Studio 18 2026" -A x64 -DBUILD_TESTING=ON -DCONCURRENCY_STUDY_TEST_STARTERS=OFF
cmake --build build/c2 --config Release
ctest --test-dir build/c2 -C Release -R '^C2_bounded_queue_condvar_reference$' --no-tests=error --output-on-failure
```

上面只验 Reference。C2 Starter 的 student_channel 留有构造、push、pop、close 四组局部提示与状态成员；C1 则要求完成 student_wait、student_broadcast、student_wait_until。完成对应 Part 后解除 part*_done 启动保护，执行实际调用学生实现的测试：

```powershell
cmake -S C2_bounded_queue_condvar -B build/student-C2_bounded_queue_condvar -G "Visual Studio 18 2026" -A x64 -DBUILD_TESTING=ON -DCONCURRENCY_STUDY_TEST_STARTERS=ON
cmake --build build/student-C2_bounded_queue_condvar --config Release --target C2_bounded_queue_condvar
ctest --test-dir build/student-C2_bounded_queue_condvar -C Release -R '^C2_bounded_queue_condvar_student$' --no-tests=error --output-on-failure
```

原样 Starter 在创建线程前返回 1；该 student 测试预期失败，默认门禁仍只运行 Reference。把题名替换成 C1 或使用其 README 的完整两套命令，可运行前半章。所有检查使用 cs::check；异步线程异常经 future.get 回到主线程，生产/消费错误会尝试先关闭队列。学生实现若破坏等待/关闭协议，由 CTest 30 秒超时限制，不能借 Reference 的通过宣称学生代码正确。

## 自测与解析

**条件变量保存了几次通知？** 不保存通知计数。需要保存事件数量时，用业务队列、受保护计数或信号量。

**为什么不能看到非空就解锁，再读 front？** 其他消费者可以取走并销毁该对象；检查、移动和移除必须作为同一次操作完成。

**notify_all 能代替谓词吗？** 不能。它改变等待者的调度机会，不能证明状态满足，也不能阻止另一消费者先拿走资源。

**容量为一时同时入队两个任务会怎样？** 第一个成功后 size 为一，第二个必须等 pop 或 close；capacity 限制的是尚在队列中的元素，不包括已经交给消费者执行的任务。

**取消和 close 哪个更“高级”？** 它们解决不同需求。close 宣告不再有新元素且保留已接受值；取消表达某个操作不再愿意等。二者可以组合，但必须指定优先级和剩余元素的归属。

**通过运行可否断言比 Q0 更快？** 不可以。本题没有进行等契约计时；等待避免部分空转，增加了登记、唤醒与调度成本。性能结论要另设公平负载与完成量检查。
