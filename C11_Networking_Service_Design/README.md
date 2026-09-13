# C11 网络编程与服务设计

本课面向会写 C++、需要解释和实现网络服务的工程师。从一条有界 TCP 请求开始，逐步建立连接状态、异步执行、加密协议、多路复用和任务服务。语言采用 C++23；Boost.Asio、stdexec、gRPC 是固定第三方实现，不称为标准 C++ 网络库。

## 先修与路线

硬先修是 [C02 对象和资源](../C02_Objects_Lifetime_Ownership/README.md)、[C03 错误与接口](../C03_Type_Modeling_Interface_Design/README.md)、[C05 字节、解析与时间](../C05_Data_Representation_Standard_Facilities/README.md)、[C07 系统 I/O](../C07_OS_Memory_System_IO/README.md)的对应主题。进入多线程服务前补 [C08 同步](../C08_Concurrency/README.md)。第 5 章协程表达需要 C09 的 await 协议；sender 桥接需要 C10 的完成通道、环境与 operation state。它们不是同步网络入门的先修。

| 阅读顺序 | 本章解决的问题 | 实践入口 |
|---|---|---|
| [01 网络与传输](chapters/01-transport.md) | 地址、TCP/UDP、拥塞与可靠性的责任边界 | L03 sockets |
| [02 Socket 与 DNS](chapters/02-sockets-dns.md) | 句柄、候选地址、名字压缩与输入校验 | L03 实现题 |
| [03 分帧、背压与关闭](chapters/03-framing-backpressure-shutdown.md) | 字节如何成为有界且可排空的连接 | L01、L02 |
| [04 Reactor 与 Proactor](chapters/04-reactor-proactor.md) | 就绪/完成、公平性、取消后的存活责任 | L04 |
| [05 Asio 与组合生命周期](chapters/05-asio-lifetime.md) | executor、strand、协程、deadline、收束 | L06 |
| [06 TLS 与身份](chapters/06-tls.md) | 握手、信任链、主机名、双向认证与关闭 | L07 |
| [07 HTTP 与 WebSocket](chapters/07-http-websocket.md) | 解析边界、持久连接、升级、单写者 | P1 |
| [08 HTTP/2](chapters/08-http2.md) | 帧、HPACK、双层流控、多流与连接错误 | L08 |
| [09 QUIC](chapters/09-quic.md) | 包号、流偏移、丢包恢复、TLS、迁移 | L09 |
| [10 RPC 与服务策略](chapters/10-rpc-service-policy.md) | schema、gRPC、池、重试、幂等和限流 | L05、L10、P2 |
| [11 有界任务服务](chapters/11-task-service.md) | 接纳、执行、观察、错误和关闭如何闭合 | P1、P2 |
| [12 跨课桥接、源码与测量](chapters/12-bridges-source-performance.md) | 协程/sender 接线、生产源码与成本归因 | L11、B01 |

第一次阅读先走 01—07，再以第 11 章串起 P1；随后深入 08—10 和 P2，最后完成第 12 章。进阶部分是完整课程内容，不能以已运行 P1 代替。

## 动手方式

先看 [构建指南](exercises/BUILD_GUIDE.md)。默认离线核心包含分帧、socket、DNS、原生 I/O、任务策略和连接池。Asio/TLS/HTTP2/QUIC/gRPC/sender 通过显式选项接入已准备的固定源码；配置不会自动下载。

实现题 L01/L02/L03/L05/L10 的 `student/solution.hpp` 是独立编辑位置。Reference 与 good/bad 检查器控制保留；未完成 Student 明确失败。其余单元是完整观察驱动，需先预测再运行，完成 README 中的修改与解释题；程序通过不自动代表作业完成。

本次验证范围为 **Windows 编译与关键行为**。Linux 专属源代码及实验规格保留，并标记未在本次环境执行；无需安装 Linux、WSL 或虚拟机。功能检查使用回环临时端口、进程内测试 CA、有界资源和进程外超时。基准不是性能承诺，课程不发布从本机单次结果推导的通用加速比。

知识点、实现与能力边界见 [覆盖索引](references/coverage.md)；规范和固定源码入口见 [来源索引](references/standards-and-implementations.md)。原 C09 RPC 的 API、wire 和 Student 保留；C07 继续主讲系统机制，C10 继续主讲执行组合。
