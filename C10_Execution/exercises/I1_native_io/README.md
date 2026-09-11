# I1 native_io

目标：把真实 OS completion 接成 sender。Windows 路径使用 IOCP + overlapped `ReadFile`；Linux 路径使用 `io_uring`。同步 `read()`/`ReadFile` 包线程池不算通过。

Part 1：使用 provided `io_context` 后端。它只提供文件打开、bounded request 和 backend completion；你仍要自己实现 sender、operation state 和完成映射。

Part 2：实现 `read_at(file, offset, buffer)` sender。buffer 为借用，operation state 地址在完成前必须稳定；完成信号后不能再访问可能已销毁的 state。

Part 3：处理空读、短读、EOF、打开失败、多在途和取消竞态。取消请求不是完成；目标读成功早于取消时应给 value，取消赢时给 stopped。

独立位置：只改 `src/student/solution.hpp`。Reference、good、bad 与 checker 分离；Student 初始状态必须在真实 `read_at` 路径报告 `UNFINISHED`，不能靠完成开关或预跑检查绕过。

## 完成早于提交返回的回归实验

Windows 的 IOCP worker 可能在 ReadFile 尚未返回时取到完成包。后端必须先把 request 注册进持有表，再向系统提交；否则 take_alive 找不到请求，完成信号被丢弃，sync_wait 永久等待。同步提交失败时撤销注册并回滚在途计数；持有表分配失败在提交前转为资源错误。Linux 同样先持有再准备/提交 SQE，无空闲 SQE 时撤销本次持有。

I1 Windows checker 对各实现启用 C10_NATIVE_IO_TEST_POST_SUBMIT：立即保存 GetLastError 后延迟10ms，放大 completion 抢先窗口。这是测试编译开关，P1 正常路径不启用。旧顺序在受控副本中稳定 timeout；当前顺序必须在同样窗口下完成并关闭。延长超时不能修复丢失完成，原始对照见 io-review/review-report-r4.md 所列记录。

## Part 解析与所有权

sender 保存文件共享所有权、offset和借用buffer；connect 为每次连接构造独立且不可移动的 operation。Reference 将 receiver、stop callback、请求句柄和终结标志放入共享状态，避免空读或极快完成在 start 尚未返回时删除操作对象导致后续成员访问。Good用独立的状态组织实现同一契约；Bad可编译但故意违反offset读取，被 `offset read bytes match` 拒绝。

停止回调可能与请求发布并发：回调先记录停止请求，再在 request_mutex 下复制已发布请求；start 成功发布后重新检查停止位。系统最终成功时发value，系统报告取消时发stopped，其他错误发exception_ptr；request_stop本身不能提前释放buffer或替系统制造完成。完成回调撤销stop callback并移动receiver，之后不访问可能被终结销毁的operation。

所有者必须让context、文件和buffer活到目标请求及停止回调收束；在所有者线程关闭/join context，不能从其完成线程析构context并join自己。公开限制是regular file、每次buffer/文件最多1 MiB、合法offset；目录、超大文件、超大buffer由checker确定性拒绝。普通短读/EOF属于value结果，不能误判为后端失效。

## 验证入口

先按[构建指南](../BUILD_GUIDE.md)配置本目录。Windows使用IOCP；Linux传入固定liburing前缀并启用io_uring。检查器覆盖真实offset/短读/空读、多在途、pre-stop与取消竞态，并让终结receiver直接删除operation以检查早完成。Student初态为UNFINISHED/exit2；Reference与Good为exit0；Bad必须命中特定行为失败，崩溃或timeout不算成功拒绝。完整推导、平台差异及异常边界见[正文11](../../chapters/11-native-io.md)。
