# 12 Windows IOCP：完成通知与请求身份

## IOCP 解决的问题

同步 I/O 让调用线程等到结果。非阻塞 readiness 让线程知道“现在尝试可能推进”。IOCP 的问题不同：先把一个 overlapped 请求交给系统，之后从 completion port 取回完成包。完成包告诉你哪个关联对象、哪个请求上下文、传输了多少字节，以及最终错误。

因此 IOCP 的核心不是“异步更快”，而是“请求接受后，完成以前谁还活着”。`OVERLAPPED`、缓冲区、request_id 和业务状态必须活到 `GetQueuedCompletionStatus` 返回对应完成包。只要请求已被接受，就不能因为取消函数返回、句柄关闭或上层超时而直接释放这些对象。

## 关联、提交与完成

典型步骤是：创建支持 overlapped 的 HANDLE，用 `CreateIoCompletionPort` 把它关联到一个 port，提交 `ReadFile`/`WriteFile` 等 overlapped 操作，然后在 port 上等待完成。同步完成和异步完成都要按同一条逻辑收束：如果 API 表示操作已接受，最终结果必须进入状态机，不能只有“返回 true”那条分支。

completion key 和 `OVERLAPPED*` 是定位请求的入口。课程样例不把 `OVERLAPPED` 当作裸临时变量到处传，而是把它视作 operation context 的一部分。真实项目通常会把 context 嵌入结构体，再从完成包里的指针找回 request_id、缓冲区和回调。

如果 `ReadFile` 返回 false 且 `GetLastError()==ERROR_IO_PENDING`，这不是失败；它表示请求等待完成。如果返回 true，也可能已经完成但仍按 overlapped 规则处理。错误分支必须先判断请求是否已经接受。已接受请求的失败处理不是“return error”，而是 cancel/drain 或 fatal 边界，保证栈上的 buffer 和 `OVERLAPPED` 不被内核继续使用。

## 取消不是完成

`CancelIoEx` 的返回值描述取消请求本身，不描述目标 I/O 最终已经怎样结束。目标请求可能以 `ERROR_OPERATION_ABORTED` 完成，也可能在取消到达前正常完成。两种结果都要求从 port 取走完成包。把取消成功当作可以释放 context，是典型 UAF 起点。

L08 的 Windows platform probe 覆盖两类现象。第一类是临时文件：用 `FILE_FLAG_OVERLAPPED` 打开，关联 IOCP，提交 `ReadFile`，然后从 `GetQueuedCompletionStatus` 取回 completion。`operation_context` 持有 request_id、buffer 和 `OVERLAPPED`：

```cpp
operation_context read_context{1001, std::vector<std::byte>(payload.size())};
ReadFile(file.get(), read_context.buffer.data(), DWORD(payload.size()), nullptr,
         &read_context.overlapped);
auto event = wait_iocp_event(port.get(), read_context,
    completion_event_kind::read_completed, 5000);
```

第二类是 pending cancel：打开临时目录 watch，提交 `ReadDirectoryChangesW`，不制造目录变化，随后调用 `CancelIoEx`，再从 IOCP drain 目标 completion。这里 `CancelIoEx` 的结果记录成 `cancel_completed` 事件；它不是额外 IOCP 包。目标请求自己的最终结果记录成 `target_completed`。

checker 不相信实现自报状态。默认 L08 只消费 ledger，拒绝缺失 target completion、重复 completion、未知 ID、错误顺序和错误 bytes；带 `--platform` 时才运行真实 IOCP probe，并把真实事件交给同一个 ledger 校验。

## 本章边界

本章不实现通用线程池或 Reactor。最小模型是单线程 submit/reap、有限在途请求、明确 request_id。线程池、协程恢复和 sender completion signal 属于 C08/C09/C10 的承接内容。

本章也不把所有 Windows 等待对象都纳入 IOCP。只有支持 overlapped completion 的资源才能按这种方式使用；不同资源的打开 flag、权限和完成行为要分别验证。当前 L08 没有使用 NULL DACL，也没有宣称早期 named pipe `Access is denied` 的确切根因已经定位；最终平台路径使用文件 read 与目录 watch，减少无关权限变量。

失败路径若已经提交请求，必须先 cancel/drain。实验若无法收束已接受请求，会用不分配的诊断 `_Exit(70)` 退出受监督进程；这是课程 probe 的 fatal 边界，不是生产恢复策略。
