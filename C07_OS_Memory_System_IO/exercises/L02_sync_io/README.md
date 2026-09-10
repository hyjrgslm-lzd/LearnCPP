# L02 sync_io：同步分块复制与 EOF

先读 [03 同步文件 I/O](../../chapters/03-synchronous-files.md)，并完成 [L01](../L01_handles/README.md)。本题实现 `copy_file_exact(source, target, chunk_size)`：按块复制一个文件，拒绝覆盖目标，返回实际复制字节数。

## Part 1：建立输入输出资源

编辑 [src/student/sync_io.hpp](src/student/sync_io.hpp)。源文件必须已存在；目标文件必须是新文件。`chunk_size == 0` 是无效参数，直接返回错误。不要为了省事使用会截断目标的打开模式。

Reference 使用 `c07::open_existing_file` 和 `c07::create_new_file`，让目标存在由系统调用原子拒绝。good 使用标准流和独立循环，证明 checker 不依赖 Reference 的内部写法。

## Part 2：读多少写多少

循环调用 `read_some`。返回错误就传播；返回 0 是 EOF；返回正数时只写这一段有效字节。最后一块通常短于 buffer，不能把旧 buffer 内容写出去。

`write_all` 负责短写循环。Student 若只调用一次 write，普通小文件可能偶然通过，但 pipe/socket 或平台短写会失败。本题 checker 用跨 chunk 的输入和尾段字符串拒绝“只复制第一块”的 bad。

## Part 3：边界输入

空文件复制应成功，返回 0，并创建空目标。这说明 EOF 是正常结束，不是错误。已有目标应失败并保留原内容。这说明复制函数不是通用覆盖工具，而是安全实验入口。

路径包含空格和中文字符，验证实现不能假设 ASCII 路径。checker 在 Windows 构造 wide path；Linux path 由 `std::filesystem::path` 转给 `open`。不要用窄 `std::string` 里的中文路径当作 Windows Unicode 证据，它会受当前 ACP 影响。

## 检查与解析

checker 在 [checks/sync_io_checks.cpp](checks/sync_io_checks.cpp)。它覆盖：

- 257 字节以上输入，chunk size 为 17，强制出现多个完整块和尾段。
- 返回字节数等于输入长度。
- 目标内容和源内容逐字节相同。
- 空文件成功复制。
- 目标已存在时拒绝覆盖。
- `chunk_size == 0` 被拒绝。

[validation/good](validation/good/sync_io.hpp) 先确认源文件可读，再用 C++23 `std::ios::noreplace` 原子拒绝覆盖目标，并在返回前显式 `flush()`/`close()` 检查流错误。[validation/bad](validation/bad/sync_io.hpp) 只复制第一块，并可能截断目标，必须被 `copy reports every byte` 拒绝。Student 初始实现返回 `function_not_supported`，应在 `copy succeeds` 处失败。

本题通过不证明网络 backpressure、异步 completion 或持久化。它建立后续 mapped/completion 文件处理的同步正确性基线。

## 单题构建

从 `C07_OS_Memory_System_IO/exercises` 执行：

```powershell
cmake -S L02_sync_io -B build/L02_sync_io-msvc -G "Visual Studio 18 2026" -A x64
cmake --build build/L02_sync_io-msvc --config Debug
ctest --test-dir build/L02_sync_io-msvc -C Debug --output-on-failure
```

启用 Student baseline：

```powershell
cmake -S L02_sync_io -B build/L02_sync_io-student -G "Visual Studio 18 2026" -A x64 -DC07_STUDY_TEST_STUDENTS=ON
cmake --build build/L02_sync_io-student --config Debug --target L02_sync_io_student
ctest --test-dir build/L02_sync_io-student -C Debug -R student --output-on-failure
```
