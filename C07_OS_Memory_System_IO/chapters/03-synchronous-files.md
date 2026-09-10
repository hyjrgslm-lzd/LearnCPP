# 03 同步文件 I/O：短读写、EOF 与完整复制

## 正确基线先于异步和映射

文件处理课程容易直接跳到 mmap、epoll、IOCP 或 io_uring。那会遮住最基本的问题：你是否已经能把一个同步文件复制写正确。同步模型里，请求返回时缓冲区不再被内核借用，控制流也容易观察；它适合做所有后续方案的 correctness oracle。

L02 的 `copy_file_exact(source, target, chunk_size)` 建立这个基线。契约是：源文件存在，目标必须是新文件，chunk size 大于 0；函数按块复制全部字节，返回复制总数。空文件是成功，返回 0。目标已存在要拒绝，不能覆盖。

## short read 和 short write

一次 `read` 返回少于请求大小，不一定是错误。可能到了 EOF，可能当前设备或管道只提供了部分数据，也可能信号打断后需要重试。普通文件常常给出较大连续块，但课程不能把“这次普通文件读满了”写成接口保证。

`read_some` 因此只承诺“一次读取返回本次拿到的字节数或错误”。调用者看到 0 才把它解释为 EOF。L02 的循环每次只写实际读到的 `n` 字节，尾段不会把旧缓冲内容写出去。

核心循环只有这一层：

```cpp
std::uintmax_t copied = 0;
for (;;) {
    auto got = c07::read_some(in.get(), buffer);
    if (!got) return std::unexpected(got.error());
    if (*got == 0) return copied;       // EOF
    auto written = c07::write_all(out.get(), {buffer.data(), *got});
    if (!written) return std::unexpected(written.error());
    copied += *got;
}
```

错误实现常见写法是“读一块，写一块，返回”。L02 的输入长度故意大于 chunk，并在尾部放可识别字符串，保证这种实现被拒绝。

写入更不能假定一次完成。`write_all` 在 Linux 和 Windows 都循环到全部字节写完，或返回错误。Linux 侧 `EINTR` 重试，其他错误立即返回。普通文件上 0 字节写入进展异常，封装把它当 I/O 错误，避免无限循环。

## EINTR、SIGPIPE 与平台边界

Linux 的 `read`/`write` 可能因信号返回 `EINTR`。这表示本次调用没有形成可用结果，通常可重试。它和 `close` 的规则不同；`close` 失败后 fd 编号可能已经释放，不能盲目循环。

`write_all` 没有在公共 helper 里修改 `SIGPIPE` 进程策略。向已关闭 pipe/socket 写入时，Linux 默认可能先触发 `SIGPIPE` 终止进程，而不是只返回 `EPIPE`。网络和 pipe 单元会在自己的实验里控制信号或使用对应 flag；同步文件复制 helper 不应改全局 signal handler 来让一个局部练习更方便。

Windows 的 `ReadFile`/`WriteFile` 同步路径通过 BOOL 和 `GetLastError()` 报告错误，短写仍需按实际 `transferred` 推进。长度参数是 `DWORD`，公共 helper 每轮限制到 32 位可表达大小。

## EOF 不是错误

EOF 是输入结束状态。把 EOF 当错误会让空文件无法复制；把错误当 EOF 会截断文件却返回成功。L02 checker 同时检查非空尾段和空文件，防止这两类混淆。

另一个边界是目标文件。L02 使用新建目标，而不是截断已有目标。这样练习能明确区分“复制成功”和“覆盖了不该覆盖的数据”。bad 变体只读写第一块，并且会截断目标；checker 用大于 chunk 的输入和已有目标拒绝它。

## 偏移、文件位置与 positioned I/O

本章的同步复制使用顺序文件位置：每次 read/write 成功后，内核维护的位置向前移动。复制出来的字节顺序来自这个状态。后续映射、并发和异步单元会引入 positioned I/O：请求显式带 offset，多个请求不共享隐式文件位置。

顺序位置不是线程安全协议。多个线程或进程共享同一个 open file description 时，谁先推进位置会影响后续读写。课程后面会把“共享描述”和“重复 fd/HANDLE”分开验证。L02 暂时保持单线程、单输入、单输出，减少无关变量。

## 验证边界

文件位置也能由 `pread/pwrite` 或 Windows 的显式 `OVERLAPPED` offset 表达，避免让多个操作竞争隐式位置；P1 的完成式后端实际使用每个请求的 offset。学过 [09 进程](09-processes.md) 后，再回访 [10 IPC 的字节锁](10-ipc.md) 与 [L06_file_locking](../exercises/L06_process_ipc/README.md)：该观察同时验证锁冲突/解锁，以及 Linux positioned I/O 不推进共享 file offset。记录锁、映射可见性和持久性各有独立契约，不能把“已加锁”解释成一切访问都会被阻止。

L02 的 Reference 使用 `c07::open_existing_file`、`create_new_file`、`read_some` 和 `write_all`。good 使用标准流和独立循环，证明 checker 没绑定 Reference 实现。bad 只复制第一块，必须被“copy reports every byte”拒绝。

直接运行：

```powershell
cmake --build C07_OS_Memory_System_IO/build/sample-author-msvc-core --config Debug --target L02_sync_io_reference L02_sync_io_validation_good L02_sync_io_validation_bad
ctest --test-dir C07_OS_Memory_System_IO/build/sample-author-msvc-core -C Debug -R "L02_sync_io" --output-on-failure
```

通过 L02 证明：给定本地临时文件、有限输入和当前平台同步 API，复制循环处理了 EOF、尾段、目标存在和零 chunk。它不证明断电持久性，不证明网络 socket 的 backpressure，也不证明映射或异步路径更快。那些结论需要各自的机制和证据。
