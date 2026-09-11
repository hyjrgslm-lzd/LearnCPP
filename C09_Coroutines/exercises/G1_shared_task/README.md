# G-1 从 task 到 shared_task

对应正文：[09 模块 G](../../09-模块G-symmetric_transfer与高级task.md#g1)。

`task<T>` 是单 owner、单次消费。`shared_task<T>` 要让同一个 producer 执行一次，多个 awaiter 都读到同一个结果。本题重点是共享状态、结果缓存和多 waiter 唤醒。

**本题基线契约：** 示例在同一线程上登记与恢复；每个已登记 waiter 的 owner 必须活到 producer 完成，不能提前销毁。`resume()` 只用于本题受控的 `suspend_always` 观察点，不是任意异步任务的驱动入口。Reference 本轮验证 `int`，不把这些结果推广成所有 T、任意线程或取消协议。

满足上述 owner 顺序的释放路径应能通过 ASan；先销毁登记 waiter 再恢复 producer 会产生 UAF。后者是本基线不支持的输入，不进入默认运行。需要支持等待者放弃时，继续到 Capstone5 的弱登记/phase 实现；这是契约扩展，不是宣称当前基线已支持注销。

## Part 1：对象关系

先画：

```text
shared_task<T> copies
  -> shared_ptr<control_block>
       started/completed
       vector<coroutine_handle<>> waiters
       producer handle -> promise { value, error, weak owner }
```

本题 control_block 共同拥有 producer frame；结果留在 promise，后续 awaiter 仍可读取。Capstone5 则把缓存放进独立 shared_state，producer 按值持有它直到收束；state 不反向持有 producer，避免引用环。

## Part 2：首次等待启动 producer

第一个 awaiter：

```text
本题单线程：completed=false
登记 caller 到 waiters
started=false -> true
返回 producer handle，转交执行
```

第二个 awaiter 只登记 waiter，不重复启动 producer。

## Part 3：完成后唤醒全部 waiter

producer 成功时在 promise 保存 value，失败时保存 error；final awaiter 设置 completed 并取出 waiters。它恢复后面的等待者，再返回首个 handle 做控制转交。每个 waiter 在 `await_resume()` 中读取 promise 缓存。返回值要拷贝，不能让第一个消费者 move 走共享结果。

## 验收

以下解析对应本目录的 [solution.cpp](solution.cpp)：`control_block` 共同拥有 producer frame，结果保存在该帧的 promise 中。[Capstone5 reference](../Capstone5_mini_corolib/reference/include/mini_ref/mini.hpp) 提供独立缓存、弱 waiter 与 producer 自收束的对照，不能混用两者的字段图和线程/注销保证。

- producer 只执行一次。

  **答案解析：** 本目录 `control_block::started` 记录 producer 是否已经启动。第一个 awaiter 登记后，用 `std::exchange` 把标记置为 true，并返回 producer 的 handle 以转交执行；后续 awaiter 只登记自己并返回 noop handle。当前示例在同一线程上依次登记等待者，`producer_runs == 1` 对应共享 producer 只进入函数体一次。
- 两个 waiter 都拿到相同值。

  **答案解析：** `co_return 42` 把值存入 producer promise 的 `value`。共享 control block 持有该 frame，两个 waiter 在 `await_resume()` 中通过 `cb->h.promise()` 访问同一结果，并各自取得拷贝。Reference 中 `a == 42 && b == 42` 展示了这次结果保存和重复消费。
- 异常能从每个 waiter 的 `await_resume()` 传播。

  **答案解析：** 本题 promise 的 `unhandled_exception()` 把异常存入 `error`，每个 waiter 的 `await_resume()` 都会检查该字段并重新抛出。观察错误路径时，在各等待者的 `co_await` 周围捕获并记录异常，就能看到共享的失败结果；各等待者仍按自己的异常策略处理它。Capstone5 的另一种实现把 `error` 放在独立 `shared_state<T>` 中。
- 没有 handle 被 resume 两次。

  **答案解析：** 本题 final awaiter 先设置 `completed=true`，再将登记的 waiters 移出 control block；它显式恢复后面的等待者，并返回首个等待者 handle 做控制转交。后来登记的 awaiter 看到 completed 会直接取值。这个单线程示例依靠“每个 waiter 登记一次、完成列表处理一次”组织恢复；并发登记还需要独立同步设计。
- 能指出生产实现还需处理提前销毁 waiter 与并发完成窗口。

  **答案解析：** G1 的 `control_block` 使用 vector 保存 waiters；Capstone5 的实现进一步用 mutex 保护等待者登记与完成状态。扩大使用范围时，需要处理等待者提前销毁后的注销、登记与完成同时发生，以及最后一个共享 owner 释放时 producer 的存活关系。这些条件决定了共享状态、producer frame 和各等待者之间的生命周期安排。

## Reference

Reference 验证两个 awaiter 都拿到值且 producer 只执行一次；教学重点是 control block、结果缓存、异常缓存和多等待者唤醒。

```powershell
cmake -S . -B build/dg-lane -DCOROUTINE_STUDY_BUILD_REFERENCE=ON
cmake --build build/dg-lane --config Release --target G1_shared_task G1_shared_task_reference
ctest --test-dir build/dg-lane -C Release -R G1_shared_task_reference
```
