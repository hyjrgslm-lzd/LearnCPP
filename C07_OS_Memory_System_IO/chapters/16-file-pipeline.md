# 16 综合文件管线：从 native I/O 到有界组装

前面的章节分别看过同步文件、文件映射、IOCP 和 io_uring。综合项目把它们放进同一个资源文件处理器：源文件由后端分块读取，完成块带着 `request_id`、`offset` 和实际 `bytes` 回来；上层按区间组装，再写入一个新目标文件。

这里的重点不是“异步一定更快”。L02 的同步循环已经能正确处理 EOF、短读和安全创建目标文件。mapping 分支把内核页缓存映射进进程地址空间，减少显式 read 调用，但仍然要受页错误、映射粒度和生命周期约束。completion 分支把多个 read 请求同时交给内核，完成顺序由内核决定，所以应用层必须用 `offset` 还原文件布局。

公共接口固定在 `c07::read_file_chunks()` 和 `c07::write_new_file()`。读取后端支持 `buffered`、`mapped` 和 `completion`；每个请求拥有自己的 buffer，完成后移动进 `completed_chunk`。后端限制 `chunk_size` 为 1 到 1 MiB，`in_flight` 为 1 到 16，总文件不超过 64 MiB，chunk 数不超过 65536。空文件成功返回 0 个 chunk。

P1 的学生任务刻意窄：只实现 `c07_p1::assemble()`。这让练习集中在 completion 协议最容易写错的地方：

```cpp
std::expected<std::vector<std::byte>, std::error_code>
assemble(std::span<const c07::completed_chunk> chunks, std::size_t total_size);
```

一个完成块合法，不代表整批合法。整批必须满足这些条件：

- `total_size <= c07::max_pipeline_bytes`，`chunks.size() <= c07::max_pipeline_chunks`。
- `request_id` 非零且唯一。
- 非空文件中的 chunk payload 非空。
- `offset + bytes.size()` 不溢出，不越过 `total_size`。
- 所有区间刚好覆盖 `[0, total_size)`，没有重叠，也没有洞。
- `total_size == 0` 时只能有空 chunk 列表。

Reference 的实现先记录 chunk 索引，按 `offset` 排序，再线性检查每个区间是否从当前期望 offset 开始。全部验证通过后才分配结果 vector 并复制。good 变体用 coverage 位图独立检查每个字节位置，证明 checker 没有绑定 Reference 的排序写法。bad 变体按完成顺序 append，所以合法乱序会产出错误内容，重叠场景也会被拒绝。

真正的文件管线还有一个同步 sink：`write_new_file()` 只创建新目标，写完后 flush 并检查 close 结果。它证明用户态已经把字节交给了系统调用路径，但不承诺掉电持久性；那需要 fsync/FlushFileBuffers 级别的单独实验和成本说明。

资源边界同样重要。这个项目保留完整输入和已完成块，因此是有界 materialization，不是无限流处理框架。这样做换来教学清晰度：每个 backend 可以独立比较最终字节，checker 能稳定构造乱序、重叠、缺洞和取消边界。后续若要做真正流式处理，应把 `assemble()` 改成按 offset 推进的 sink，并重新设计背压、失败回滚和部分目标文件清理策略。

构建 P1：

```powershell
cmake -S C07_OS_Memory_System_IO/exercises/P1_file_pipeline -B build/pipeline-author -G "Visual Studio 18 2026" -A x64
cmake --build build/pipeline-author --config Debug
ctest --test-dir build/pipeline-author -C Debug --output-on-failure
```

启用 Student baseline：

```powershell
cmake -S C07_OS_Memory_System_IO/exercises/P1_file_pipeline -B build/pipeline-student -G "Visual Studio 18 2026" -A x64 -DC07_STUDY_TEST_STUDENTS=ON
cmake --build build/pipeline-student --config Debug --target P1_file_pipeline_student
ctest --test-dir build/pipeline-student -C Debug -R student --output-on-failure
```

如果 `c07/file_pipeline_io.hpp` 已存在，Reference/good 测试会额外跑真实 `buffered` 和 `mapped` 后端；Windows 或启用 liburing 的 Linux 环境还会注册 `completion` 后端。Linux 测试建议在 WSL 文件系统内构建，再把 records 导回课程目录，避免 `/mnt` 文件系统语义干扰 I/O 观察。
