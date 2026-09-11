# 结课项目 4：mini RPC 框架

对应主讲义：`13-第三阶段结课-RPC框架.md`。

这个项目用纯 Asio `awaitable` 实现最小 RPC。`src/` 是学生 TODO 起点，公开头文件在 `include/rpc/`；`good/` 是作者提供的完整可运行实现，`reference/` 是隔离答案。不要依赖 Reference 才开始做题：先按本文件的阶段契约实现 protocol，再接 client/server，最后用 checker 验证真实请求结果和 drain 断言。

`Capstone4_rpc_framework` 是 student demo smoke。默认 TODO 起点会输出 `TODO: ...` 并返回 2；学生逐步补完 `src/` 后，同一个 demo 会自然返回 0。`Capstone4_rpc_framework_student_check` 链接 `src/`，默认 CTest 真实 FAIL，不用 `WILL_FAIL` 把原失败改成绿灯；`Capstone4_rpc_framework_answer_check` 链接 `good/` 的 reference-adapted answer 并必须通过同一 checker；`Capstone4_rpc_framework_protocol_good_check` 用独立 protocol good + answer client/server 覆盖 protocol Part；`Capstone4_rpc_framework_reference` 独立检查 Reference。开启 `COROUTINE_STUDY_TEST_STARTERS=ON` 时，CTest 会注册 student/answer/protocol-good/bad/reference 检查，超时 60 秒。

## Public API

学生主要编辑 `src/`，public headers 固定接口：

- `src/protocol.cpp`：`Request/Response/RpcError`、8 字节长度帧、`Q/C/R` body、`parse_*`。
- `src/client.cpp`：`RpcClient::connect/call/shutdown/in_flight`，pending map、timer、read loop、重试。
- `src/server.cpp`：`RpcServer::start/stop/in_flight`，accept loop、connection loop、handler、cancel map。

保留的类型名：`rpc::Request`、`rpc::Response`、`rpc::RpcError`。`Request::idempotent` 默认 `false`；`RpcClient::call(req, timeout, retries = 0)` 保持旧的两参数调用可用。

## 项目链路

```text
client.call(req, timeout, retries)
  -> request_id
  -> encode length-prefixed frame
  -> pending[id] = timer/result state
  -> queued_writer.send
  -> read_loop 收 response 或 timer 到点
  -> expected<response, error>

server
  -> accept_loop
  -> handle_connection
  -> read_frame
  -> dispatch(add/delay_add/error/unknown)
  -> queued_writer.send(response)
```

协议 body：

- `Q|id|idempotent|method|args`
- `C|id`
- `R|id|result|status`

长度头固定 8 字节十进制。这样 TCP 粘包/拆包不会破坏帧边界。

## 先画 6 张图

1. Client 调用图：`co_await rpc.call` 到 response/error。
   **答案解析：** 图中要包含 request id 分配、`pending[id]`、queued writer、read loop 和 timer。response 先到时 read loop 写 result 并 cancel timer；timeout 先到时 call erase pending、发送 cancel frame，并根据 idempotent/retry 返回 timeout 或重试。
2. Server handler 树：accept、connection、request handler、`dispatch`。
   **答案解析：** accept loop 拥有连接入口，connection loop 持续读同一 socket，request handler 按 id 独立处理请求。`dispatch` 把 `add`、`delay_add`、`error_method`、未知 method 转成 response status，`delay_add` 还要在 timer 检查点观察 cancel flag。
3. Protocol 状态机：读 header、读 body、parse、dispatch、写 frame。
   **答案解析：** 状态机从 8 字节长度头开始，长度合法后再读 body。body 的首字段决定 Q/C/R 路径，解析失败是 protocol error；写 response 时仍经过 length-prefixed frame，保持同一连接可复用。
4. Cancellation 路径：timeout、pending erase、cancel frame、server cancel flag。
   **答案解析：** client timeout 后本地 pending 已经不存在，随后写 `C|id` 通知 server。server 收到 cancel frame 只设置对应 `cancel_state`，handler 在自己的检查点停下；晚到 response 会被 client read loop 因找不到 pending 而丢弃。
5. Drain 拓扑：accept/read/write/handler 每个后台协程的 in-flight 加减。
   **答案解析：** 每个 `co_spawn` 后台路径都要有对应计数：server accept、connection、request handler、writer loop 和 client read loop。shutdown 后 client/server `in_flight()` 都为 0，才说明后台协程没有继续访问 socket、pending 或 cancel map。
6. Trace 图：用 J-3 的 trace 思路标一次请求耗时。
   **答案解析：** 参考路径可以标 `call start -> writer.send -> async_write -> server read_frame -> dispatch -> response write -> client read_loop -> timer.cancel -> call return`。每个节点附 request id 和耗时，就能定位慢在排队、server handler、网络读写还是 timeout 分支。

## Reference 覆盖

6 个并发请求：

- 3 个 `add` 正常返回。
  **答案解析：** 三个请求分别走同步求和路径，response status 为 `ok`，测试只统计 ok 个数为 3。它们覆盖同一连接上的多个并发 request id 和 response 路由。
- 1 个 `delay_add`，100ms timeout，server 侧协作取消。
  **答案解析：** client 给 `delay_add{500}` 设置 100ms deadline 且允许一次 retry；每次 timeout 都移除 pending 并发送 cancel frame。server handler 每 10ms 检查 cancel flag，看到取消后自然返回且不写 response，最终 client 统计一个 timeout。
- 1 个 `error_method`，映射到 `server_error`。
  **答案解析：** server dispatch 把 `error_method` 的 response status 设为 `"error"`。client 收到 response 后在 `call` 中把非 ok 且非 unknown 的 status 映射成 `error::server_error`。
- 1 个未知 method，映射到 `unknown_method`。
  **答案解析：** dispatch 对未识别 method 返回 status `"unknown_method"`。client 在读取 result 后把这个业务状态映射成 `error::unknown_method`，和协议解析错误、断连错误分开。

额外检查：

- protocol parse error。
  **答案解析：** 测试覆盖 request args 非整数、response id 非 u32、非法 body 等路径，确保解析失败不会进入业务 handler。client 侧协议错误会通过 `fail_all(protocol)` 唤醒 pending。
- frame 超过 `max_frame_size`。
  **答案解析：** `frame()` 编码和 `read_frame()` 解码都守住 4096 字节上限。超过上限返回 protocol error，避免无界 body 分配。
- raw server 断开连接后 `pending.fail_all(connection_lost)`。
  **答案解析：** read loop 读帧失败后遍历所有 pending，写入 `connection_lost` 并 cancel timer。等待中的 call 被唤醒后读到错误，pending map 清空，不会永久挂起。
- client/server shutdown 后 `in_flight()` 都是 0。
  **答案解析：** 这是 drain 证据，不只是清理结果。client shutdown 关闭 socket 并 fail pending，server stop 关闭 acceptor；runner 退出后计数归零，说明所有后台 read/write/handler/accept 路径都完成。

## 运行

```powershell
cmake -S C09_Coroutines/exercises -B C09_Coroutines/exercises/build/capstone4-asio -DCOROUTINE_STUDY_ENABLE_ASIO=ON -DCOROUTINE_STUDY_BUILD_REFERENCE=ON -DCOROUTINE_STUDY_FETCH_DEPS=ON
cmake --build C09_Coroutines/exercises/build/capstone4-asio --config Release --target Capstone4_rpc_framework_reference
ctest --test-dir C09_Coroutines/exercises/build/capstone4-asio -C Release -R Capstone4_rpc_framework_reference --output-on-failure
```

## 阅读顺序

1. `reference/include/rpc_ref/rpc.hpp`：先读 `frame/encode/read_frame/parse_*`。
2. `queued_writer`：看为什么同一 socket 写入要串行化。
3. `client::call`：看 timer 如何同时表示 timeout 和 response arrival 的唤醒点。
4. `client::read_loop`：看 response 如何路由到 pending state。
5. `server::accept_loop/handle_connection/handle_request`：看后台协程 ownership 和 cancel map。
6. `reference/tests/rpc_reference_test.cpp`：看 6 请求分布和 drain 断言。

实现时按五阶段推进：

1. 编码/分帧/单请求：先替换 `src/protocol.cpp` 的 TODO，令 `serialize(Request)` 产出 8 位长度头，`parse_request/parse_response` 拒绝坏字段和超长 body。
2. pending 并发：每个 `call()` 分配新 id，read loop 用 response id 唤醒正确 pending。
3. timeout/cancel：timer 先到就 erase pending，发送 `C|id`，server handler 在检查点观察 cancel flag。
4. 幂等 retry：只有 `Request::idempotent == true` 且 `attempt < retries` 才重试，并使用新 request id。
5. close/drain：`shutdown()` 可重复调用，断连唤醒全部 pending，最终 client/server `in_flight()` 都为 0。

**答案解析：** 单请求先验证协议和 dispatch；pending map 加入后验证 response 能按 id 回到正确 call；timeout 加入后验证 timer 能唤醒等待者；cancel frame 和 retry 加入后验证 loser handler 能协作停止。每一步都检查 in-flight，能及时发现 writer loop、read loop 或 handler 没有收束。
