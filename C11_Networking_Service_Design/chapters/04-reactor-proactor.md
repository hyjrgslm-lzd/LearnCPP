# 04 从等待模型到公平事件循环

先读第 3 章，复习 C07 的 readiness/completion。这里的“异步”首先是操作与调用栈分离，和是否采用协程语法无关。[L04](../exercises/L04_native_io/README.md)给出 WSAPoll Reactor 与真实 Winsock IOCP，Linux 附加源仅作本次未执行的平台分支。

## 1. 为什么一个正确连接还不够

第 3 章同步基线在等待残头期间占据线程。为每连接分配一线程能保持正确，但线程栈、调度和大量阻塞等待成为新成本。事件循环把“下次应从哪一步继续”保存在连接对象，线程只处理能推进的事件。此处改变的是执行资源，不是 TCP 契约。

`reactor.hpp` 的连接拥有 decoder、写队列、读写状态；Reactor 拥有监听 socket 和最多 16 个连接。每轮建立需要监听的事件集合，等待后依次推进。只有发送队列非空才监听可写，避免一个始终可写的空连接制造忙循环。只有 `can_read()` 为真才监听可读，背压才能传到内核接收窗口。

## 2. 公平性和排空条件

“读到 would-block”对一个持续生产数据的客户端可能永远不结束。因此一次 dispatch 的读、写方向各最多推进 16 KiB，之后服务其他连接。每次读块最多 4096，TX 高水位 48 KiB、硬上限 64 KiB 之间为当前输入的响应留出空间。这个推导针对固定回送协议；若一个请求能生成无限响应，就必须在业务生产端另设额度。

L04 将一个客户端设置为慢读者，等程序实际观察到背压后再发正常请求。检查正常客户端仍得到完整回送；内存上界由连接模型检查与队列容量约束验证，此驱动不记录进程内存峰值。同步原子/条件通知用于确认状态，不把 sleep 之后“应该已经慢了”作为证据。关闭检查还注入对端 RST：即使容器已清空，只要丢过已接受响应，`clean_` 必须保持失败。资源都释放和业务成功排空是两件事。

## 3. epoll LT、ET、oneshot 的责任差异

LT 模式下，只要条件仍就绪，后续 wait 还可返回事件；执行预算用完后可回到主循环。ET 主要通知状态变化，如果未读空就停下，内核可能不再提供新边沿。因此有两种正确选择：持续处理到 EAGAIN，或在预算用尽时把连接放进**用户态可运行队列**，下一轮不用等 epoll 就继续。盲目复制 LT 的预算到 ET 会造成永久停顿。

oneshot 将一次事件消费后禁用后续通知，处理者更新完整 interest 后再 MOD 重装。重装不是释放连接：事件批次可能仍带有旧标识。生产多线程事件循环常用“槽位＋generation”防止关闭后描述符复用，或者将对象保留到当前批次结束。本课单 owner 的 poll 循环不在多个线程间转移连接；因此不需要凭空增加一套无锁回收机制。

可推演一个反例：socket 中有 24 KiB，预算 16 KiB，只收到一次 ET；余下 8 KiB 没有用户态 continuation 就永远不处理。附加 epoll 例子对 LT/ET/oneshot 使用同一实际 socket 和有界接收目标，ET 通过 continuation 保证剩余数据推进。Linux 分支不借用 Windows 结果宣称通过。

## 4. IOCP 是已接受操作的完成队列

Proactor 提交“把数据放到这个缓冲”这一操作，随后接收完成量。`iocp.cpp` 中每个 operation 内嵌 OVERLAPPED 与 64 字节数组。socket 与 completion port 关联，completion key 标识连接绑定，返回的 OVERLAPPED 指针标识具体操作。两者不能互相替代，同一连接可能同时有读操作和写操作。

默认通知模式下，WSARecv/WSASend 立即成功也仍会产生 completion packet。课程没有启用跳过立即完成通知的特殊模式，因此统一等到队列消费后清除 pending。ERROR_IO_PENDING 表示提交被接受；其他立即错误没有对应的已接受操作，必须在提交路径处理。完成字节数仍可能少于目标，需要带偏移继续提交。

CancelIoEx 只是发出取消请求。ERROR_NOT_FOUND 可能意味着操作已经完成或不再可取消，并不允许立即释放缓冲。最后收到的完成可能是成功，也可能是 ERROR_OPERATION_ABORTED；课程测试接受这两个合法竞态结果，并检查操作身份只消费一次。[Microsoft 取消说明](https://learn.microsoft.com/en-us/windows/win32/fileio/canceling-pending-i-o-operations)要求应用继续处理完成通知。

## 5. io_uring 网络操作

io_uring 的 SQE 表示提交，CQE 用 user_data 标识操作，res 表示字节数或负 errno。recv 结果 0 按 TCP EOF 处理。取消 SQE 本身有一条 CQE，被取消 recv 还有其自己的 CQE；取消结果成功不能代替收取目标完成。若目标已经成功，取消可能返回 -ENOENT。buffer 和操作身份必须保留到目标 CQE，而非取消 CQE。

多次接收、buffer selection、注册缓冲和 zero-copy 都会增加新的存活条件。例如 multishot CQE 的 MORE 标志决定同一个提交是否还可能产生完成；通知 CQE 可能决定发送缓冲何时能回收。不能把单次操作示例改个 opcode 就当作掌握这些扩展。L04 的 Linux 单次 recv/cancel 分支明确只实现最小生命周期，不冒充跨版本的高性能服务器。

## 6. 自测与解析

1. **ET 预算耗尽后为什么不能直接等待下一次 epoll？** 仍然就绪但没有新边沿，必须保留用户态 runnable 状态，或本轮读空。
2. **IOCP 立即成功时 pending 是否为 false？** 本课通知模式下应为 true，直到消费相应 packet；否则会复用仍有旧完成的 OVERLAPPED。
3. **取消后缓冲还能被写吗？** 在最终完成前后端仍拥有访问权；只有完成收束才结束借用。
4. **清空连接容器足以证明优雅关闭吗？** 不足，还需确认已接受响应未丢失、每个已接受 I/O 已完成、所有回调与工作线程退出。
