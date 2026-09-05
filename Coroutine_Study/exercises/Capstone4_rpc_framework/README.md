# 结课项目 4：mini RPC 框架

对应文档：`13-第三阶段结课-RPC框架.md`

## 项目目标

用 **纯 Asio awaitable** 实现一个最小但骨架完整的 RPC reference；自写 `task<T>` 与 stdexec bridge 放在模块 H / Capstone5 中练。
重点不在协议完备性，而在同时管理：

- 协程化的 client 调用 `co_await rpc.call(req)`；
- 协程化的 server handler；
- 超时 / 取消 / 重试的组合编排；
- `co_spawn` 后台协程的 completion handler / future 所有权；
- client/server `in_flight()` 归零，证明 shutdown drain 完整。

## 固定题面

实现"远程计算服务"。协议：

- Request  : `{req_id: u32, method: string, args: [int]}`；
- Response : `{req_id: u32, result: int, status: "ok"|"error"|"unknown_method"}`；timeout 是 client 本地 `RpcError`。

至少实现五层：

1. **Protocol 层** —— 序列化 / 反序列化（`include/rpc/protocol.hpp`）；
2. **Transport 层** —— Asio TCP，单连接复用（`src/client.cpp` 内 Connection）；
3. **Client 层** —— `asio::awaitable<std::expected<Response, RpcError>> call(Request, ms, retries)`；
4. **Server 层** —— accept 循环 + handler 注册 + 协程 dispatch；
5. **Drain 层** —— completion handler 维护 `in_flight`，禁 `asio::detached` 和裸 `detach()`。

## 必做任务（含 6 张图要求）

1. **画 6 张图，再写代码**（强烈建议保留为项目文档的一部分）：
   - **图 1：Client 调用图** —— `co_await rpc.call(req)` 到 response 的完整对象链：
     Asio awaitable -> pending map -> queued writer -> `async_write` / read loop -> timer -> resume；
   - **图 2：Server handler 树** —— 协程 handler 调用子协程的关系，标注每个 co_await 点；
   - **图 3：Protocol 状态机** —— 一条 TCP 连接上请求/响应交错的状态：空闲、读 header、读 body、dispatch、写 header、写 body、复用；
   - **图 4：Cancellation 传播路径** —— client timer 超时 -> pending map 移除 -> cancel frame -> server handler 检查点 -> 终止；
   - **图 5：Drain 拓扑** —— accept loop、connection loop、writer loop、handler、client read loop 如何维护 `in_flight`；
   - **图 6：性能 trace** —— traced_awaitable（J-3）对一次请求打 trace，标注每阶段耗时。

2. 实现 Protocol 层（约 50-80 行）；
3. 实现 Transport 层（约 80-120 行）；
4. 实现 Client RPC 接口（约 80-120 行）；
5. 实现 Server handler（约 60-100 行）；
6. 实现 Server accept 循环（约 50-80 行）；
7. 编写测试场景：
   - 6 个并发请求：3 个 add（正常）、1 个 delay_add（100ms 超时 / 500ms 实际） -> Timeout、1 个 error_method -> ServerError、1 个 missing -> UnknownMethod；
   - shutdown 后等待 client/server `in_flight()` 归零；
   - 验证拿到 6 个结果（3 ok / 1 timeout / 1 server_error / 1 unknown_method）；
8. 验证结构化收束：`io_context` 停止前 client/server `in_flight()` 都为 0，无 detached 协程。

## 项目骨架

```
Capstone4_rpc_framework/
  include/rpc/
    task.hpp         # starter 练习占位；reference 不使用自写 task
    scope.hpp        # starter 练习占位；reference 用 in_flight drain
    stop_token.hpp   # starter 练习占位；reference 用 cancel frame
    protocol.hpp     # Request / Response / RpcError + serialize/parse
  src/
    server.cpp       # RpcServer + 三个 handler
    client.cpp       # RpcClient + PendingMap + Connection
    main.cpp         # demo driver: 6 并发请求
  CMakeLists.txt     # 启用 ASIO starter/reference target
  README.md
```

`src/` 是学生 starter，只保证能编译，不输出假成功结果。`reference/` 是可运行答案：

```powershell
cmake -S Coroutine_Study/exercises -B Coroutine_Study/exercises/build/capstone4-asio -DCOROUTINE_STUDY_ENABLE_ASIO=ON -DCOROUTINE_STUDY_BUILD_REFERENCE=ON -DCOROUTINE_STUDY_FETCH_DEPS=ON
cmake --build Coroutine_Study/exercises/build/capstone4-asio --config Release --target Capstone4_rpc_framework_reference
ctest --test-dir Coroutine_Study/exercises/build/capstone4-asio -C Release -R Capstone4_rpc_framework_reference --output-on-failure
```

## 验收点

- 6 个并发请求全部收到结果，结果分布正确（3 ok / 1 timeout / 1 server_error / 1 unknown_method）；
- 6 张图各自标注了核心对象的拥有关系和生命周期边界；
- 没有 `asio::detached`、没有全局可变状态、没有裸 `detach()`；
- `shutdown()/stop()` 后 client/server `in_flight()` 都归零；
- 能解释：把 `rpc.call()` 返回类型从 Asio awaitable 改为 sender 时，
  client 代码哪些需要改、哪些不变；
- 能解释 cancellation 从 client 到 server handler 的完整传播路径。

## 设计约束（再次强调）

1. Client 端调用接口必须是 `co_await rpc.call(req, timeout, retries)` 形式；
2. Server handler 必须是协程；
3. 至少包含 value / timeout / error 三种完成路径；
4. 所有 in-flight 请求和后台协程都必须可观察并在 shutdown 时 drain；
5. **不允许全局可变状态**存储请求计数 / 连接池 / pending map；
6. **必须**画 6 张图；
7. Client reference 发起 6 个并发请求，含 1 个超时、1 个 server_error、1 个 unknown_method。

## 提示

- 先用单连接跑通一个请求-响应全路径，再加并发；
- request_id 用自增整数即可（4 字节够）；
- 序列化推荐 JSON 单行 + `\n` 分隔；如需性能对比，预留 Serializer 接口；
- 超时实现：reference 使用 `asio::steady_timer`；如果另写 stdexec 对比版，再单独验证 loser 取消行为；
- 卡点最常见 3 个：① `co_spawn` 后台协程必须有 completion handler 或 future 所有者；
  ② PendingMap 的线程安全（建议整个 RPC 跑在同一个 io_context）；
  ③ 连接断开后清理（必须 `pending.fail_all(ConnectionLost)`）。

## 进阶任务（可选）

- 重试策略：超时后自动重试最多 2 次，每次单独受超时控制；
- 请求级 cancellation：client 超时后通过 TCP 发送 cancel 帧，server handler 在检查点协作返回；不要 destroy 正在运行或可能恢复的 handler；
- 用 traced_awaitable 收集 latencies，输出 p50 / p99 / max；
- 对比实现：纯 sender-receiver 版（不用 task），观察代码行数与可读性差异；
- transport 层从 TCP 替换为 Unix domain socket 或 in-memory channel。
