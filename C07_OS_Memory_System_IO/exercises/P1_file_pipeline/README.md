# P1 file_pipeline：乱序完成块组装

先读 [03 同步文件 I/O](../../chapters/03-synchronous-files.md)、[05 文件映射](../../chapters/05-file-mapping.md)、[12 Windows IOCP](../../chapters/12-windows-iocp.md)、[13 Linux io_uring](../../chapters/13-linux-io-uring.md)，再读 [16 综合文件管线](../../chapters/16-file-pipeline.md)。

本题只实现 `c07_p1::assemble(chunks, total_size)`。读取文件、映射文件、completion 提交和写回目标文件由课程公共后端完成。学生代码只面对已经完成的 `c07::completed_chunk`：

```cpp
std::expected<std::vector<std::byte>, std::error_code>
assemble(std::span<const c07::completed_chunk> chunks, std::size_t total_size);
```

## Part 1：先验证边界

编辑 [src/student/pipeline.hpp](src/student/pipeline.hpp)。`total_size` 最大为 `c07::max_pipeline_bytes`，chunk 数最大为 `c07::max_pipeline_chunks`。空文件必须是 `total_size == 0 && chunks.empty()`，返回空 vector。非空文件中，每个完成块都必须有非零 `request_id` 和非空 `bytes`。

检查 `offset + bytes.size()` 时不要让 `size_t` 溢出。溢出、越过 `total_size`、重复 `request_id` 都是协议错误。

## Part 2：乱序按 offset 组装

completion 后端可以先完成后面的请求，所以不能按到达顺序追加。正确做法是先证明区间刚好覆盖 `[0, total_size)`：无重叠、无洞、无越界。Reference 用排序后的索引检查区间；good 用 coverage 位图独立验证。两者都在验证完成后才分配结果并复制。

这个顺序很重要：如果先写入结果，再发现后续块重叠或缺洞，调用方已经拿到了被部分提交的状态。这里的 `std::bad_alloc` 可以自然传播；本题不把内存耗尽包装成 `std::error_code`，因为课程重点是 I/O completion 的协议数据，而不是异常策略。

## Part 3：真实后端怎么接入

checker 会先跑 synthetic 场景：合法乱序、重复 id、重叠、缺洞、offset 溢出、空 payload、越界、超限、空文件。若公共 `c07/file_pipeline_io.hpp` 已存在，Reference/good 还会把真实 `read_file_chunks()` 读出的完成块喂给 `assemble()`，再用 `write_new_file()` 写新文件并回读比较。

默认平台测试覆盖 `buffered` 和 `mapped`。`completion` 只在 Windows 或 Linux `C07_STUDY_ENABLE_IO_URING=ON` 且 `c07_liburing` 可用时注册；真实能力缺失时才允许返回 77。普通 Student 和 bad 变体不允许 SKIP。

## Part 4：CLI

Reference 构建会生成 `P1_file_pipeline_demo`：

```powershell
cmake -S C07_OS_Memory_System_IO/exercises/P1_file_pipeline -B build/pipeline-author -G "Visual Studio 18 2026" -A x64
cmake --build build/pipeline-author --config Debug
ctest --test-dir build/pipeline-author -C Debug --output-on-failure
```

示例：

```powershell
.\build\pipeline-author\Debug\P1_file_pipeline_demo.exe --input .\input.bin --output .\output.bin --backend mapped --chunk 65536 --inflight 4
```

参数非法返回 2；输出路径已存在也返回 2。Windows 入口使用 `wmain`，路径经 `std::filesystem::path` 保留 Unicode。

本项目验证的是同步 sink 和有界 materialization 的正确性。它不预设 mapped 或 completion 更快；不同后端的开销要用后续独立 benchmark 归因。
