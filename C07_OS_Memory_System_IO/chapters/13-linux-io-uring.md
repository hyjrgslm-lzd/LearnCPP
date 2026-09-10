# 13 Linux io_uring：提交队列、完成队列与能力探测

## 为什么不是“Linux 版 IOCP”

io_uring 也是完成式 I/O，但不是 Windows IOCP 的重命名。它有提交队列 SQ、完成队列 CQ、opcode、flags、内核版本能力和 liburing 用户态封装。用户把 SQE 填好并提交，内核稍后产生 CQE；CQE 的 `user_data` 是找回请求身份的关键。

相似处在生命周期：请求被接受后，buffer、fd、用户态状态和 request_id 必须活到 CQE 被消费。不同处在提交和取消的细节：一次 submit 可能只提交部分 SQE；取消请求本身也有自己的 CQE；目标请求可能已经完成、被取消、或返回其他错误。课程必须分别处理“取消请求完成”和“目标请求完成”。

## 能力探测与主体测试分离

io_uring 的头文件存在，不等于当前 kernel、权限和 opcode 能运行。C07 固定 liburing 2.15，并由构建选项显式启用。能力探测只回答“这个环境是否具备运行主体的条件”；主体测试再验证课程算法。如果探测失败，相关路径 SKIP；如果探测通过后主体 ledger 错误，那是 FAIL。

本课不安装系统包，不静默换版本。父任务已在 WSL 的独立目录准备固定 liburing 前缀，CMake 通过显式根路径接入。Windows 构建不启用 io_uring；Linux 未启用选项时，不注册 platform 测试，不把它报告为 SKIP。只有启用了 platform 路径且 ring/opcode/权限能力不足时，测试才输出 `SKIP:` 并返回 77。

## 提交、CQE 与 request_id

L08 的最小正确循环是：申请 SQE，填 opcode、fd、buffer、length 和 offset，设置 `user_data`，submit，并确认接受数量。之后 wait CQE，读取 `res` 和 `user_data`，调用 `io_uring_cqe_seen` 归还 CQE 槽。漏掉 `cqe_seen` 会让 ring 空间耗尽；忽略 `user_data` 会把多个请求结果混在一起。

```cpp
io_uring_prep_read(sqe, file.get(), buffer.data(), buffer.size(), 0);
io_uring_sqe_set_data64(sqe, request_id);
if (io_uring_submit(&ring) != 1) return error;
io_uring_wait_cqe_timeout(&ring, &cqe, &timeout);
auto id = io_uring_cqe_get_data64(cqe);
auto res = cqe->res;
io_uring_cqe_seen(&ring, cqe);
```

对 pipe 这类没有随机偏移的对象，read/write offset 应用 `-1`，表示使用当前文件位置或对象语义。把 offset 写成 0 可能只适用于普通文件，换到 pipe/socket 就不是同一契约。

## 取消与收束

io_uring cancel 是另一个 SQE。它提交后会产生取消请求自己的 CQE；被取消的目标请求也会产生自己的 CQE，常见结果是 `-ECANCELED`，但目标也可能已经成功完成。L08 ledger 分别记录 `cancel_completed(cancel_id, target_id, error)` 和 `target_completed(target_id, bytes, error)`，不把二者合并。

部分提交是硬边界。如果代码准备了 read 和 cancel 两个 SQE，却只确认 `submitted >= 1`，随后等待两个 CQE，就可能等待一个从未提交的请求。课程实现要么逐个提交并检查数量，要么维护已接受请求集合，只等待已接受的 request_id。

## 本章边界

io_uring 可以注册 buffer、注册文件、poll、timeout、link、multishot 等。C07 样章只覆盖单线程、有限在途、临时文件 READ、pipe pending read、ASYNC_CANCEL 和 drain 的核心生命周期。高级批处理、网络协议、协程恢复和 sender 接入分别由后续课程承接。

通过一次小 payload 不证明 io_uring 更快。性能结论必须回到 B01 的阶段计数和独立进程样本；本章只建立正确的请求身份与完成收束。
