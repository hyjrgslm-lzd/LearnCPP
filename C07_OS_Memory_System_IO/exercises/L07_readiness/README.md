# L07 readiness

本题练的是“就绪通知”这一层，而不是完成式 I/O。检查器给你一个已经设成非阻塞的本地端点：Windows 是只绑定 `127.0.0.1` 的 loopback socket，Linux 是 `socketpair`。你的代码只需要实现一个小函数：在给定 `max_bytes` 预算内反复 `recv`，直到读到 EOF、遇到 would-block，或者预算耗尽。

接口在 `readiness.hpp` 中：

```cpp
std::expected<drain_result, std::error_code>
drain(native_endpoint endpoint, std::size_t max_bytes);
```

`drain_result::bytes` 保存本轮真正读到的字节；`would_block` 表示非阻塞读已经排空到暂时没有数据；`eof` 表示对端关闭了发送方向；`budget_exhausted` 表示调用者给的本轮预算先耗尽。三者不是同一种状态：读到一部分字节以后仍可能 EOF；读到一部分字节以后也可能只是 would-block；预算耗尽时还有没有剩余数据，函数本身不知道，所以要把控制权交还给调用者。

## Part 1：阻塞、非阻塞与 would-block

阻塞 socket 上的 `recv` 在没有数据时会睡住当前线程。就绪模型通常把 fd/socket 设为非阻塞，然后用 `poll`、`WSAPoll`、`epoll_wait` 等等待“现在做一次操作大概率不会卡住”。等待返回以后仍然必须用真实 I/O 验证状态，因为通知和实际读写之间存在竞态：别人可能已经消费了数据，对端可能关闭，缓冲区可能只剩一部分。

因此本题的正确循环不是“收到一次 readiness 就读一次”：

```cpp
while (result.bytes.size() < max_bytes) {
    auto n = recv(endpoint, buffer, wanted, 0);
    if (n > 0) { append bytes; continue; }
    if (n == 0) { result.eof = true; break; }
    if (error is would-block) { result.would_block = true; break; }
    return unexpected(error);
}
```

`EINTR` 只在 POSIX 路径需要重试：它说明系统调用被信号中断，并不说明 fd 状态已经改变。`EAGAIN`/`EWOULDBLOCK` 则是非阻塞 I/O 的正常控制流，不能当成失败。

## Part 2：排空与有限预算

就绪处理器有两个边界：一是“排空到 would-block”，二是“本轮预算”。边沿触发 `epoll` 尤其依赖排空；如果只读一次，内核不会因为缓冲区里仍有旧字节就持续给你重复事件。预算则是业务公平性边界：一个活跃连接不能无限占用事件循环，所以 `drain` 在 `max_bytes` 用完时返回 `budget_exhausted=true`，让上层稍后继续处理。

检查器每次运行都会生成未知 payload，并打印 `L07 readiness seed=...` 供复现。它覆盖这几个事实：

- 空端点必须返回 would-block，而不是挂住或伪造 EOF。
- 多片 payload 必须一次 drain 到完整运行时生成内容；bad 实现只读 4 字节，会在 `runtime payload matches drained bytes` 处被拒绝。
- 第二次调用要能继续处理后来到达的数据，不能用全局“已完成”标志跳过。
- 对端半关闭时，要返回已排队字节并标出 EOF。
- `max_bytes` 小于排队字节数时，要停在预算边界，下一次调用继续读剩余字节。
- writer 端切到非阻塞后，检查器会有限填充 socket 缓冲区，观察 would-block 背压，再确认 drain 释放的是本轮真实写入序列的正确前缀。

## Part 3：平台差异

Windows 路径只使用 Winsock：`ioctlsocket(FIONBIO)` 设置非阻塞，`WSAPoll` 做 readiness 观察，错误码来自 `WSAGetLastError()`，would-block 是 `WSAEWOULDBLOCK`。检查器的 socket 只在本机 loopback 上建立，不访问外网，也不模拟 epoll。

Linux 路径使用 `socketpair` 驱动学生函数，用 `poll` 观察普通 level-triggered readiness；额外观察 `epoll` 的三个机制：

- `EPOLLET` 下只读一字节后，`epoll_wait(..., timeout=0)` 不会因为缓冲区仍有旧数据而重复返回；随后直接 `read` 还能读出剩余字节，证明“事件消失”不是“数据消失”。
- `EPOLLONESHOT` 在第一次返回后会禁用该注册；即使后来又写入数据，也要 `epoll_ctl(EPOLL_CTL_MOD, ...)` 重新武装才会继续收到事件。
- 普通文件不是本题这种 readiness 源。检查器把真实 regular file 加入 epoll，要求 `epoll_ctl` 返回 `EPERM`，把“文件总是可读”和“可以挂进 epoll 事件循环”分开。

## 本地复现

Windows 单题构建：

```powershell
cmake -S C07_OS_Memory_System_IO/exercises/L07_readiness -B C07_OS_Memory_System_IO/build/readiness-author -G "Visual Studio 17 2022" -A x64
cmake --build C07_OS_Memory_System_IO/build/readiness-author --config Debug
ctest --test-dir C07_OS_Memory_System_IO/build/readiness-author -C Debug --output-on-failure
cmake --build C07_OS_Memory_System_IO/build/readiness-author --config Release
ctest --test-dir C07_OS_Memory_System_IO/build/readiness-author -C Release --output-on-failure
```

学生 baseline 可单独构建目标并直接运行，未实现时应安全返回错误而不是假装通过：

```powershell
cmake --build C07_OS_Memory_System_IO/build/readiness-author --config Debug --target L07_readiness_student
C07_OS_Memory_System_IO/build/readiness-author/Debug/L07_readiness_student.exe
```

Linux 可在 WSL 内对同一目录运行：

```bash
cmake -S /mnt/f/CPPTrain/LearnCPP/C07_OS_Memory_System_IO/exercises/L07_readiness \
      -B /mnt/f/CPPTrain/LearnCPP/C07_OS_Memory_System_IO/build/readiness-linux \
      -DCMAKE_BUILD_TYPE=Debug
cmake --build /mnt/f/CPPTrain/LearnCPP/C07_OS_Memory_System_IO/build/readiness-linux
ctest --test-dir /mnt/f/CPPTrain/LearnCPP/C07_OS_Memory_System_IO/build/readiness-linux --output-on-failure
```

## 完整解析

Reference 实现用 64 字节块读取。每轮先根据剩余预算缩小请求大小，避免一次读取超过调用者授权；读到正数就追加并继续，读到 0 就设置 EOF，读到 would-block 就设置 `would_block`。循环自然表达“readiness 处理器必须 drain”的机制。

Good 实现故意用 1 字节读取，算法独立但状态机相同。它证明通过检查不是因为和 Reference 共享了块大小或内部策略，而是因为遵守了相同外部契约。

Bad 实现只做一次 4 字节读取。它在空端点、短数据、EOF 场景看似合理，但一旦 payload 超过 4 字节，就把仍在 socket 缓冲区里的数据留给下一轮。对 level-triggered poll，这通常只是低效；对 edge-triggered epoll，这会直接造成“还有数据但没有新事件”的停滞。检查器用真实 socket payload 抓住这一点。`validation/scripted_bad` 还会真实 `recv` 并丢弃数据，再按旧固定调用序返回字符串；运行时 payload 会拒绝这种伪实现。

