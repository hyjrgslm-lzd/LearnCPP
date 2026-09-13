# 10 RPC、幂等与有限服务能力

先修是 HTTP2、错误建模和任务状态。[L05](../exercises/L05_tasks/README.md)实现接纳策略，[L10](../exercises/L10_service_policy/README.md)实现重试与连接池观察，[P2](../exercises/P2_grpc/README.md)把同一领域逻辑接到 gRPC。

## 1. RPC 不是普通函数调用

本地函数返回之前，调用者通常知道是否得到结果。网络上存在一个不可消除的窗口：服务器已接受或执行，响应却丢失。客户端超时只能说明在预算内未得到结果，不能推出“远端没发生”。传输重试、函数异常和业务失败因此必须分层。

request ID 关联一次请求与响应；task ID 标识长期任务；idempotency key 关联同一业务提交的多次尝试。C09 RPC 每次重试更新 req_id，沿用 wire 可以保证关联，却不自动让服务器识别同一业务。这是 C11 新增幂等表的原因，不应偷偷修改 C09 协议把两种身份混合。

序列化契约同样属于 API。Protobuf 用字段号编码，新增可选字段比复用旧字段号容易兼容。删字段应保留其号码/名称，避免历史字节被误解释；未知字段可保留并不表示未知业务行为可接受。proto3 标量默认值与“字段未提供”可能相同，所以 P2 将 left/right/budget_ms 标为 optional，以区分合法零值和缺失。预算缺省 5000 ms，steps 缺省 0，与 HTTP JSON 一致。

## 2. gRPC 的实际边界

gRPC 基于 HTTP/2 承载方法、metadata 和消息，单条消息有压缩标志与长度前缀，Protobuf 是本项目选择的消息格式。unary 是一请求一响应；server streaming 多次发送快照；client streaming 与 bidirectional streaming 还需要分别管理两方向流结束、同方向读写串行和总消息预算。流式 API 不意味着结果队列可以无界。

P2 使用生成的同步 Service 作为易读基线，处理线程通过拥有捕获的 packaged_task 将领域调用提交给单 owner，然后有界等待 future。不能捕获 request 指针后让 ServerContext 结束：回调晚执行会访问已释放请求。这里在跨线程前复制整数和 key，结果也按值返回。

每个调用有 gRPC ClientContext deadline，服务另有 5 秒内部等待上限。取消等待后用 active 标记让尚未执行的 owner action 拒绝运行；如果 owner 已经接受任务，取消 RPC 不会撤销任务。ServerContext::IsCancelled 只描述调用状态，不提供业务事务回滚。Watch 还有独立活跃标记，作用域离开后 registry 清理遗留订阅。

同步 Watch 最多 32 个，阻塞 Write 由有限 deadline guard 请求取消；guard 在 ServerContext 失效前 join。这是一条有明确线程上限的教学路线。高并发可以改为 callback reactor/异步 CQ，但那是改变执行模型，需要重新证明 OnDone/CQ 完成前的对象存活，不能只比较代码行数。

## 3. 幂等记录必须在接纳时预留

最小记录包含 key、规范化 spec、task ID、当前状态与结束时间。接纳顺序是先验证输入，再查同 key：同 spec 返回旧任务，不同 spec 返回 conflict；只有全新任务才检查 draining 和容量。这样服务满载时，已有任务的合法重试仍可以查到原身份，不会被当成新的工作。

本课最多 128 个 live 任务，1024 条 active＋retained 记录，终态保留 5 分钟。记录槽位在接纳时预留，不能任务做完才尝试保存结果，否则成功执行后可能没地方存幂等状态。active 记录不能因缓存压力被淘汰；TTL 从进入终态计，不从排队开始计。

这保证的是**单进程内、保留窗口内**的重复接纳语义，不是跨崩溃的 exactly-once。进程重启后记录丢失，窗口外相同 key 可以成为新任务。持久业务需要 C12 的事务、唯一约束、WAL 或外部系统幂等协议；网络层不能凭重试标志凭空提供这些保证。

## 4. 重试预算要覆盖所有尝试

L10 的 retry_context 明确 idempotent、transient、attempt、max_attempts、now、deadline 和 Retry-After。只有幂等且瞬时失败才考虑重试；参数错误或幂等冲突没有靠等待恢复的理由。max_attempts 包含第一次，最大 8 次。attempt 从 0 开始，因此 `attempt >= max_attempts-1` 就不再重试，避免 unsigned 减法在 max_attempts=0 时下溢。

退避上界是 `min(500 ms, 25 ms * 2^attempt)`，在 [0,上界] 选 full jitter，最后与 Retry-After 取较大值。delay 必须严格小于剩余预算，否则不应发起注定没有执行窗口的新尝试。Retry-After 可能是日期或秒数，HTTP 适配需先校验再转成相对等待；L10 接口接收已校验毫秒值。

每层自己重试会相乘，例如三个调用层各最多三次可放大到 27 次。应让拥有端到端业务预算的一层协调，并记录 attempt 与实际接纳数。熔断器解决持续失败时快速拒绝与有限探测，不是替代限流；若引入半开态，必须限制探测并发并避免所有实例同时恢复放量。本课不把未实现的熔断框架加入运行时。

## 5. 限流和连接池各管哪种资源

token bucket 以容量控制突发，以补充周期控制长期平均率。L10 将 token 表示为时间信用，避免浮点累积误差；时间前进时补充到上限，倒退时不凭空增加，cost 也必须有界。速率限制与并发限制不同：瞬时接受 128 个很慢任务可能满足每秒速率，却耗尽 worker 队列。P1 的接纳上限是并发/记录限制，L10 的速率策略独立演示，并未声称已经部署全局 QPS 配额。

连接池先提供上限、等待队列、租约与关闭契约，再讨论复用收益。pool.acquire 使用绝对 deadline，满池最多排入 128 个等待者；关闭唤醒等待者，但不能销毁仍被租约借用的 socket。租约默认丢弃，只有完整收到并校验响应后显式 reusable。读取半条响应后归还会污染下一请求。

空闲 socket 的 EOF/残留数据探测能拒绝已知脏连接，却不能保证探测后不会立即断开，这是检查与使用之间的竞态。因此池必须继续传播下一次 I/O 错误，调用方再按业务幂等决定是否重试。TTL 控制空闲复用年龄，不与 DNS TTL 混同。第 12 章用真实连接计数和分阶段时间衡量复用成本，不预设池一定更快。

## 6. 自测与解析

1. **超时后将请求号加一重发足够吗？** 只解决响应关联；副作用去重还需稳定幂等键与服务端记录。
2. **任务被取消能立即释放 live 槽位吗？** queued 可以；running 必须等 worker 确认，否则新任务叠加到仍在执行的旧工作。
3. **池探测到连接活着为何下一 read 仍失败？** 对端可在探测后关闭，探测不与远端后续行为构成事务。
4. **Retry-After 大于剩余预算怎么办？** 放弃当前预算下的重试，向上报告；不能无视服务器建议，也不能延长原 deadline。
5. **Protobuf 未提供 left 能否当合法 0？** 本 API 不允许，因为 left 必填；optional presence 让校验能区分两者。
