# 13 第三阶段结课：RPC 框架

这个结课项目把 H/I/J 串起来：用纯 Asio `awaitable` 写一个最小 RPC 框架，覆盖请求协议、client/server 协程、超时、取消帧、重试、连接断开和 shutdown drain。自写 `task<T>` 与 stdexec bridge 放在模块 H 和 Capstone5；本项目的 reference 只使用 Asio，减少模型混杂。

仓库稳定 ID：`Capstone4_rpc_framework`。`src/` 是学生 starter，只保证 compile-only；`reference/` 是可运行答案。项目完成以 reference 行为和你补完后的 starter 行为为准。

## 1. 先看协议

reference 的 wire format 是 8 字节十进制长度头 + body：

```text
00000013Q|1|1|add|1,2
00000003C|1
00000008R|1|3|ok
```

`Q` 是 request：`id | idempotent | method | args`。`C` 是 cancel frame：只带 request id。`R` 是 response：`id | result | status`。长度头让 TCP 粘包/拆包不影响解析；`request_id` 让同一连接上多个请求能够复用。

关键代码：`reference/include/rpc_ref/rpc.hpp` 中的 `frame`、`encode(request)`、`encode_cancel`、`encode(response)`、`read_frame`、`parse_request`、`parse_response`。

预测协议检查：

- `parse_request("Q|1|1|add|x")` 失败，因为 args 无法解析为 int。
  **答案解析：** request body 拆成 5 个字段后，`split_args("x")` 会调用 `parse_int`。`std::from_chars` 没有完整消费出整数，reference 返回 `error::protocol`，所以这属于协议格式错误，尚未进入业务 `dispatch`。
- `parse_response("R|bad|0|ok")` 失败，因为 id 无法解析为 u32。
  **答案解析：** response 的第二个字段必须是十进制 `uint32_t` request id。`bad` 无法通过 `parse_u32`，client 读 loop 会把它当 protocol error，并通过 `fail_all(protocol)` 唤醒所有 pending call。
- body 大于 `max_frame_size` 会返回 protocol error。
  **答案解析：** `frame(body)` 和 `read_frame` 都检查长度上限，reference 的上限是 4096 字节。这个边界保护解析和内存分配，避免对端发送超大长度后让本地无界分配。
- 读到非数字长度头会返回 protocol error。
  **答案解析：** `read_frame` 先固定读取 8 字节 header，并逐字节检查 `std::isdigit`。只要 header 中出现非数字字符，就不会继续按长度读 body，而是直接返回 `error::protocol`。

## 2. Transport：读帧与 queued writer

Asio TCP socket 是字节流。`read_frame(socket)` 先 `async_read` 固定 8 字节 header，再按长度读 body。错误用 `std::expected<..., error>` 返回：EOF/断连映射到 `connection_lost`，格式错误映射到 `protocol`。

写侧由 `queued_writer` 串行化。多个 handler 可以同时想写响应；直接并发 `async_write` 到同一 socket 会让帧交错。writer 维护一个 `deque<string>`，第一次入队时 `co_spawn(write_loop)`，loop 逐帧写完再退出。`in_flight_` 在 writer loop 生命周期内加一减一。

```text
writer.send(frame)
  -> push_back
  -> if idle: co_spawn(write_loop)

write_loop
  -> pop front
  -> async_write(socket, frame)
  -> loop until queue empty
  -> writing = false
```

## 3. Client：pending map、timer 和重试

`client::call(req, timeout, retries)` 是项目的主入口。它返回 `asio::awaitable<std::expected<response, error>>`，调用者通过 `co_await` 等结果。

单次 attempt 的流程：

```text
req.id = ++next_id
encode(req)
state = pending_state{timer}
pending[id] = state
writer.send(frame)
timer.expires_after(timeout)
co_await timer.async_wait(redirect_error)
```

这里的 timer 既代表超时，也代表“响应已到”的唤醒点。read loop 收到 response 后写 `state->result`，取消 timer；于是 `async_wait` 以 `operation_aborted` 返回，`call()` 读取结果并把 server status 映射成 `ok / unknown_method / server_error`。

如果 timer 正常到点，说明响应没在 deadline 前到达。client 会从 pending map 移除 id，发送 cancel frame。若请求缺少 idempotent 标记或 retry 次数已用完，返回 `timeout`；若还能重试，进入下一轮 attempt，使用新的 request id。

连接断开时 `read_loop` 调 `fail_all(error)`：给所有 pending 写入同一个错误，取消它们的 timer，然后清空 map。这样等待中的 `call()` 都能醒来，不会永久挂起。

## 4. Server：accept、connection、handler

server 启动时创建 `tcp::acceptor` 并 `co_spawn(accept_loop)`。accept loop 每接到一个 socket，就 `co_spawn(handle_connection)`，completion handler 减少 `in_flight_`。

connection loop 反复读帧：

```text
read_frame
  -> C|id: 标记 cancel[id] = true
  -> Q|...: parse request
       create cancel_state
       co_spawn(handle_request(writer, req, cancel_state))
  -> error: cancel_all and return
```

`handle_request` 调 `dispatch(req, cancel)`。`add` 同步求和但仍以协程形式返回；`delay_add` 每 10ms 设一个 timer，并在每个检查点看 `cancel->cancelled`；`error_method` 返回 `status="error"`；未知 method 返回 `status="unknown_method"`。

取消走协作式路径。client 超时后发送 `C|id`；server connection loop 收到后只设置 cancel flag；handler 在检查点自然返回，不写响应。

## 5. Drain：证明后台协程都收束

本项目要求后台工作可观察。reference 每条后台路径都维护 `in_flight_`：

- server accept loop：`server.start()` 加一，accept loop 结束减一。
- server connection loop：每个 accepted socket 加一，connection handler 结束减一。
- server request handler：每个 request 加一，handler completion 减一。
- writer loop：从 idle 变 writing 时加一，队列清空减一。
- client read loop：connect 后加一，read loop 退出减一。

shutdown 时 client 先 `fail_all(connection_lost)` 并 close socket，server close acceptor。`io_context` 的 work guard 释放后，runner thread 退出。测试最后断言 `client.in_flight() == 0 && server.in_flight() == 0`。

## 6. 必画的 6 张图

写代码前先画，写完后按 reference 修正：

1. Client 调用图：`co_await call`、pending map、writer、read loop、timer、结果返回。
   **答案解析：** 一次 call 会分配新 id，把 `pending_state{timer,result}` 放进 map，再通过 queued writer 写 request。response 先到时 read loop 写 result 并 cancel timer；timer 先到时 call 移除 pending、发 cancel frame，并按 idempotent/retry 决定重试或 timeout。
2. Server handler 树：accept loop、connection loop、request handler、`dispatch`、timer await。
   **答案解析：** server start 拥有 accept loop；每个 socket 进入 connection loop；每个 request 再独立 `co_spawn(handle_request)`。`dispatch` 里 `delay_add` 用 timer 分段等待并检查 cancel flag，`add/error/unknown` 则直接形成 response status。
3. Protocol 状态机：读 header、读 body、parse、dispatch、写 response、复用连接。
   **答案解析：** TCP 只提供字节流，所以状态机必须先按 8 字节长度头确定 body 边界。body 解析成 Q/C/R 后分别进入 request、cancel 或 response 路径；同一连接通过 request id 区分多次调用。
4. Cancellation 路径：client timeout、pending erase、cancel frame、server cancel_state、handler 检查点。
   **答案解析：** timeout 到点表示本地 deadline 已过，client 先移除 pending，随后发送 `C|id`。server connection loop 收到 cancel frame 后设置对应 `cancel_state`，handler 只在自己的检查点响应，因此取消是协议驱动的协作式停止。
5. Drain 拓扑：每个 `co_spawn` 对应的 in-flight 加减。
   **答案解析：** reference 对 accept loop、connection loop、request handler、writer loop、client read loop 都维护 `in_flight_`。每个后台协程启动前加一，completion handler 或 writer loop 结束后减一；测试最后断言 client/server 都为 0，证明 shutdown 没留下后台路径。
6. Trace 图：用 J-3 的 await trace 标一次 `call()` 的耗时阶段。
   **答案解析：** 可以把一次 request 标成 `call enqueue -> writer async_write -> server read_frame -> dispatch/timer -> response write -> client read_loop -> timer cancel -> await_resume`。每段记录 request id、线程/executor 和耗时，就能分辨延迟来自排队、网络读写、server handler 还是本地 timeout。

## 7. Reference 验收

```powershell
cmake -S C09_Coroutines/exercises -B C09_Coroutines/exercises/build/capstone4-asio -DCOROUTINE_STUDY_ENABLE_ASIO=ON -DCOROUTINE_STUDY_BUILD_REFERENCE=ON -DCOROUTINE_STUDY_FETCH_DEPS=ON
cmake --build C09_Coroutines/exercises/build/capstone4-asio --config Release --target Capstone4_rpc_framework_reference
ctest --test-dir C09_Coroutines/exercises/build/capstone4-asio -C Release -R Capstone4_rpc_framework_reference --output-on-failure
```

reference 发起 6 个并发请求：3 个 `add` 正常返回，1 个 `delay_add` 100ms 超时并可重试一次，1 个 `error_method` 返回 server error，1 个未知 method 返回 unknown method。测试还覆盖协议错误、连接断开 fail_all、编码过大失败和 in-flight 归零。

完成后你应该能从任意一个 request id 反向追踪：它何时分配，何时进入 pending，何时写出，response 或 timer 谁先到，pending 如何移除，server 侧 handler 是否收到 cancel，后台协程如何完成。

**答案解析：** id 在 `client::call` 每次 attempt 中递增分配，随后进入 pending map 并写出 request。正常 response 由 read loop 根据 id 找到 pending、写 result、cancel timer 并 erase；timeout 由 call 自己 erase pending 并写 cancel frame，晚到 response 会因为 `pending_.find(resp->id) == end` 被忽略。server 侧通过 cancel map 把 `C|id` 转成 handler 可见的 flag，所有 accept/read/write/handler 路径再由 `in_flight_` 归零证明收束。
