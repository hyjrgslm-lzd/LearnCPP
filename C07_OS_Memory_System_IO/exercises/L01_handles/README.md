# L01 handles：独占创建与句柄所有权

先读 [01 系统模型](../../chapters/01-system-model.md) 和 [02 句柄与错误](../../chapters/02-handles-errors.md)。本题只做一件事：实现 `create_note(path, payload)`，在新文件中写入 payload，并拒绝覆盖已有文件。

## Part 1：只创建新文件

编辑 [src/student/handles.hpp](src/student/handles.hpp)。函数返回 `std::expected<std::size_t, std::error_code>`。成功时返回写入字节数；失败时返回系统错误。目标路径已存在时必须失败，并保持原内容。

Reference 在 [src/reference/handles.hpp](src/reference/handles.hpp)，使用 `c07::create_new_file`。Windows 对应 `CREATE_NEW`，Linux 对应 `O_CREAT | O_EXCL`。不要先 `exists()` 再普通创建；那会把原子性留给竞态窗口。

## Part 2：写入全部字节

payload 是 `std::span<const char>`，不是以 `'\0'` 结尾的 C 字符串。实现要按长度写入，不靠 `strlen`。公共 helper `c07::bytes_of` 只做视图转换，不拥有数据；同步写入返回前借用结束。

Reference 使用 `c07::write_all`。它处理短写，失败时保留错误码。Student 不能用打印成功、返回 payload size 或预填输出绕过真实文件内容检查。

## Part 3：资源只释放一次

使用 `c07::unique_file` 或等价 move-only RAII。不要复制 fd/HANDLE 到两个所有者里。移动后源对象必须不再释放资源。析构不抛异常；需要业务级持久化诊断时应在析构前显式处理，不在本题里扩展。

Linux `close` 不按 `EINTR` 重试。man7 `close(2)` 说明错误返回后 fd 编号可能已经释放，重试可能关闭复用的新 fd。这和 `read/write` 的 `EINTR` 重试规则不同。

## 检查与解析

checker 在 [checks/handles_checks.cpp](checks/handles_checks.cpp)。它创建临时路径，检查：

- 新文件创建成功。
- 返回字节数等于 payload 长度。
- 文件内容逐字节等于 payload。
- 已有文件被拒绝，原 sentinel 内容保留。

[validation/good](validation/good/handles.hpp) 使用独立字节缓冲写法通过同一 checker。[validation/bad](validation/bad/handles.hpp) 用 `ofstream` 截断已有文件，必须被 `existing target is refused` 拒绝。Student 初始实现返回 `function_not_supported`，应明确失败。

本题通过不证明断电持久性，也不证明任意权限错误都可恢复。它只证明独占创建、短写循环入口和错误传播契约接线正确。
[checks/thread_process_observation.cpp](checks/thread_process_observation.cpp) 是另一个独立观察入口，不改变学生任务。它同时保留两个 `std::jthread`，用 `ready` latch 等两个线程都写完样本，再由主线程读取；用 `release` latch 让 TLS 对象在比较期间仍然存活。观察点很窄：同一进程内两个线程有相同 process ID 和相同全局对象地址，但有不同 native thread ID 和不同 TLS 地址。第二个线程创建失败时会先释放 `release` latch，让第一个线程能退出并被 `jthread` join，避免主线程卡在 ready 等待上。C08 会继续讲同步结构和线程生命周期，本观察只给 C07 的 OS 模型建立最小证据。

[checks/ownership_observation.cpp](checks/ownership_observation.cpp) 是独立观察入口，不改变学生任务。它打开一个临时文件，执行 move、release、重新绑定和重复 reset，确认 move 后源对象为空、release 后 wrapper 不再释放、重复 reset 不保留或重复关闭文件。这个观察只证明 RAII 状态转换，不证明所有平台关闭错误都可恢复。

## 单题构建

从 `C07_OS_Memory_System_IO/exercises` 执行：

```powershell
cmake -S L01_handles -B build/L01_handles-msvc -G "Visual Studio 18 2026" -A x64
cmake --build build/L01_handles-msvc --config Debug
ctest --test-dir build/L01_handles-msvc -C Debug --output-on-failure
ctest --test-dir build/L01_handles-msvc -C Debug -R L01_thread_process --output-on-failure
```

启用 Student baseline：

```powershell
cmake -S L01_handles -B build/L01_handles-student -G "Visual Studio 18 2026" -A x64 -DC07_STUDY_TEST_STUDENTS=ON
cmake --build build/L01_handles-student --config Debug --target L01_handles_student
ctest --test-dir build/L01_handles-student -C Debug -R student --output-on-failure
```


