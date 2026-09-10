# L08 completion：完成 ledger、取消竞态与平台 probe

先读 [12 Windows IOCP](../../chapters/12-windows-iocp.md)、[13 Linux io_uring](../../chapters/13-linux-io-uring.md) 和 [14 取消与关闭](../../chapters/14-cancellation-shutdown.md)。本题不要求 Student 写平台 I/O 引擎。平台驱动由 `c07::run_completion_probe()` 提供；Student 只实现 [src/student/completion.hpp](src/student/completion.hpp) 里的 `validate_completion_ledger(report)`。

默认 `L08_completion_*` 只运行 ledger 检查。带 `--platform` 的测试才调用真实 Windows IOCP 或 Linux io_uring。这样可以把“状态机是否正确”和“当前机器是否支持平台机制”分开：OFF 时不注册 platform 测试，不是 SKIP；已启用平台但能力缺失时，platform 测试输出 `SKIP:` 并返回 77。

## Part 1：读懂事件模型

公共事件在 `include/c07/completion_io.hpp`：

```cpp
enum class completion_event_kind {
    read_accepted,
    read_completed,
    cancel_submitted,
    cancel_completed,
    target_completed
};

struct completion_event {
    std::uint64_t request_id;
    completion_event_kind kind;
    std::uint64_t target_request_id;
    std::size_t bytes;
    std::error_code error;
};
```

`read_accepted` 表示请求已经被平台接受，后续必须看到同一 request_id 的完成。`read_completed` 表示普通 read 完成，bytes 必须等于 payload 长度。`cancel_submitted` 表示取消控制动作已提交或调用。`target_completed` 表示被取消目标请求已经收束。`cancel_completed` 在 Windows 中记录 `CancelIoEx` 调用结果；它不是额外 IOCP 包。在 Linux 中它来自 async cancel 自己的 CQE。

## Part 2：接受两种合法竞态

取消和目标完成有竞态。checker 给出两份合法 ledger：

```cpp
read_accepted(17, payload.size())
read_completed(17, payload.size())
read_accepted(29, 1)
cancel_submitted(31, 29)
target_completed(29, 0, operation_canceled)
cancel_completed(31, 29)
```

以及目标先完成、取消随后报告找不到目标的情况：

```cpp
read_accepted(53, 1)
target_completed(53, 0)
cancel_submitted(59, 53)
cancel_completed(59, 53, no_such_process)
```

Student 不能只检查事件数量。必须按 request_id 和 target_request_id 建立 accepted set，保证完成发生在接受之后，普通 read 的 bytes 正确，取消目标最终收束，取消结果只接受本题允许的竞态。

## Part 3：拒绝坏 ledger

checker 会构造这些反例：

- 缺失 target completion。
- 重复 completion。
- 未知 request_id。
- completion 出现在 accepted 之前。
- payload bytes 错误。
- 缺失 cancel completion。
- cancel 或 target 出现非预期 native error。

[validation/bad](validation/bad/completion.hpp) 只要看到一次正常 read 就返回成功，必须被 `missing target completion is rejected` 拒绝。Reference 和 good 是两份独立 ledger 校验实现；good 使用不同的数据结构，不调用 Reference。

## Part 4：真实平台 probe

Windows platform probe 使用两个受控场景。第一个场景创建临时文件，用 `FILE_FLAG_OVERLAPPED` 打开，关联 IOCP，提交 `ReadFile`，再从 `GetQueuedCompletionStatus` 取回 read completion，检查 payload。第二个场景创建临时目录，打开 directory watch，提交 `ReadDirectoryChangesW` 形成 pending I/O，调用 `CancelIoEx`，再从 IOCP drain 目标 completion。代码没有使用 NULL DACL，也没有把早期 named pipe `Access is denied` 当作已定位根因。

Linux platform probe 先初始化最小 ring，并用 liburing probe 检查 `IORING_OP_READ` 和 `IORING_OP_ASYNC_CANCEL`。主体先对临时文件提交 READ 并消费 CQE，再对 pipe 提交 pending read，随后提交 async cancel；目标 CQE 和 cancel CQE 分别收束。pipe read 的 offset 使用 `-1`，因为 pipe 没有普通文件偏移。

`operation_context` 持有 request_id、buffer 和 Windows `OVERLAPPED`。请求接受后，buffer/context 必须活到完成被消费。失败路径先 cancel/drain；如果实验无法收束已接受请求，`completion_probe_fatal_undrained()` 使用不分配的诊断并 `_Exit(70)`。这是实验 fatal 边界，不是生产恢复策略。

## 构建与运行

默认 ledger 测试：

```powershell
cmake -S C07_OS_Memory_System_IO/exercises -B C07_OS_Memory_System_IO/build/sample-author-msvc-core -G "Visual Studio 18 2026" -A x64
cmake --build C07_OS_Memory_System_IO/build/sample-author-msvc-core --config Debug --target L08_completion_reference L08_completion_validation_good L08_completion_validation_bad
ctest --test-dir C07_OS_Memory_System_IO/build/sample-author-msvc-core -C Debug -R "L08_completion_(reference|validation_good|validation_bad)" --output-on-failure
```

Windows platform 测试由 CMake 自动注册：

```powershell
ctest --test-dir C07_OS_Memory_System_IO/build/sample-author-msvc-core -C Debug -R "L08_completion_.*platform" --output-on-failure
```

Linux 启用固定 liburing 后运行：

```bash
cmake -S C07_OS_Memory_System_IO/exercises -B /root/learncpp-c07/build-l08 \
  -DCMAKE_BUILD_TYPE=Debug \
  -DC07_STUDY_ENABLE_IO_URING=ON \
  -DC07_LIBURING_ROOT=/root/learncpp-c07/deps/install-liburing-2.15
cmake --build /root/learncpp-c07/build-l08 --target L08_completion_reference L08_completion_validation_good L08_completion_validation_bad
ctest --test-dir /root/learncpp-c07/build-l08 -R "L08_completion" --output-on-failure
```

## 解析

通过默认测试说明 ledger 校验能区分正常完成、取消竞态和代表性错误状态。通过 platform 测试说明当前平台驱动能产生一份被同一 ledger 校验接受的真实完成记录。两者合起来仍不证明通用异步框架完成，也不证明性能收益。它只证明本题有限 request_id、payload、取消和 drain 契约闭合。
