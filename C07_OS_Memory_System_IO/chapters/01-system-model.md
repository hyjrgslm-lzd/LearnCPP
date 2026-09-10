# 01 系统模型：进程、内核与可观察边界

## 为什么 C++ 程序需要操作系统模型

C++ 对象模型回答“这个对象何时存在、谁能访问它、析构时发生什么”。系统编程还要回答另一组问题：这个对象背后是否持有内核资源，系统调用是否已经接受请求，失败码属于哪一次操作，另一个进程能否看见同一份状态。

最容易出错的地方，是把这两组问题混成一个。`std::filesystem::path` 不是文件，`std::vector<std::byte>` 不是 I/O 请求，`int fd` 和 `HANDLE` 也不是普通整数。它们都可以放在 C++ 对象里，但真正的文件位置、引用计数、访问权限、阻塞等待和完成通知由内核维护。C++ 析构只能在某个时刻调用释放函数，不能改变平台对已提交操作的生命周期规则。

本课使用同一个判断顺序：

1. C++ 层：对象是否存在，是否独占，是否还有借用者。
2. 资源层：fd/HANDLE 是否有效，是否有重复引用，关闭后编号或句柄值能否复用。
3. 请求层：系统调用是否已经接受，后续会同步返回、等待就绪，还是异步完成。
4. 证据层：这次运行的输出证明了什么，哪些只是当前平台和输入下的观察。

这比背 API 名更重要。API 名会随平台改变，但“所有权、请求、完成、证据”这四层会贯穿文件、映射、IPC、动态装载和异步 I/O。

最小观察入口在 L01：

```powershell
cmake -S C07_OS_Memory_System_IO/exercises -B C07_OS_Memory_System_IO/build/sample-author-msvc-core -G "Visual Studio 18 2026" -A x64
cmake --build C07_OS_Memory_System_IO/build/sample-author-msvc-core --config Debug --target L01_handles_ownership
ctest --test-dir C07_OS_Memory_System_IO/build/sample-author-msvc-core -C Debug -R L01_handles_ownership --output-on-failure
```

这个 observation 不要求你实现 Student。它只观察 move、release、reset 后资源责任如何转移；如果这一步都说不清，后面的映射和 completion 会更难判断。

## 用户态、内核态与系统调用

普通 C++ 代码运行在用户态。它可以读写本进程地址空间、调用库函数、构造对象，却不能直接修改文件系统、调度线程或改变页表。需要这些能力时，它通过系统调用进入内核。Windows 的 `CreateFileW`、`ReadFile`、`WriteFile`，Linux 的 `open`、`read`、`write`、`mmap`、`epoll_wait` 和 `io_uring_enter` 都在这条边界上。

系统调用不是普通函数调用的另一个名字。跨过边界后，内核会检查句柄、权限、地址、长度、当前状态和资源限制。它可能立即完成，也可能阻塞当前线程，也可能只登记请求并稍后通知完成。返回值只能说明这次调用在平台契约下的结果，不能自动证明更强的事。例如写入返回成功通常说明字节被内核接受，不说明数据已经经受断电保存；取消请求返回成功说明取消请求被接受，不说明目标请求已经从完成队列里消失。

系统调用失败码要立即捕获。Linux 的 `errno` 和 Windows 的 `GetLastError()` 都是线程局部的“最近错误”状态，下一次库调用或系统调用可能覆盖它。公共封装应把失败立即转成 `std::error_code`，再返回给调用者；不要先打印、清理、再回头读取错误。

## 进程、线程与资源表

进程是资源容器：地址空间、打开的文件描述符表或句柄表、环境、当前目录、动态装载模块等都挂在进程上。线程是执行流：它共享进程的大部分资源，但有自己的栈、寄存器、线程局部状态和调度位置。

这解释了两个常见现象。第一，一个线程关闭 fd/HANDLE，会影响同进程其他线程后续使用同一资源编号；如果另一个线程已经在阻塞 I/O 中，平台还可能让那个 I/O 持有底层对象引用直到完成。第二，子进程继承资源不是“C++ 对象被复制”。Windows 需要显式可继承句柄与创建参数，Linux `fork` 会复制描述符表项，而 `exec` 后是否继续持有取决于 close-on-exec 标志。后面的进程和 IPC 单元会单独验证这些规则。

资源编号可以复用。一个 fd 关闭后，下一次 `open` 可能拿到同一个整数；一个 HANDLE 关闭后，旧值也不能再当作身份使用。因此“我手上还保存着旧编号”不是有效所有权证据。C++ RAII 类型的责任是让一个对象在任意时刻最多负责释放一次资源，并且移动后源对象不再释放。

关键代码形状如下，真实实现见 `c07::unique_fd` 和 `c07::unique_handle`：

```cpp
unique_file a = open_existing_file(path).value();
unique_file b = std::move(a);      // a 不再负责 close
auto raw = b.release();            // b 也不再负责 close
unique_file c{raw};
c.reset();                         // 释放一次
c.reset();                         // 空对象，无第二次释放
```

这里检查的是所有权状态，不是文件内容。文件内容由 L01/L02 的 checker 验证。


## 最小线程观察：同一地址空间，不同执行实体

L01 的 `L01_thread_process` observation 只观察进程和线程最基础的 OS 边界，不展开 C08 的同步课程。程序先预备两个样本槽，再启动两个 `std::jthread`。每个线程把 native process ID、native thread ID、全局对象地址和 `thread_local` 对象地址写入自己的槽位。

```cpp
std::array<sample, 2> samples{};
std::latch ready(2);
std::latch release(1);
std::array<std::jthread, 2> threads;

threads[0] = std::jthread(capture, std::ref(samples[0]), std::ref(ready), std::ref(release));
threads[1] = std::jthread(capture, std::ref(samples[1]), std::ref(ready), std::ref(release));
ready.wait();
// main thread reads both slots here
release.count_down();
```

`ready` 的作用是让主线程只在两个槽位都写完以后再读；`release` 的作用是让两个 worker 在线程局部对象仍然存活时停住，避免主线程比较到已经结束线程的 TLS 地址。创建第二个线程如果抛异常，代码会先 `release.count_down()`，让已经启动的第一个线程退出，再交给 `jthread` 析构 join；这样失败路径不会把第一个线程困在 latch 上。

Windows 观察使用 `GetCurrentProcessId()` 和 `GetCurrentThreadId()`；Linux 观察使用 `getpid()` 和 `syscall(SYS_gettid)`。运行结果应满足：两个线程的 process ID 相同，全局对象地址相同；native thread ID 不同，TLS 地址不同。这只能证明“同一进程共享地址空间、线程是不同执行实体”这个边界，不证明锁、条件变量、内存序或线程池设计；这些内容留给 C08。

## 阻塞、就绪与完成

同步阻塞 I/O 的模型最直接：调用 `read` 或 `ReadFile`，当前线程等到有结果或错误。这个模型适合建立正确性基线，因为调用返回时缓冲区借用结束，下一步可以立即检查结果。

非阻塞和 readiness 改变的是“何时尝试不会阻塞”。`select`、`poll`、`epoll` 等机制告诉你某类操作现在可能推进；它不替你完成读写，也不保证一次读写拿完全部数据。边沿触发、一次性触发、对端关闭和背压都属于 readiness 章节的核心。

completion 改变的是“请求已经提交，完成以后通知你”。Windows IOCP 和 Linux io_uring 都属于这种方向，但契约不同。共同点是：提交成功后，请求关联的缓冲区、OVERLAPPED/SQE 相关上下文、request_id 和结果槽必须活到最终完成被取走。取消只能进入这个状态机，不能跳过完成收束。

## 本章对应的练习边界

L01 只处理独占创建、写入和拒绝覆盖，目的是把资源所有权和错误传播讲清楚。L02 在 L01 之上加入分块读写、短 I/O 和 EOF，仍保持同步模型。L08 再回到“请求已接受以后谁持有上下文”的问题；它不应把完成式 I/O 简化成同步函数返回。

本章不要求读者记住全部平台 API。先能画出进程、线程、资源、请求和完成之间的关系，再看具体 API，错误会少很多。


