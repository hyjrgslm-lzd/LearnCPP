# 12 跨课桥接、源码阅读与可归因测量

本章在网络和异步两侧基础之后进入。目标是从已有库协议出发，解释实际 socket 完成怎样安全地成为协程恢复或 sender 完成，再检查优化是否真的改变所定位的成本。

## 1. C07、C09、C10 各保留什么

C07 主讲句柄/描述符、readiness/completion 和系统调用，C11 增加连接字节序、协议状态、背压、半关闭与服务接纳。C09 主讲协程语言协议，原 RPC 的 wire/API/Student 保留。C10 主讲 sender/receiver、环境与完成通道，文件 I/O 后端不能换一个函数名就叫网络桥接。

[L11](../exercises/L11_bridges/README.md)的 legacy 目标直接编译未修改的 C09 good client/protocol，以 standalone Asio1.38.2 运行客户端；服务端使用 C11 native socket 和增量分帧独立检查字节。实际路径覆盖 add→42、unknown_method、server error、超时后 C|id 取消。服务端固定请求脚本是互操作 oracle，不是通用 RPC server。

同一目标只使用 standalone Asio，不把 Boost.Asio 的 tcp::socket、error_code、awaitable 传进去。即使 API 看起来相似，它们也是不同类型和实现配置。C11 新服务的任务 ID、幂等键和 schema 是独立协议；兼容入口明确测试旧契约，而非声称旧客户端能调用新 HTTP API。

## 2. 一个真实网络 receive sender

`network_sender.hpp` 定义一次最多 32 字节的 async_read_some，不是完整帧读取。connect 构造 operation state，start 注册 receiver 环境中的 stop token 并 post 到 socket executor。共享 state 拥有 socket、接收数组、receiver、timer 和停止回调；完成处理捕获该 state，使 operation 对象在接收完成信号后可被调用者销毁。

完成签名为 value(string)、error(exception_ptr)、stopped。成功读取将拥有的 string 交给 receiver；EOF/系统错误走 error；停止请求导致的取消走 stopped。停止请求先于 start 时直接 stopped。成功读取已经发生而 stop 后到时保留数据，不用停止意图擦掉成功结果。socket executor 必须串行执行本操作的 timer/read handler（单个 io.run owner 或 strand）。调用者在操作完成前必须独占该 socket 的读/取消责任，socket.cancel 会影响同 socket 其他操作，不能偷偷支持多个互不相关的并行 read。

stop callback 可从任意线程运行，只设置 atomic stop；5 ms owner timer 观察并发起 socket.cancel。这个明确的轮询上限省去跨线程取消投递的异常路径，代价是至多一个轮询周期左右的调度等待，实际延迟还受 owner 调度影响。2 秒内部预算最终取消未完成 read；它不是实时保证。

read 完成后取消 timer，并等 timer handler 也返回才发送最终信号。若 timer 先触发取消则等待 read handler，不能在 timer 回调里立即 set_stopped 并释放数组。最终完成前 reset stop callback，与可能正在执行的停止回调同步；receiver 完成可能销毁 operation，所以发送信号之后不再访问 operation 成员。三个通道由真实 socket 数据、EOF、活动取消和提前取消检查。

## 3. 按状态追源码

固定版本和可点击入口见[来源索引](../references/standards-and-implementations.md)。每条导读都沿“发起 → 内部状态 → 事件 → 完成/错误 → 释放”阅读，不能只找同名函数。

- **Asio**：从 async_read 的 initiation 到 composed read_op，观察总完成量如何积累，关联 executor/cancellation 如何传播，再进入 Windows socket service 的 overlapped 操作。对照 L06，区分算法组合状态和操作系统请求状态。
- **Beast**：从 HTTP async_read 到 parser 的消费循环，观察 partial message、body limit 和 buffer 剩余量；从 websocket write_op 到写锁、控制帧协调与 close，解释 P1 单写者为何必要。
- **nghttp2**：从 mem_recv2 的 frame header 状态到 stream/window 更新，再观察 on_frame_recv 与 on_stream_close 的时序；从 data provider 返回 EOF 到 END_STREAM 与 payload 释放。
- **MsQuic**：StreamSend 进入发送请求队列，stream_send.c 消费 buffer；stream_recv.c 管理接收偏移与 pending completion；loss_detection.c 管理包的确认/丢失。追踪 CANCEL/RESET 时 SEND_COMPLETE 与 SHUTDOWN_COMPLETE 的先后责任。
- **gRPC**：从生成 Stub 的 Submit 到 ClientContext deadline/call_op_set，再到 HTTP2 transport；服务端同步线程进入自写 invoke，再切回 owner。不要把 gRPC 自己的 transport worker 当作本课两个计算 worker。

阅读作业：为每条路径写出一个“库仍可能访问应用内存”的最后状态和一个失败出口。解析是：Asio handler、Beast composed operation、nghttp2 data source、MsQuic send buffer 和 gRPC ServerContext 各有不同释放事件；所有路径都不能把发起函数返回成功当作最后访问。

## 4. 先定位连接建立成本，再决定是否复用

[B01](../exercises/B01_pool_cost/README.md)使用同一 Reactor、128 字节 body、100 次顺序请求，比较每次新连接与同一池租约复用。两者都核对精确响应，只有完整请求成功后才记为有效回合。它是连接策略对照，不把异步、多线程或不同消息大小混进同一加速比。

每回合记录 acquire_us、transfer_us、work_us、drain_us 和连接创建数。初始化 socket runtime、监听器、owner 线程在 work 区间外；work 包含每轮获取、发送接收、租约归还或 socket 析构；drain 单独计。acquire 对 fresh 是 connect，对 pool 是池查找/探测/可能 connect，所以这是两种策略真实成本，并非同一函数的微基准。

预测是复用把连接创建数从 N 降为 1，并可能减少 acquisition 总时间。第一项是可检查机制事实，第二项必须看测量；如果 transfer 占绝大多数，端到端收益可能很小。如果某次 fresh 更快，先检查排队、调度、单回合噪声、池探测开销，不删除反常样本来保持叙事。

脚本默认每方案一个预热和五个独立进程样本，各有外部超时，失败立即保存并不进入统计。输出中位数；完整样本保存在用户指定的 build 文件，可计算范围与分位数。课程不固化本机数字，也不把 loopback 结果外推到广域网或 TLS 建连成本。

## 5. 延迟与吞吐实验的升级边界

顺序闭环请求反映一个客户端的服务时间，不代表固定到达率下的排队尾延迟。开放负载应记录计划发送时刻与实际开始时刻，避免服务变慢导致生成器也变慢、漏掉本该排队的请求。限制 outstanding、总请求量和运行时间，超过上限记录拒绝而非继续分配。

若要定位高并发尾延迟，先拆 pool wait、connect、TLS、parse、queue、execute、write 的时间和队列深度，再用 profiler/计数器验证候选。只看总耗时下降不能证明锁、cache miss 或系统调用是根因。关闭日志后正式测量，诊断插桩与计时条件分开，保留没有收益的结果。当前交付按 Windows 编译与关键行为验收，不要求安装额外分析工具。

## 6. 自测与解析

1. **sender 返回 string_view 可少一次拷贝吗？** 可能，但本接口完成后允许 operation 销毁，内部数组随之消失；改为借用值就必须改变外部存活契约。拥有 string 是清晰基线。
2. **连接创建减少就能写“吞吐提升 N 倍”吗？** 不能，减少次数是机制证据，吞吐还受数据传输、调度和工作量影响。
3. **Linux 专属源存在是否算本次通过？** 不算；本次只验证 Windows，源码和复现条件是后续学习入口。
4. **C09 协程自动获得新服务的幂等性吗？** 不会；控制流形式与服务协议保证独立，需要新协议或显式适配。
