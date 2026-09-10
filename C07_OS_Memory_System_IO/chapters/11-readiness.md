# 11. Readiness：就绪通知不是完成通知

同步文件 I/O 的模型很直接：调用 `read` 或 `ReadFile`，线程进入内核，直到结果出现或者错误返回。完成式 I/O 的模型也很直接：先提交一个操作，之后从完成队列取回“这个操作已经结束”的事实。readiness 位在二者之间：它只告诉你“现在尝试某类操作可能不会阻塞”，不告诉你已经完成了多少业务动作，也不替你拥有缓冲区、请求对象或生命周期。

这个区别决定了本章所有代码的形状。一个 readiness 事件不是一条 payload，也不是一次完整请求。它只是让事件循环获得一次执行权：现在去 `recv`、`read`、`accept` 或 `write`，直到真实系统调用告诉你应该停。停下来的原因必须来自 I/O 本身，而不是来自通知 API 的返回值。

## 从 blocking 到 nonblocking

阻塞描述符最适合直线程序：没有数据就等待，有数据就返回。这对单连接工具很简单，对事件循环却很危险。一个 fd 上没有数据会卡住整条循环，使其他已经就绪的 fd 无法被处理。

非阻塞模式把“等待”从 I/O 调用里拿出来。描述符没有数据时，POSIX 返回 `-1` 并设置 `errno=EAGAIN` 或 `EWOULDBLOCK`；Winsock 返回 `SOCKET_ERROR`，`WSAGetLastError()` 是 `WSAEWOULDBLOCK`。这不是异常故障，而是状态机的一条边：当前缓冲区已经被排空，等下一次 readiness 通知。

本章练习的核心循环就是这条边界：

```cpp
std::expected<drain_result, std::error_code>
drain(native_endpoint endpoint, std::size_t max_bytes) {
    drain_result result;
    std::array<char, 64> buffer{};

    while (result.bytes.size() < max_bytes) {
        const auto want = std::min(buffer.size(), max_bytes - result.bytes.size());
        const auto n = recv(endpoint, buffer.data(), want, 0);
        if (n > 0) {
            result.bytes.append(buffer.data(), n);
            continue;
        }
        if (n == 0) {
            result.eof = true;
            break;
        }
        if (last_error_is_would_block()) {
            result.would_block = true;
            break;
        }
        return std::unexpected(last_error_code());
    }

    if (result.bytes.size() == max_bytes) result.budget_exhausted = true;
    return result;
}
```

这里的 `recv` 是真实判定点。`poll`、`WSAPoll`、`epoll_wait` 只能告诉你“值得试一下”。从返回事件到进入处理函数之间，状态可能已经变化；即使没有竞争，内核也没有承诺一次读取能拿完所有数据。

## select、poll 与 Windows WSAPoll

`select` 和 `poll` 都是 level-triggered 思路：只要 fd 仍满足条件，再次等待仍可能返回它。`select` 用固定大小的 fd 集合，受 `FD_SETSIZE` 和最高 fd 扫描影响；`poll` 用数组表达 fd 和事件，避免了 `select` 的位图限制，但每次调用仍要把整个观察集合交给内核。

Windows 的 `WSAPoll` 属于 Winsock socket API。它可以表达本章需要的“本地 socket 当前是否可读”，但它不是 epoll，也不提供 edge-triggered 或 oneshot 语义。课程代码在 Windows 上只做 Winsock readiness：本机 loopback 建连接，`ioctlsocket(FIONBIO)` 设置 server 端非阻塞，`WSAPoll` 观察读事件，然后用学生的 `drain` 读取真实 payload。

最小观察入口是 L07 checker 自己。多片测试先发送一串超过 bad 实现单次读取容量的数据，再确认 `WSAPoll`/`poll` 报告可读，最后调用 `drain`：

```cpp
send_all(writer, "first fragment + second fragment drained + tail");
check(poll_readable(reader, 1000), "readiness reports queued data before drain");
expect_drain(reader, 128, data, true, false, false, "second fragment drained");
check(!poll_readable(reader, 0), "drained socket has no remaining level-triggered readiness");
```

实际 checker 不使用固定字符串；它每次运行生成未知 payload，并打印 `L07 readiness seed=...`。这段代码只展示结构。真实断言比较本轮写入的完整字节，所以“丢弃真实输入再按调用序返回 one/two/last/012345...”的伪实现也会被 negative fixture 拒绝。

## Linux epoll：LT、ET 与 ONESHOT

`epoll` 把观察集合留在内核里，用户态用 `epoll_ctl` 增删改注册，再用 `epoll_wait` 取事件。默认的 `EPOLLIN` 是 level-triggered：只要缓冲区仍可读，后续等待还能看到它。`EPOLLET` 是 edge-triggered：内核在状态变化边沿报告事件，而不是在“仍然可读”这个状态上反复提醒。

这就是 ET 代码必须排空到 `EAGAIN` 的原因。一个可重复反例很小：写入 6 字节，只读 1 字节，然后立刻用 0 超时再等一次。`epoll_wait` 不再返回事件，但直接 `read` 还能读出剩下 5 字节。

```cpp
epoll_ctl(ep, EPOLL_CTL_ADD, read_end, EPOLLIN | EPOLLET);
write(write_end, "abcdef", 6);
check(epoll_wait(ep, &event, 1, 1000) == 1, "first transition");
read(read_end, &one, 1);
check(epoll_wait(ep, &event, 1, 0) == 0, "no repeated ET event");
check(read(read_end, rest, sizeof rest) == 5, "bytes still existed");
```

这个反例没有 sleep，也不会无限挂；所有等待都有明确超时。它证明 ET 的失败模式不是“读不到数据”，而是事件循环失去再次进入处理器的机会。

`EPOLLONESHOT` 解决的是另一个问题：同一个 fd 在并发事件循环中不能被多个 worker 同时处理。oneshot 事件返回一次后，这个注册会被禁用。处理器排空到 `EAGAIN`，更新自己的状态，然后用 `epoll_ctl(EPOLL_CTL_MOD, ...)` 重新武装。即使本章不写线程池，也要理解重装顺序：先把当前可读数据消费到稳定边界，再 rearm；否则可能把旧状态和新事件混在一起。

## regular file 的边界

普通文件通常不会像 socket 或 pipe 那样“等待对端生产数据”。它们在概念上经常是可读的：当前位置有数据就返回数据，到末尾就返回 EOF。这个性质不等于“可以作为 epoll 事件源”。Linux 练习会创建一个真实临时 regular file，然后调用 `epoll_ctl(EPOLL_CTL_ADD, file, EPOLLIN)`，期望看到 `EPERM`。

这个观察用于划清模型边界。readiness API 不是统一文件抽象；它服务于会随外部事件变化的内核对象，例如 socket、pipe、eventfd 等。同步文件复制仍应按上一章的短读、短写、EOF 和错误处理来写，不要因为学了 epoll 就把普通文件强行塞进事件循环。

## EOF、EAGAIN 与背压

`EAGAIN` 是“现在没法继续”，EOF 是“对端不再发送”。如果 socket 中先有数据、随后对端半关闭，正确结果是同一轮返回这些数据并设置 EOF。只返回 EOF 会丢数据；读到数据就立刻返回则可能漏掉关闭状态，让上层多等一轮。

写方向也有 readiness。非阻塞 `send` 在发送缓冲区满时返回 would-block，这就是背压。背压不是错误；它要求上层暂停生产或注册写就绪。L07 checker 把 writer 端切到非阻塞，有限填充本地 socket 缓冲区，要求在有界循环内观察到 would-block，再调用 `drain` 释放一部分数据，并验证读回内容是本轮写入序列的正确前缀。这个测试不改变全局 signal 策略；POSIX 发送使用 `MSG_NOSIGNAL`，Windows 使用 Winsock 错误码。

## 练习如何验证机制

本章练习不是让学生写 `poll` 封装，也不是做跨平台 reactor。学生只实现一个 drain 状态机；平台驱动由 checker 提供。这样分层有两个目的：一是把可验证任务压缩到一个清晰函数，二是仍然用真实 OS 对象暴露 readiness 的失败模式。

Reference 用 64 字节块循环读取。Good 用 1 字节循环读取。两者策略不同，但都遵守同一契约：正数字节继续，would-block 停，EOF 停，预算用尽停，其他错误传播。Bad 只读一次，它在短 payload 下可能看起来正确，但在多片 payload 和 ET 模型下会留下未消费数据。另一个 scripted bad 会真实读取并丢弃 OS 数据，再按旧固定字符串回报；运行时 payload 断言会拒绝它，防止 checker 只验证调用脚本而不验证真实字节。

最小复现命令：

```powershell
cmake -S C07_OS_Memory_System_IO/exercises/L07_readiness -B C07_OS_Memory_System_IO/build/readiness-author -G "Visual Studio 17 2022" -A x64
cmake --build C07_OS_Memory_System_IO/build/readiness-author --config Debug
ctest --test-dir C07_OS_Memory_System_IO/build/readiness-author -C Debug --output-on-failure
```

Linux 上使用同一题目录：

```bash
cmake -S /mnt/f/CPPTrain/LearnCPP/C07_OS_Memory_System_IO/exercises/L07_readiness \
      -B /mnt/f/CPPTrain/LearnCPP/C07_OS_Memory_System_IO/build/readiness-linux \
      -DCMAKE_BUILD_TYPE=Debug
cmake --build /mnt/f/CPPTrain/LearnCPP/C07_OS_Memory_System_IO/build/readiness-linux
ctest --test-dir /mnt/f/CPPTrain/LearnCPP/C07_OS_Memory_System_IO/build/readiness-linux --output-on-failure
```

通过这些测试只能证明本章这些边界：非阻塞 drain、真实本地 socket readiness、Linux epoll ET/ONESHOT/EPERM 观察。它不声称实现了完整网络协议、线程池、跨平台统一 reactor，也不替代完成式 I/O 章节中的请求生命周期管理。

