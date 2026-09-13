# L09 MsQuic 的真实流、信用与取消

正文：[QUIC](../../chapters/09-quic.md)。使用固定 MsQuic2.6.1 DLL 与其固定 OpenSSL 子模块构建的私有 TLS 库，配置见 [BUILD_GUIDE](../BUILD_GUIDE.md)。不需要 XDP 驱动或系统服务。

## Part A：两个流

[main.cpp](main.cpp)是完整 Reference。先预测流 0 的 32768 字节发送在接收方返回 pending 后是否完成，以及流 4 的 small 是否仍能收到 ack。构建 C11_L09_quic，运行对应 CTest。测试内部建立临时 CA、localhost 证书和 UDP 端口，正常退出删除 PEM 文件。

解析：每流信用独立且连接信用足够时，小流可完成。pending 表示应用还在借用接收数据，StreamReceiveComplete 归还信用后大流继续。发送关闭 buffering，所以大流缓冲在 SEND_COMPLETE 前必须保留。

## Part B：取消与身份

第三个单向流在接收 pending 时 ABORT_SEND，观察 peer reset 与 canceled send completion，再收齐 stream/connection shutdown complete。第二轮用不相关 CA，预期握手失败且从未 connected。

```powershell
cmake --build build/c11-quic --config Release --target C11_L09_quic
ctest --test-dir build/c11-quic -C Release -R '^C11_L09_quic$' --output-on-failure
```

解析：取消调用成功不归还发送数组，handle 也不在任意回调立刻关闭。Windows OpenSSL 路径显式选择内建证书验证与 CA 文件，不能禁用验证。未知 CA 的 TLS alert 与同名但错误签名的 CA 可能不同，因此 fixture 使用明确不同颁发者。

## Part C：包丢失与迁移推演

按正文包 10/11/12 轨迹写出包号与流偏移，解释为什么重传新包号仍是同一流内容。按 CID→候选新地址→路径验证→拥塞状态的顺序推导迁移。扩展 UDP 损伤转发器应有限报文数、确定性丢弃和双向计数；本次未执行该扩展，不用应用层丢消息声称验证 QUIC 恢复。

本目标验证 QUIC transport/TLS，不包含 HTTP3/QPACK、0-RTT、公网损伤或迁移实测。
