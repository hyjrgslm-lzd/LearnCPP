# H-1 as_awaitable 桥

知识讲解：[H1 对应章节](../../10-模块H-协程与sender_receiver桥接.md#h1)。

对应主讲义：`10-模块H-协程与sender_receiver桥接.md` 的 H-1。

这题把一个 stdexec sender 变成可 `co_await` 的对象。先记住对象分工：sender 描述工作，receiver 接 completion，`connect` 产出 operation state，`start` 才启动。你的 `sender_awaitable` 要持有 operation state 和结果 storage；receiver 收到 `set_value / set_error / set_stopped` 后写结果，再恢复等待它的协程。

先预测 reference 输出：`just(42)` 和 `just(10)` 都会同步 `set_value`，因此 value path 返回 52；`just_error` 会在 `await_resume` 重新抛出并被 demo 捕获，返回 -1；`just_stopped` 会进入 stopped tag，抛 `stopped_error`，demo 返回 -2。

同步完成是本题最容易写错的点。`stdexec::just` 可以在 `start()` 调用栈内完成。协程进入 `await_suspend` 前已经挂起；`await_suspend` 发布 handle 后，同步或并发恢复都可能发生。reference 用原子握手让 completion 先写结果；如果 completion 早于 publication 完成，`await_suspend` 返回 `false`，协程经过挂起点后立即继续到 `await_resume`。这和 `await_ready(true)` 跳过挂起点不同。

运行：

```powershell
cmake -S Coroutine_Study/exercises -B Coroutine_Study/exercises/build-h -G Ninja -DCOROUTINE_STUDY_ENABLE_STDEXEC=ON -DCOROUTINE_STUDY_FETCH_DEPS=ON -DCOROUTINE_STUDY_BUILD_REFERENCE=ON
cmake --build Coroutine_Study/exercises/build-h --target H1_as_awaitable_reference
ctest --test-dir Coroutine_Study/exercises/build-h -R H1_as_awaitable_reference --output-on-failure
```

观察到 `value=52 error=-1 stopped=-2` 后，回到代码看三处：`bridge_receiver::set_value` 如何写入 result，`await_suspend` 如何保存 operation state，`await_resume` 如何把三条 completion channel 映射回协程世界。

**答案解析：** `value=52` 来自两次 `just` 同步 `set_value`，bridge receiver 依次写入 42 和 10，两个 `await_resume()` 把它们还原成普通整数再相加。`error=-1` 来自 `just_error` 写入 `exception_ptr`，`await_resume()` 在 `co_await` 位置重新抛出并被 demo 捕获；`stopped=-2` 来自 `just_stopped` 写入 stopped tag，再由教学 `stopped_error` 映射到 catch 分支。operation state 必须保存在 awaitable 成员里，因为异步 completion 到来前它仍代表已连接的 sender/receiver 状态。
