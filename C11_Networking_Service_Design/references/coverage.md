# 知识覆盖与反向依赖

本表按实际问题连接正文、练习和代码，不用题号数量代表完成程度。正文含自测解析，练习 README 含 Part、编辑位置与预期解释；构建与结果含义见[构建指南](../exercises/BUILD_GUIDE.md)。执行日志、测量样本和独立审查记录属于本机 build 材料，不作为读者理解课程的前提。

| 下游场景 / 核心问题 | 硬先修 | 主讲与解析 | 实现 / 观察入口 | 能力边界 |
|---|---|---|---|---|
| socket 连接失败与传输失败分层 | C02 RAII、C05字节、C07调用 | [01](../chapters/01-transport.md)、[02](../chapters/02-sockets-dns.md) | [L03](../exercises/L03_transport/README.md)、socket.hpp | IPv4/6回环、系统候选、有限顺序连接；Happy Eyeballs为算法推导 |
| DNS 名称指针、长度与完整报文校验 | span、expected、字节序 | [02](../chapters/02-sockets-dns.md) | L03 Student/Reference/good/bad、dns.hpp | ASCII hostname A-query，真实UDP fixture；不声称递归resolver/DNSSEC |
| 任意 TCP 分片、空帧、截断 | C05解析、对象拥有 | [03](../chapters/03-framing-backpressure-shutdown.md) | [L01](../exercises/L01_framing/README.md) | 增量decoder、有界坏控制；模型分片和真实socket分开 |
| 部分写、FIFO、缓冲保留、慢读者 | 帧、移动/借用 | [03](../chapters/03-framing-backpressure-shutdown.md) | [L02](../exercises/L02_connections/README.md)、write_queue.hpp | 64KiB应用有效载荷拥有量，不等于RSS/内核缓冲 |
| 半关闭、RST和已接受响应排空 | TCP、状态机 | [03](../chapters/03-framing-backpressure-shutdown.md)、[04](../chapters/04-reactor-proactor.md) | L02 baseline/states/loopback、[L04](../exercises/L04_native_io/README.md) | 容器清空不代替clean，真实RST失败记录 |
| Reactor公平性、LT/ET/oneshot | C07 readiness | [04](../chapters/04-reactor-proactor.md) | WSAPoll/poll reactor、epoll.cpp | Windows路径验证；Linux专属源码本次未执行 |
| IOCP/io_uring取消后缓冲存活 | C02借用、C07 completion | [04](../chapters/04-reactor-proactor.md) | iocp.cpp、uring.cpp | IOCP真实socket；uring单次Linux分支未执行，不混称multishot |
| handler、executor、strand、协程帧 | C08同步、C09 await | [05](../chapters/05-asio-lifetime.md) | [L06](../exercises/L06_asio/README.md) | 固定Boost；runtime部分在任务域后回读 |
| TLS身份、mTLS、关闭 | socket、异步生命周期 | [06](../chapters/06-tls.md) | [L07](../exercises/L07_tls/README.md) | memory BIO+TCP；ALPN/恢复/0RTT为推导，私有CA不入系统信任 |
| HTTP边界、JSON、WS单写者 | 帧、Asio、类型验证 | [07](../chapters/07-http-websocket.md) | [P1](../exercises/P1_task_service/README.md) | 回环HTTP/WS；TLS另实验，未接成HTTPS网关 |
| HTTP2帧/HPACK/流与连接窗口 | HTTP、TCP顺序 | [08](../chapters/08-http2.md) | [L08](../exercises/L08_http2/README.md) | 真TCP双流、信用、reset/GOAWAY；固定有限驱动 |
| QUIC包号/流偏移/拥塞/路径 | UDP、TLS、多流 | [09](../chapters/09-quic.md) | [L09](../exercises/L09_quic/README.md) | 真QUIC/TLS/背压/取消；恢复/迁移/HTTP3推导，未做公网损伤实测 |
| RPC deadline与业务幂等 | C03错误、HTTP2、状态 | [10](../chapters/10-rpc-service-policy.md) | [L05](../exercises/L05_tasks/README.md)、[P2](../exercises/P2_grpc/README.md) | protobuf/gRPC真实TLS；单进程保留窗口，不承诺跨崩溃exactly-once |
| 连接池、令牌桶、退避/抖动 | 时间预算、RAII、同步 | [10](../chapters/10-rpc-service-policy.md) | [L10](../exercises/L10_service_policy/README.md) | 真实复用/队列/超时；速率策略独立，不虚称P1已部署QPS配额 |
| 两worker、有界记录、快照订阅 | L05策略、C08同步、Asio | [11](../chapters/11-task-service.md) | task_registry/runtime、P1/P2 | 相同领域实现、独立进程状态；无持久存储/多租户 |
| 日志/metrics/trace与故障范围 | 阶段与错误建模 | [11](../chapters/11-task-service.md)、[12](../chapters/12-bridges-source-performance.md) | P1实际计数、B01阶段时间 | trace字段/高基数边界为设计推导，未接外部collector |
| 旧协程RPC wire互操作 | C09 client/awaitable | [12](../chapters/12-bridges-source-performance.md) | [L11 legacy](../exercises/L11_bridges/README.md) | 旧API/wire不变；C11独立有限peer，不改旧Student |
| socket完成到sender三通道 | C10环境/operation state | [12](../chapters/12-bridges-source-performance.md) | L11 network_sender | 真async_read_some；单owner/strand，独占socket取消责任 |
| 源码状态/释放路径、成本归因 | 对应协议和所有权 | [12](../chapters/12-bridges-source-performance.md) | [源码索引](standards-and-implementations.md)、[B01](../exercises/B01_pool_cost/README.md) | 固定实现；连接策略闭环负载，不外推公网吞吐 |

## 反向检查与原知识去向

P1 的 JSON 数值收窄来自 C05/C03，缓冲生命周期来自 C02、第3/5章，取消收束来自 C08、第5/11章；P2 的 schema/presence 和状态映射来自第10章，跨线程值捕获来自 C02/C08。L11 的 completion signatures、stop token 环境与operation state 由 C10 主讲，本课补网络借用和实际完成接线。HPACK/QPACK、TLS、QUIC 不以泛型基础已会为理由省略协议机制。

C07 原有系统I/O继续主讲；C09 原RPC保留接口、wire、Student、good，不迁移删除；C10 原文件I/O sender保留，C11新建真实socket分支。新增内容承担之前缺少的独立网络与服务深度，没有以“另有标题”替代旧知识。

本次验证按用户约定限于 Windows 编译和关键行为；参考成功、观察成功、故意错误被拒绝、Student完成是不同结论。Linux专属路径保留明确未验证状态。独立检查提出的已知技术/教学矛盾需在实际文件修正，不通过修改汇总标签绕过。
