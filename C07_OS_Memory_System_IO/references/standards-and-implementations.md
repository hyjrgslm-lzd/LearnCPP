# 规范、平台与实现索引

核对日期：2026-09-10。C++ 语言/标准库要求、操作系统接口契约、上游库版本与本次运行结果是四种信息。本课核心使用 C++23，pmr 起于 C++17；不把 Win32、POSIX/Linux 或 liburing 包装成标准 C++ I/O。

## 来源及其责任

| 来源 | 阅读问题与边界 |
|---|---|
| [C++23 N4950](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/n4950.pdf) 的 allocator/memory_resource 与对象生命期条款 | 对齐、构造/销毁、资源相等和传播属于语言/库层，不保证物理页放置或跨进程同步 |
| [Microsoft VirtualAlloc](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualalloc) | reserve/commit 的含义；分配粒度与页大小不同；提交不等于页面已在工作集 |
| [MapViewOfFile](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-mapviewoffile) | mapping object 与 view 分开，偏移对齐、访问权限和 view 生命周期 |
| [mmap](https://man7.org/linux/man-pages/man2/mmap.2.html)、[msync](https://man7.org/linux/man-pages/man2/msync.2.html) | MAP_SHARED/MAP_PRIVATE、长度/偏移、回写边界；不同平台不靠名字相似推定语义相等 |
| [read](https://man7.org/linux/man-pages/man2/read.2.html)、[write](https://man7.org/linux/man-pages/man2/write.2.html)、[close](https://man7.org/linux/man-pages/man2/close.2.html) | 短读写与 EINTR；Linux close 的 fd 生命周期不可照搬 read/write 重试循环 |
| [epoll](https://man7.org/linux/man-pages/man7/epoll.7.html)、[epoll_ctl](https://man7.org/linux/man-pages/man2/epoll_ctl.2.html) | readiness、ET 排空与 oneshot 重装；普通文件注册并非支持路径 |
| [IOCP](https://learn.microsoft.com/en-us/windows/win32/fileio/i-o-completion-ports)、[同步与异步 I/O](https://learn.microsoft.com/en-us/windows/win32/fileio/synchronous-and-asynchronous-i-o) | API 返回与完成通知的关系，缓冲/OVERLAPPED 生命周期和失败 completion |
| [CancelIoEx](https://learn.microsoft.com/en-us/windows/win32/fileio/cancelioex-func) | 取消不等待最终完成；保留正常成功、取消和其他失败竞态 |
| [io_uring](https://man7.org/linux/man-pages/man7/io_uring.7.html)、[取消](https://man7.org/linux/man-pages/man7/io_uring_cancelation.7.html) | SQE/CQE、独立取消请求、目标收束、能力与执行错误分层 |
| [liburing 2.15](https://github.com/axboe/liburing/releases/tag/liburing-2.15) | 本课固定 commit d41bf9220ec39277ff235379e9089d9e0fd6c2a5，源码阅读与运行均绑定该输入 |
| [CreateProcessW](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessw)、[fork](https://man7.org/linux/man-pages/man2/fork.2.html)、[execve](https://man7.org/linux/man-pages/man2/execve.2.html)、[waitpid](https://man7.org/linux/man-pages/man2/waitpid.2.html) | 子进程身份、句柄继承、退出等待与多线程 fork 后限制 |
| [LoadLibraryExW](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-loadlibraryexw)、[dlopen](https://man7.org/linux/man-pages/man3/dlopen.3.html) | 搜索路径、符号、引用计数与卸载；C ABI 不自动提供跨模块异常/资源安全 |

这些是课程核对与进一步阅读入口，不能代替正文推导。Linux man-pages 的相关条目用于 Linux 行为，不能泛化成所有 POSIX 系统都同样实现。C++ 当前工作草案中的后续变化应另记版本，本课未因 C++29 规划视野就声称系统 I/O 已标准化。

## 本次实现与能力记录

Windows 的具体 SDK、MSVC 与 STL 版本，以及 Linux 的 GCC/libstdc++、kernel、glibc 和挂载类型都进入验证记录。源码导读引用本机标准库实际文件或固定上游源码，保存所读文件指纹；不能引用滚动 master 再把观察归到固定构建。

liburing 编译/链接通过，只证明用户态库入口成立。单独探测 ring 创建和所需 opcode，再运行主题；最小能力探测不执行整套主题。能力具备后的提交/完成错误归为 FAIL，不能被宽泛捕获改成 SKIP。选项 OFF 单列“未启用”。

Windows 正常成功、Linux 正常成功以及安全模型检查各有自己的证据项。WSL 的系统调用行为与文件实验可以实测，但不能据此宣称裸机设备吞吐、跨主机文件系统或真实掉电恢复已验证。
