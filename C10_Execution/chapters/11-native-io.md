# 11. Native I/O sender

本章只解决一个问题：真实 OS completion 如何变成 sender 的三种完成信号。C07 已经讲过句柄、文件身份、IOCP、io_uring 和取消系统调用；这里把它们接到 execution 的 `connect/start/receiver` 契约上。

## 场景

我们要实现：

```cpp
auto sender = read_at(file, offset, std::span<std::byte>{buffer});
auto op = stdexec::connect(sender, receiver);
stdexec::start(op);
```

`buffer` 是借用，不由 sender 分配。`file` 是已打开的 regular file。`op` 是 operation state，地址必须稳定到完成信号发出为止。完成信号发出后，receiver 可能销毁 `op`，所以后端回调在调用 `set_value/set_error/set_stopped` 后不能再访问 operation state。

## 两层责任

底层 `c10_native::io_context` 是 provided backend。它负责打开 bounded regular file，在 Windows 使用 overlapped `ReadFile` + IOCP，在 Linux 使用 `io_uring` 提交 read，并保持 OS request 活到 completion。它也区分 backend unsupported 与 setup/runtime failure。

练习实现层负责定义 sender 类型和 completion signatures；每次 `connect` 创建独立 operation state；在 `start` 中读取 receiver environment 的 stop token；把 backend completion 映射到 receiver；保证 exactly once terminal signal。

这样拆分不是把答案藏进公共头。公共头没有 sender，没有 `connect`，没有 receiver 环境转发，也没有完成通道映射；这些必须在练习里写。

## 状态机

operation state 最小状态：

1. `constructed`：保存 file、offset、buffer、receiver。
2. `started`：检查 stop token；若已请求停止，直接 `set_stopped`。
3. `registered`：注册 stop callback。callback 只记录 stop 请求并尝试取消已提交 request。
4. `submitted`：调用 backend `async_read_at`。若提交失败，走 `set_error`。
5. `settled`：backend 回调一次，取消 stop callback，移动 receiver，发一个终结信号。

普通错误不能因为曾经请求过 stop 就改成 stopped。只有 backend 报告取消完成，才映射为 `set_stopped`。如果目标 read 已经成功，之后到达的取消请求输了竞态，结果仍是 `set_value`。

## 失败边界

Linux 没启用 liburing 是配置错误，由 CMake 报错。内核或策略明确不支持 io_uring 是能力 SKIP。文件打开失败、EMFILE、ENOMEM、线程创建失败、队列运行错误都是 FAIL。`offset > size`、buffer 超过公开上限、非 regular file 是输入错误。短读与 EOF 是文件结果，不是 backend failure。

## Part 解析

Part 1 写 sender shell：`read_sender` 保存 file、offset、buffer，`connect` 返回 operation state。

Part 2 写 `start`：查 stop token，注册 callback，提交 backend。提交失败要发 error，不能抛出到 caller。

Part 3 写 completion mapping：`completion.stopped -> set_stopped`，`completion.error -> set_error(exception_ptr)`，否则 `set_value(read_result{bytes})`。

Part 4 验证真实 I/O：打开失败、offset read、短读、多在途、pre-stop、cancel/read race、close 后拒绝 open。

## Linux submit failure boundary

io_uring 的 SQE 一旦写入，用户态不能在 `io_uring_submit()` 返回异常形状时假设内核没有看到它。本章实现对 `submitted != 1` 和 wait fatal error 采用 fail-fast 的 fatal-undrained 边界：不释放 request，不发送伪造 completion，不让后续 submit 把悬空 user_data 冲进内核。课程正文把它标成基础设施失败；普通能力缺失仍由 probe/unsupported 路径报告 SKIP。

## Deterministic boundary checks

I1 的 checker 实际运行这些稳定边界：`offset == file.size()` 返回 0 字节；`offset > file.size()` 拒绝；buffer 超过 1 MiB 拒绝；目录路径作为 non-regular file 拒绝；稀疏文件大小超过 1 MiB 拒绝。SQE/IOCP 队列耗尽依赖系统瞬时资源，当前不做自动化耗尽测试；submit 异常路径由 Linux fatal-undrained 规则覆盖，不把普通库准备错误伪装成 SKIP。

## 完成早于提交返回的回归实验

Windows 的 IOCP worker 可能在 ReadFile 尚未返回时取到完成包。后端必须先把 request 注册进持有表，再向系统提交；否则 take_alive 找不到请求，完成信号被丢弃，sync_wait 永久等待。同步提交失败时撤销注册并回滚在途计数；持有表分配失败在提交前转为资源错误。Linux 同样先持有再准备/提交 SQE，无空闲 SQE 时撤销本次持有。

I1 Windows checker 对各实现启用 C10_NATIVE_IO_TEST_POST_SUBMIT：立即保存 GetLastError 后延迟10ms，放大 completion 抢先窗口。这是测试编译开关，P1 正常路径不启用。旧顺序在受控副本中稳定 timeout；当前顺序必须在同样窗口下完成并关闭。延长超时不能修复丢失完成，原始对照见 io-review/review-report-r4.md 所列记录。

本教学后端的open/submit与close由所有者外部串行化；完成与停止回调可以并发。close负责等待已接受I/O，context寿命还必须覆盖调用者与停止回调退出。若要支持close和新提交竞争，需要额外的接纳锁建立共同线性化点；当前课程调用方式不依赖这项扩展。
