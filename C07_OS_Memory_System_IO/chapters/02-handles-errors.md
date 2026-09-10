# 02 句柄、描述符与错误传播

## 句柄不是普通值

Linux 文件描述符是小整数，Windows `HANDLE` 是不透明值。它们都可以复制，但复制出来的 C++ 值不等于复制了所有权。两个 RAII 对象如果持有同一个 fd 或 HANDLE，并在析构时都释放它，第二次释放面对的可能已经不是原来的资源。

所以本课公共头 `exercises/include/c07/os.hpp` 只提供 move-only 的 `unique_fd`、`unique_handle` 和条件别名 `unique_file`。它们允许移动，不允许复制。移动后源对象进入无效状态，析构不再释放；目标对象成为唯一释放者。这是 C02 RAII 在系统资源上的直接应用。

invalid sentinel 也不能混用。Linux fd 的无效值是 `-1`；Windows 文件 API 常以 `INVALID_HANDLE_VALUE` 表示失败，但部分 Win32 API 也会使用 `nullptr` 作为无效 HANDLE。`unique_handle::valid()` 同时拒绝这两类无效值，避免一个封装只能适配一半 API。

## 创建、打开与拒绝覆盖

L01 的 `create_note(path, payload)` 故意使用“只创建新文件”的语义：如果目标已经存在，函数应失败，并保持旧内容不变。这个契约比“能写出一个文件”强。它防止课程实验覆盖用户文件，也迫使实现者把资源创建模式纳入接口语义。

Windows 侧使用 `CREATE_NEW`；Linux 侧使用 `O_CREAT | O_EXCL`。这两个选择都让“目标已存在”成为系统调用层的原子判定，而不是先 `exists()` 再创建。先查再创在并发场景有竞态：检查之后、创建之前，另一个进程可能已经创建同名文件。

创建成功后写入 payload。写入失败应返回 `std::error_code`，不是在公共 API 里 `exit()`。检查器失败可以退出进程，因为它是在给练习判定结果；课程公共 API 应把错误交回调用者，让上层决定是重试、报告、清理还是回滚。

## 错误码要马上取走

系统调用失败后，错误状态很脆弱。Linux 的 `errno` 和 Windows 的 `GetLastError()` 都可能被后续调用覆盖。正确封装模式是：

```cpp
auto file = c07::create_new_file(path);
if (!file) return std::unexpected(file.error());
```

在封装内部，失败点立即调用 `last_error_code()`。不要在失败和取错误之间插入日志、路径格式化或清理调用。日志可以在拿到 `std::error_code` 之后做。

公共封装的模式保持很小：

```cpp
auto file = c07::create_new_file(path);
if (!file) return std::unexpected(file.error());

auto written = c07::write_all(file->get(), c07::bytes_of(payload));
if (!written) return std::unexpected(written.error());
return payload.size();
```

`check()` 只出现在 checker 里，不进入 `create_note`。业务 API 返回错误，测试进程负责把错误解释成 PASS/FAIL。

错误码也不是异常安全的替代品。对象构造、容器分配、路径转换仍可能抛异常。练习输入控制在小范围内，重点检查系统错误传播；更完整的异常回滚在后续映射、pmr 和综合项目里展开。

## close/CloseHandle 的边界

释放函数自身也可能失败。课程的 RAII 析构不抛异常，只负责尽力释放；需要持久化或错误诊断的代码应在析构前显式执行 `flush/fsync` 或平台对应检查，并处理返回值。析构期再报告错误通常太晚，调用者已经不知道如何恢复。

Linux `close(2)` 有一个容易误学的点：不要像 `read`/`write` 那样在 `EINTR` 后盲目重试 `close`。man7 `close(2)` 说明，Linux 会在 close 操作早期释放文件描述符编号，后续刷新等步骤才可能报错；重试可能关闭另一个线程刚复用到同一编号的新资源。`unique_fd::reset()` 因此只调用一次 `close`。这不是忽略错误，而是 RAII 析构没有安全恢复通道；需要诊断时应在业务层显式处理。

Windows `CloseHandle` 也不让旧 HANDLE 继续代表资源身份。关闭后的值只能被当作无效状态保存，不能再比较、等待或传给 I/O API。

## L01 的有效检查

L01 checker 覆盖三类条件：新路径成功创建、写入字节数与内容完全一致、已有文件被拒绝且原内容保留。Reference 用公共 `create_new_file` 和 `write_all`，good 用独立的字节转换路径，bad 用 `ofstream` 的截断写入模拟常见错误。bad 必须被“existing target is refused”拒绝。

直接运行：

```powershell
cmake --build C07_OS_Memory_System_IO/build/sample-author-msvc-core --config Debug --target L01_handles_reference L01_handles_validation_good L01_handles_validation_bad L01_handles_ownership
ctest --test-dir C07_OS_Memory_System_IO/build/sample-author-msvc-core -C Debug -R "L01_handles" --output-on-failure
```

这组检查不能证明任意磁盘持久性，也不证明所有权限错误都能恢复。它证明的是：在有限临时文件输入下，接口把独占创建、所有权释放和错误传播接上了。
