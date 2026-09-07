# I2 io_uring / IOCP awaiter

知识讲解：[I2 对应章节](../../11-模块I-真实异步IO与并发框架.md#i2)。

对应主讲义：`11-模块I-真实异步IO与并发框架.md` 的 I-2。

这题看 OS completion 如何恢复协程。awaiter 必须活到 completion 到达；最小安全形状是 awaiter 位于协程帧中，`await_suspend` 提交一次异步请求，event loop 取回身份 token 后写 result，再 resume。

Linux reference 用 `io_uring_sqe_set_data(sqe, this)` 保存 awaiter 身份，`io_uring_cqe_get_data(cqe)` 取回，先 `io_uring_cqe_seen()` 再 resume。预测输出：`CQE user_data resumed coroutine`。

**答案解析：** `this` 指向位于协程帧中的 awaiter，completion 到达后 CQE 的 `user_data` 让 event loop 找回同一个 awaiter。loop 把 `cqe->res` 写进 awaiter，消费 CQE，再恢复 continuation；协程随后在 `await_resume()` 读取字节数或错误。

Windows reference 把 `OVERLAPPED ov` 放在 awaiter_base 第一个成员。`GetQueuedCompletionStatus` 返回 `OVERLAPPED*` 后还原 awaiter，写入 `transferred/error`，再 resume。预测输出：`IOCP resumed coroutine with loopback bytes`。

**答案解析：** `static_assert(offsetof(awaiter_base, ov) == 0)` 保证 `OVERLAPPED*` 能还原成 awaiter base 地址。`GetQueuedCompletionStatus` 成功时写入 transferred bytes；失败但 `ov` 非空时仍是一次失败 completion，要记录 `GetLastError()` 并恢复协程。reference 的 loopback payload 是 `"iocp-loopback"`，恢复后检查收到的字符串相同。

Windows：

```powershell
cmake -S Coroutine_Study/exercises -B build/coroutine-i2 -DCOROUTINE_STUDY_ENABLE_IOCP=ON -DCOROUTINE_STUDY_BUILD_REFERENCE=ON
cmake --build build/coroutine-i2 --target I2_io_uring_iocp_reference
ctest --test-dir build/coroutine-i2 -R I2_io_uring_iocp_reference --output-on-failure
```

Linux：

```bash
cmake -S Coroutine_Study/exercises -B build/coroutine-i2 -DCOROUTINE_STUDY_ENABLE_IO_URING=ON -DCOROUTINE_STUDY_BUILD_REFERENCE=ON
cmake --build build/coroutine-i2 --target I2_io_uring_iocp_reference
ctest --test-dir build/coroutine-i2 -R I2_io_uring_iocp_reference --output-on-failure
```

回读代码时只追一条链：`await_suspend` 提交请求，loop 取 completion，awaiter 保存结果，`await_resume` 返回字节数。

**答案解析：** 这条链回答所有权和数据流：awaiter 由协程帧拥有，OS 只保存能找回 awaiter 的身份 token，event loop 是写结果和恢复 handle 的执行者。先写结果再 resume，能保证业务代码从 `await_resume()` 看到完整 completion 状态。
