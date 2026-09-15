# L04 就绪与完成驱动

正文：[Reactor/Proactor](../../chapters/04-reactor-proactor.md)。本单元是观察与修改题，提供完整可运行基线，没有假装未完成的 Student。

## Part A：公平 Reactor

阅读 [reactor.cpp](reactor.cpp)与 [reactor.hpp](../include/c11/reactor.hpp)。画出 listener、连接、decoder、write_queue 的拥有关系，先预测慢读者达到高水位时 interest 怎样变化，再运行 C11_L04_reactor。

```powershell
cmake -S C11_Networking_Service_Design/exercises/L04_native_io -B build/c11-l04
cmake --build build/c11-l04 --config Release
ctest --test-dir build/c11-l04 -C Release --output-on-failure
```

已提供慢读者状态通知、另一正常客户端和 RST 场景。任务是在独立副本中改变 dispatch 预算，继续满足精确回送、容量和 clean 判定；不要让故障发生概率决定作业结果。对应 Reference 就是原文件，保留原检查器。

## Part B：实际 IOCP

阅读 [iocp.cpp](iocp.cpp)，逐个标记 OVERLAPPED 的提交、pending、最终 packet 与可复用位置。运行 C11_L04_iocp，解释立即成功、WSA_IO_PENDING、取消成功、ERROR_NOT_FOUND 分别是否允许释放数组。

解析：前两项在本课默认通知模式下都等待 packet；取消结果只说明取消请求状态，仍等目标 packet。竞态的成功和 aborted 都合法，检查器不能要求每次相同调度。写操作用完成量移动 offset，短完成不表示错误。

## Part C：Linux 源码分支（本次未执行）

[epoll.cpp](epoll.cpp)对同一 24 KiB 字节输入比较 LT、ET 和 oneshot。ET 的 16 KiB 预算用尽后设置 runnable，不等待新边沿；oneshot 用 MOD 重装。练习在纸面移除 continuation，推导为何剩下 8 KiB 可能永久等待。Linux 配置会注册 C11_L04_epoll。

[uring.cpp](uring.cpp)是独立单次 socket recv/cancel，显式 C11_ENABLE_URING=ON 才查找 liburing 头和库。先提交 recv，再提交带另一 user_data 的 cancel，收齐两条 CQE。只对 ENOSYS/EPERM 返回能力 SKIP，其他错误保留失败。推荐按来源索引的 liburing2.15 在 Linux 原生文件系统准备；本次 Windows 不构建或执行此分支，不需要 WSL。

解析：取消 SQE 的 CQE 和目标 recv 的 CQE 是不同操作；缓冲在目标完成前不得复用。单次示例不覆盖 multishot、注册缓冲或 zero-copy 通知生命周期。
## IDE 工程入口

VS solution 中本题主入口是 `C11_L04_reactor`。本单元是观察/专项入口，没有学生占位；源码、README、协议文件或脚本显示在同一项目中，依赖目标保留为独立项目。程序通过只证明本驱动运行，不代替 README 要求的预测、解释或专项依赖准备。单题可用 `cmake -S <本目录> -B <build>` 独立生成。
