# 07 HTTP/1.1 与 WebSocket 的连接状态

先修是分帧、背压、Asio 与 TLS。本章使用 [P1](../exercises/P1_task_service/README.md)中的 Beast HTTP/WS 适配层；它监听回环明文 HTTP，TLS 单独在 L07 实践。不能把二者称作已经接线的 HTTPS 服务。

## 1. 先用成熟 parser 建立正确基线

HTTP 是应用语义，HTTP/1.1 是其字节映射。请求行、字段、空行、body 依次解析，但 body 不总是“读到连接断开”：Content-Length、Transfer-Encoding、请求/响应上下文决定边界。持久连接上读过头会吃掉下一条消息，读少了会把残 body 当下一条请求行。重复或冲突长度可能让前后端解释不一致，形成请求走私风险。课程使用 Beast parser，应用再验证路由、字段数量、类型与业务输入。[HTTP/1.1 RFC9112](https://www.rfc-editor.org/rfc/rfc9112.html)规定消息边界，语义见 RFC9110。

P1 每轮构造新的 parser，设置 header_limit=8192、body_limit=4096。flat_buffer 跨请求保留，因为一次 socket read 可能已经读入下一条消息。parser.release 移出已完整解析的消息；response_ 是 session 成员，保存到 async_write 完成。若响应只放在 send 函数局部变量里，函数返回后网络写就悬空。

持久连接不意味着并行响应。当前基线严格“完整读一个请求 → 路由 → 写完一个响应 → 下一读”，自然保持 HTTP/1 响应顺序。慢响应阻塞同连接后续请求是这个契约的代价；不要直接并发 async_write 来解除它。HTTP/2 用不同 wire 与 stream 状态解决多路复用，是后续独立分支。

## 2. HTTP 状态与业务状态

POST /tasks 返回 202，表示已接受，任务可能仍 queued/running。GET /tasks/id 返回快照，DELETE 请求协作取消。参数不合法为 400、找不到为 404、同幂等键不同内容为 409、容量拒绝为 429、停止接纳为 503。请求成功送达与任务成功执行必须分开观察。

请求规定一个 Idempotency-Key 和精确 application/json。JSON 使用严格字段白名单、整数范围和必填检查，不允许字符串数字或未知字段悄悄改变语义。HTTP parser 只知道 body 是字节，不知道 steps 最大 1000；JSON parser 只知道数字合法，不知道 left/right 只能在 ±1,000,000。每一层验证自己的契约，不能互相替代。

本服务是有界教学实例，没有租户认证、持久存储或反向代理信任模型。暴露公网前需要真正加入认证/授权与传输保护，而不是把回环绑定换成通配地址就部署。

## 3. 从 HTTP upgrade 到 WebSocket

WebSocket 先进行 HTTP 握手。Upgrade/Connection 等字段和 Sec-WebSocket-Key 的握手由 Beast 验证，成功后同一 TCP 流进入 WS 帧格式。P1 只接受 `/events?task_id=...`，并拒绝升级时 buffer 中残留的预读字节，避免把 HTTP 流水线数据模糊转交给另一协议。

WebSocket 帧有 FIN、opcode、mask 与扩展长度；一条消息可分成多帧。客户端到服务器必须按协议掩码；mask 不是加密。控制帧允许插入消息分片中，因此即使服务只向外推送事件，也需要维持读操作处理 ping/pong/close。`read_message_max(4096)` 限制重组后的消息，不仅检查单个帧。[RFC6455](https://www.rfc-editor.org/rfc/rfc6455.html)是该协议分支的来源。

P1 同时允许一个 WS read 和一个 WS write，禁止重叠两个 write。`event_` 在写完成前保留；终态事件写完才启动 close handshake。关闭与正在发送的数据同属写侧状态，不能在任意回调直接发起第二个关闭写操作。

## 4. 推送不是无限消息队列

订阅首先在 registry owner 上原子地“读取当前快照＋登记订阅”，然后接收后续修订。这消除 get 与 subscribe 之间错过终态的缝隙。每订阅最多 8 个事件，总共 32 个订阅，超限关闭慢订阅；任务继续执行。客户端重连通过 GET 取得当前状态，不提供事件重放或保证每个 progress 值都可见。

传输写也有 5 秒 watchdog。registry 队列有界只能限制还没交给网络的事件；已经交给 async_write 的 event_ 仍需计入拥有量。强制关闭若丢掉正在发送的响应，增加 lost_responses，clean() 返回 false。这样测试不会把“内存清空了”误写成“所有请求成功完成”。

## 5. 自测与解析

1. **为什么 flat_buffer 不随 parser 重建？** 它可能包含下一请求的预读字节，丢掉会破坏连接字节流。
2. **202 是否意味着 sum 已可读取？** 否，返回的是任务身份与当前状态；应 GET 或订阅终态。
3. **仅服务端推送还要 async_read 吗？** 要，协议控制帧和对端关闭从读侧到达。
4. **慢订阅关闭为什么不取消任务？** 观察者生命周期与已接纳任务不同，另一个观察者可能仍需要结果。
5. **响应写完能否立刻清除幂等记录？** 不能，写完成只归还本地缓冲，客户端可能没有收到响应；记录保留按业务窗口执行。
