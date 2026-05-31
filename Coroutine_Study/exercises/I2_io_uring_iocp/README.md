# I-2 io_uring (Linux) / IOCP (Windows) 双版本 awaiter

对应文档：`11-模块I-真实异步IO与并发框架.md` 「练习 I-2」。

## 目标

为 Linux `io_uring` 和 Windows IOCP 两套底层异步 I/O 接口分别编写最小 awaiter，把
kernel completion 事件与协程的挂起/恢复打通。CMakeLists 用 `if(WIN32)` 分流到
`linux/main_io_uring.cpp` 或 `windows/main_iocp.cpp`，本机编译你能跑的那一份。

## 必做任务

### Linux 版

1. 安装依赖：`sudo apt install liburing-dev`。
2. 编译运行 `I2_io_uring_iocp`，验证它能从 `/etc/hostname` 读出几行。
3. 在笔记中画 kernel 队列与协程帧协作的时间线：
   `sqe_set_data(this) → SQ → kernel 处理 → CQ → wait_cqe → user_data → awaiter
   → cqe_seen → coro.resume()`。

### Windows 版

1. 编译运行 `I2_io_uring_iocp.exe`——它会监听 127.0.0.1:12345 等一个客户端连接。
2. 用 `nc 127.0.0.1 12345` 发一行字符串，验证 server 端 recv 输出。
3. 验证 `static_assert(offsetof(awaiter_base, ov_) == 0)` 编译通过——这是
   `OVERLAPPED* == awaiter*` 成立的前提。

## 进阶任务

- io_uring：用 `io_uring_prep_timeout` + `IORING_OP_LINK_TIMEOUT` 实现超时；
  开 `IORING_SETUP_SQPOLL` 模式对比延迟。
- IOCP：实现 `iocp_send_awaiter`，写完整 echo 循环；用 `RIO` 替换 `WSARecv`。
- 两边都做：用 `static_assert(offsetof(...))` 验证布局，避免依赖"第一个成员"隐式假设。

## 验收点

- io_uring 版能正确读到 cqe->res 字节数。
- IOCP 版能正确从 OVERLAPPED* 反向定位 awaiter。
- 你能讲清两套模型的同构性——awaiter 的 await_suspend 投递 / 事件循环找回 awaiter
  并 resume / await_resume 取结果。

## 提示

- io_uring 必须 `io_uring_cqe_seen(cqe)`，否则 CQ 会被填满。
- IOCP 中 `OVERLAPPED` 必须是第一个成员，否则 reinterpret_cast 越界。
- 同步失败路径（WSARecv 不返回 WSA_IO_PENDING）需要用 `PostQueuedCompletionStatus`
  人为 resume 一次，避免协程永远挂起。
