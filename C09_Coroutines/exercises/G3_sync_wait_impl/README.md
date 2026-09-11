# G-3 实现 `sync_wait`

对应正文：[09 模块 G](../../09-模块G-symmetric_transfer与高级task.md#g3)。

`sync_wait` 是普通线程进入协程世界的入口。最终实现应启动 root task 一次，然后等待 final completion 通知；手动循环 resume 只适合受控演示。结果提取时的帧所有权与异常实验见正文“七点一”：完成通知、结果消费和销毁是三个不同责任。

## Part 1：理解 blind-resume 的边界

下面这种代码只能驱动最简单的 toy awaiter：

```cpp
h.resume();
while (!h.done()) h.resume();
```

它假设所有挂起点都能由当前线程继续恢复。遇到跨线程完成、`await_suspend` 同步恢复、symmetric transfer 或外部事件循环时，这个循环可能重复 resume、早读结果或访问已销毁状态。

## Part 2：final completion + condvar

生产形状：

```text
sync_wait 创建 state { mutex, cv, done=false }
promise 设置 completion callback
task.start() 一次
当前线程 cv.wait(done)
final_suspend 调用 callback
callback 设置 done=true 并 notify
sync_wait 醒来读取 value/error
```

异常存放在 promise 中，`sync_wait` 醒来后重新抛出。本题复用的 `coroutine_study::sync_wait(lazy_task<void>&&)` 返回 `void`；不要与某些 sender 同步消费者的 optional/tuple 返回协议混淆。

## Part 3：回答 main 为什么不能是协程

C++ 标准没有定义 main 协程的启动和销毁实体：谁分配 main frame，谁在 initial suspend 后 resume，谁最终 destroy。`sync_wait` 就是普通 `main()` 自己可以调用的桥接入口。

## 验收

- 成功路径返回正确值。

  **答案解析：** `sync_wait` 先把 completion callback 接到 root promise，再调用一次 `start()`。协程正常 `co_return` 时把值存在 promise 中，`final_suspend` 通知等待线程，线程醒来后读取 promise value。G-3 的 `nested()` 还验证了嵌套 task 完成也会回到 root final path。
- 错误路径把协程内异常重新抛到 `sync_wait` 调用者。

  **答案解析：** 协程体异常先由 `unhandled_exception()` 保存到 promise，不直接穿过异步边界。`sync_wait` 被 final completion 唤醒后检查 promise 的 error 并重新抛出。reference 的 `fail()` 场景捕获 `std::runtime_error`。
- root task 只启动一次，二次 start 被拒绝。

  **答案解析：** `sync_wait` 要求传入未启动 task，并只调用一次 `start()`。启动后 promise 中的 started 标志已建立，手动再次 `start()` 会抛 `logic_error`，防止同一 frame 被重复 resume。G-3 的 `auto t = value(); t.start(); t.start();` 验证这个保护。
- 跨线程完成能唤醒等待线程。

  **答案解析：** 跨线程 awaiter 可能在另一个线程恢复 root task 或其子链。只要最终 root 进入 `final_suspend`，completion callback 就会加锁设置 done 并通知 condition_variable。等待线程不需要知道哪个线程完成了协程，只等 final path 的统一信号。
- 你能说明手动循环和 condvar/run_loop 方案的适用边界。

  **答案解析：** 手动循环 `resume()` 只适合完全由当前线程推进的 toy coroutine，用来观察状态机。真实异步 task 可能由外部事件、跨线程回调或 symmetric transfer 恢复，循环 resume 会造成早读、重复恢复或破坏同步完成窗口。condvar 适合阻塞普通线程等最终完成，run_loop 适合事件循环式恢复队列中的 handle。

## Reference

Reference 复用公共 `lazy_task/sync_wait`，验证 root start once、nested completion、二次 start 拒绝、异常重抛和跨线程完成。

```powershell
cmake -S . -B build/dg-lane -DCOROUTINE_STUDY_BUILD_REFERENCE=ON
cmake --build build/dg-lane --config Release --target G3_sync_wait_impl G3_sync_wait_impl_reference
ctest --test-dir build/dg-lane -C Release -R G3_sync_wait_impl_reference
```
