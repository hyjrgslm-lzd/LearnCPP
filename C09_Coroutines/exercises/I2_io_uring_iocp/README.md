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
cmake -S C09_Coroutines/exercises -B build/coroutine-i2 -DCOROUTINE_STUDY_ENABLE_IOCP=ON -DCOROUTINE_STUDY_BUILD_REFERENCE=ON
cmake --build build/coroutine-i2 --target I2_io_uring_iocp_reference
ctest --test-dir build/coroutine-i2 -R I2_io_uring_iocp_reference --output-on-failure
```

Linux：

```bash
cmake -S C09_Coroutines/exercises -B build/coroutine-i2 -DCOROUTINE_STUDY_ENABLE_IO_URING=ON -DCOROUTINE_STUDY_BUILD_REFERENCE=ON
cmake --build build/coroutine-i2 --target I2_io_uring_iocp_reference
ctest --test-dir build/coroutine-i2 -R I2_io_uring_iocp_reference --output-on-failure
```

回读代码时只追一条链：`await_suspend` 提交请求，loop 取 completion，awaiter 保存结果，`await_resume` 返回字节数。

**答案解析：** 这条链回答所有权和数据流：awaiter 由协程帧拥有，OS 只保存能找回 awaiter 的身份 token，event loop 是写结果和恢复 handle 的执行者。先写结果再 resume，能保证业务代码从 `await_resume()` 看到完整 completion 状态。

## IDE 与单题构建

Visual Studio 中主启动目标是 `I2_io_uring_iocp`；Reference、Checks、Support 目标保留在同题分组中。学生编辑入口和本题 README/CMake 文件会显示在目标文件树里。

```powershell
cmake -S . -B build/vs -G "Visual Studio 18 2026" -A x64 -DCOROUTINE_STUDY_BUILD_REFERENCE=ON -DCOROUTINE_STUDY_ENABLE_IOCP=ON
cmake --build build/vs --config Debug --target I2_io_uring_iocp
```

本题单独配置需要 `-DCOROUTINE_STUDY_ENABLE_IOCP=ON`，并提前准备 Windows IOCP。Windows 直接使用 IOCP，命令使用 COROUTINE_STUDY_ENABLE_IOCP=ON。Linux 改用 -DCOROUTINE_STUDY_ENABLE_IO_URING=ON，并通过 -DCOROUTINE_STUDY_LIBURING_ROOT=<existing-liburing-root> 或 pkg-config 提供 liburing；不要联网准备依赖。 缺依赖时配置阶段直接失败，不生成空工程。 `BUILD_TESTING=OFF` 只关闭测试注册，不删除本题可执行目标；不要手工编辑生成的 `.sln` 或 `.vcxproj`。
