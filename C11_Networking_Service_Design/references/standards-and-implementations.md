# 规范与固定实现源码索引

核对日期：2026-09-13。协议规范、第三方版本和本机功能观察分别记录。固定依赖机器可读清单见 [dependencies.json](dependencies.json)；源码准备与加密后端隔离见 [BUILD_GUIDE](../exercises/BUILD_GUIDE.md)。C++23用于课程代码；stdexec与Asio网络接口是第三方实现，不称为标准C++网络库。

## 协议与平台来源

| 主讲主题 | 一手来源 | 阅读问题 |
|---|---|---|
| TCP、半关闭、复位 | [RFC9293](https://www.rfc-editor.org/rfc/rfc9293.html) | 状态机的发送/接收方向何时独立结束？ACK到哪一层？ |
| UDP与报文边界 | [RFC768](https://www.rfc-editor.org/rfc/rfc768.html)、[Winsock recvfrom](https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-recvfrom) | 空报文与截断如何区分？ |
| DNS名称/计数/压缩 | [RFC1035 §4](https://www.rfc-editor.org/rfc/rfc1035.html#section-4)、[RFC9267](https://www.rfc-editor.org/rfc/rfc9267.html) | 压缩指针的原消费位置、展开路径和计数验证各是什么？ |
| 地址族候选 | [RFC8305](https://www.rfc-editor.org/rfc/rfc8305.html) | Happy Eyeballs如何避免一个地址族延误全部尝试？ |
| IOCP、取消 | [完成端口](https://learn.microsoft.com/en-us/windows/win32/fileio/i-o-completion-ports)、[取消说明](https://learn.microsoft.com/en-us/windows/win32/fileio/canceling-pending-i-o-operations) | 取消调用返回后哪些对象仍被系统借用？ |
| TLS1.3 | [RFC9846](https://datatracker.ietf.org/doc/html/rfc9846)、[旧RFC8446状态页](https://www.rfc-editor.org/info/rfc8446/) | 当前TLS1.3修订取代8446；握手认证、transcript、记录与关闭分别保证什么？ |
| HTTP语义和HTTP/1.1 | [RFC9110](https://www.rfc-editor.org/rfc/rfc9110.html)、[RFC9112](https://www.rfc-editor.org/rfc/rfc9112.html) | 状态语义与字节消息边界为什么分开？ |
| WebSocket | [RFC6455](https://www.rfc-editor.org/rfc/rfc6455.html) | 消息分片、控制帧和关闭握手怎样共享连接？ |
| HTTP2、HPACK | [RFC9113](https://www.rfc-editor.org/rfc/rfc9113.html)、[RFC7541](https://www.rfc-editor.org/rfc/rfc7541.html) | 流/连接窗口与压缩状态为何不能互换？ |
| QUIC transport/TLS/recovery | [RFC9000](https://www.rfc-editor.org/rfc/rfc9000.html)、[RFC9001](https://www.rfc-editor.org/rfc/rfc9001.html)、[RFC9002](https://www.rfc-editor.org/rfc/rfc9002.html) | 包号、流偏移、地址验证、丢包恢复各有哪些状态？ |
| HTTP3、QPACK | [RFC9114](https://www.rfc-editor.org/rfc/rfc9114.html)、[RFC9204](https://www.rfc-editor.org/rfc/rfc9204.html) | 指令流和字段引用怎样限制压缩阻塞？属于推导分支，L09不运行HTTP3。 |

TLS规范修订日期不意味着本课固定OpenSSL自动实现修订中的每个扩展。课程实验只声称所执行的TLS1.3握手、身份与关闭检查，算法套件/扩展全集需另行能力核验。

## 源码导读：入口到退出

下面的Boost库链接使用匹配的boost-1.92.0发行标签；完整Boost来源用archive SHA256锁定。Git依赖链接用提交锁定。打开后沿列出的符号追踪，不只阅读声明。

| 实现与版本 | 真实入口 | 状态、退出路径与教学对照 |
|---|---|---|
| Boost.Asio / Boost1.92.0 | [impl/read.hpp](https://github.com/boostorg/asio/blob/boost-1.92.0/include/boost/asio/impl/read.hpp)、[win_iocp_socket_recv_op.hpp](https://github.com/boostorg/asio/blob/boost-1.92.0/include/boost/asio/detail/win_iocp_socket_recv_op.hpp) | 从initiate_async_read/read_op跟踪已完成量与下一次read_some，再看do_complete怎样取得错误和bytes、归还operation内存并调用handler。对照L06帧存活和L04 OVERLAPPED。 |
| Boost.Beast / Boost1.92.0 | [HTTP read](https://github.com/boostorg/beast/blob/boost-1.92.0/include/boost/beast/http/impl/read.hpp)、[WS write](https://github.com/boostorg/beast/blob/boost-1.92.0/include/boost/beast/websocket/impl/write.hpp) | HTTP parser消费buffer直到message完成，未消费字节留给下一请求；WS写状态协调写锁、控制帧与错误出口。对照P1 parser/response_/writing_。 |
| OpenSSL3.5.8 | [statem_clnt.c](https://github.com/openssl/openssl/blob/f4dc4d58b48d346a8270183f89acf826d459b0ca/ssl/statem/statem_clnt.c)、[x509_vfy.c](https://github.com/openssl/openssl/blob/f4dc4d58b48d346a8270183f89acf826d459b0ca/crypto/x509/x509_vfy.c) | 从客户端握手状态进入certificate处理，再跟踪chain验证的成功和错误返回；名称校验参数来自应用配置。对照L07可信/不可信CA与hostname，不将密码实现改写成教学替代。 |
| nghttp2v1.70.0 | [nghttp2_session.c](https://github.com/nghttp2/nghttp2/blob/85e300c79fb6dbcfa9c1013215c8710c1c2cd3d2/lib/nghttp2_session.c)、[nghttp2_hd.c](https://github.com/nghttp2/nghttp2/blob/85e300c79fb6dbcfa9c1013215c8710c1c2cd3d2/lib/nghttp2_hd.c) | mem_recv2→frame/stream/window→callbacks；mem_send2→outbound DATA→provider；HPACK deflate/inflate维护方向性表。错误分别到RST/GOAWAY，stream close后释放应用payload。 |
| MsQuicv2.6.1 | [stream_recv.c](https://github.com/microsoft/msquic/blob/a01333cf7c2659cce0ff03ef3f21e1ff15bb5b83/src/core/stream_recv.c)、[stream_send.c](https://github.com/microsoft/msquic/blob/a01333cf7c2659cce0ff03ef3f21e1ff15bb5b83/src/core/stream_send.c)、[loss_detection.c](https://github.com/microsoft/msquic/blob/a01333cf7c2659cce0ff03ef3f21e1ff15bb5b83/src/core/loss_detection.c) | 跟踪receive pending/complete、发送请求与SEND_COMPLETE、ACK/丢失队列。应用stream handle在shutdown complete收束，对照L09的buffer/context拥有者。 |
| gRPCv1.84.0 | [call_op_set.h](https://github.com/grpc/grpc/blob/3252a89f10d8e92997862167ca7d095ecda85973/include/grpcpp/impl/call_op_set.h)、[chttp2_transport.cc](https://github.com/grpc/grpc/blob/3252a89f10d8e92997862167ca7d095ecda85973/src/core/ext/transport/chttp2/transport/chttp2_transport.cc) | 从生成Stub跟随batch/message/status操作，再进入HTTP2 transport流和关闭路径；回到P2 invoke区分gRPC调用线程与领域owner。 |
| standalone Asio1.38.2 | [impl/read.hpp](https://github.com/chriskohlhoff/asio/blob/8806a6803cde7054c3049d3666d3ec36786568c5/include/asio/impl/read.hpp) | 对照C09 read_frame如何使用async_read；此类型体系不与Boost.Asio互换。 |
| stdexec nvhpc-26.05 | [execution.hpp](https://github.com/NVIDIA/stdexec/blob/6d7ad689f4d4831c5136e4abe1c601f9a3b64e43/include/stdexec/execution.hpp) | 从connect/start/完成签名入口到receiver环境和stop callback；对照L11，标准/实现协议主讲在C10。 |
| liburing2.15 | [liburing.h](https://github.com/axboe/liburing/blob/d41bf9220ec39277ff235379e9089d9e0fd6c2a5/src/include/liburing.h) | recv与cancel64填充不同SQE，CQE_seen只归还队列条目，应用buffer何时可复用由目标完成决定。Linux分支本次未执行。 |

MsQuic的TLS子模块固定为453eaaa9e6bb1304730abacfbb73d51868cb6ab9，XDP头文件子模块为d372b52577a724e04fa4c06acb90bbfa4719fc25；后者不代表安装驱动。gRPC子模块由父提交gitlink锁定，Protobuf为匹配的35.1.0，不用机器中另一个protoc生成代码。

## 导读作业解析与许可证

每条路径回答：输入缓冲归谁、哪一步转移/借用、立即失败是否产生完成、取消后何时可释放。Asio/IOCP按最终handler/packet，nghttp2按provider不再可访问的stream退出，MsQuic按send/stream两级完成，gRPC按调用/ServerContext生命周期分别判断，不能统一成“函数返回成功就释放”。

第三方源码存放本地build，不复制整库进入课程。再分发库、生成代码或二进制时保留各固定来源的LICENSE/NOTICE及子模块要求：Boost/Asio、OpenSSL、nghttp2、MsQuic、gRPC/Protobuf、stdexec、liburing各按原包声明核对，不能用课程自己的许可替换上游许可。
