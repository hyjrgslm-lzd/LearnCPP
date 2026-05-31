# 结课项目 4：mini RPC 框架

对应文档：`13-第三阶段结课-RPC框架.md`

## 项目目标

用 **stdexec + 自写 task<T> + asio** 实现一个最小但骨架完整的 RPC 系统。
重点不在协议完备性，而在同时管理：

- 协程化的 client 调用 `co_await rpc.call(req)`；
- 协程化的 server handler；
- 超时 / 取消 / 重试的组合编排；
- `async_scope` 结构化收束所有 in-flight 请求；
- sender-receiver 与协程在同一个工程中的配合。

## 固定题面

实现"远程计算服务"。协议：

- Request  : `{req_id: u32, method: string, args: [int]}`；
- Response : `{req_id: u32, result: int, status: "ok"|"timeout"|"error"|"unknown_method"}`。

至少实现五层：

1. **Protocol 层** —— 序列化 / 反序列化（`include/rpc/protocol.hpp`）；
2. **Transport 层** —— Asio TCP，单连接复用（`src/client.cpp` 内 Connection）；
3. **Client 层** —— `task<expected<Response, RpcError>> call(Request, ms)`；
4. **Server 层** —— accept 循环 + handler 注册 + 协程 dispatch；
5. **Scope 层** —— `async_scope` 管理 in-flight，禁 detach。

## 必做任务（含 6 张图要求）

1. **画 6 张图，再写代码**（强烈建议保留为项目文档的一部分）：
   - **图 1：Client 调用图** —— `co_await rpc.call(req)` 到 response 的完整对象链：
     task -> awaitable -> connect+sender -> asio async_write/read -> bridge receiver -> resume；
   - **图 2：Server handler 树** —— 协程 handler 调用子协程的关系，标注每个 co_await 点；
   - **图 3：Protocol 状态机** —— 一条 TCP 连接上请求/响应交错的状态：空闲、读 header、读 body、dispatch、写 header、写 body、复用；
   - **图 4：Cancellation 传播路径** —— client 超时 -> stop_source.request_stop() -> stop_token via environment -> server handler 检查点 -> 终止；
   - **图 5：Scope 拓扑** —— async_scope 拥有的 operation_state，scope.on_empty() 等待依赖；
   - **图 6：性能 trace** —— traced_awaitable（J-3）对一次请求打 trace，标注每阶段耗时。

2. 实现 Protocol 层（约 50-80 行）；
3. 实现 Transport 层（约 80-120 行）；
4. 实现 Client RPC 接口（约 80-120 行）；
5. 实现 Server handler（约 60-100 行）；
6. 实现 Server accept 循环（约 50-80 行）；
7. 编写测试场景：
   - 5 个并发请求：3 个 add（正常）、1 个 delay_add（100ms 超时 / 500ms 实际） -> Timeout、1 个 error_method -> ServerError；
   - 用 `async_scope` 收束；
   - 验证拿到 5 个结果（3 ok / 1 timeout / 1 error）；
8. 验证结构化收束：client 析构前 `scope.on_empty()` 返回，无 detach。

## 项目骨架

```
Capstone4_rpc_framework/
  include/rpc/
    task.hpp         # 自写 task<T>（带 symmetric transfer）
    scope.hpp        # async_scope（最小实现 / 可换 stdexec::async_scope）
    stop_token.hpp   # 协作式取消（typedef std::stop_token）
    protocol.hpp     # Request / Response / RpcError + serialize/parse
  src/
    server.cpp       # RpcServer + 三个 handler
    client.cpp       # RpcClient + PendingMap + Connection
    main.cpp         # demo driver: 5 并发请求
  CMakeLists.txt     # stage3 helper 链接 stdexec + asio
  README.md
```

## 验收点

- 5 个并发请求全部收到结果，结果分布正确（3 ok / 1 timeout / 1 error）；
- 6 张图各自标注了核心对象的拥有关系和生命周期边界；
- 没有 detach、没有全局可变状态、没有裸 thread；
- `async_scope` 的析构 / `on_empty()` 能正确等待所有 in-flight；
- 能解释：把 `rpc.call()` 返回类型从 `task<Response>` 改为 sender 时，
  client 代码哪些需要改、哪些不变；
- 能解释 cancellation 从 client 到 server handler 的完整传播路径。

## 设计约束（再次强调）

1. Client 端调用接口必须是 `co_await rpc.call(req)` 形式；
2. Server handler 必须是协程；
3. 至少包含 value / timeout / error 三种完成路径；
4. 所有 in-flight 请求由 `async_scope` 管理；
5. **不允许全局可变状态**存储请求计数 / 连接池 / pending map；
6. **必须**画 6 张图；
7. Client 至少 5 并发请求，含 1 个超时、1 个 error。

## 提示

- 先用单连接跑通一个请求-响应全路径，再加并发；
- request_id 用自增整数即可（4 字节够）；
- 序列化推荐 JSON 单行 + `\n` 分隔；如需性能对比，预留 Serializer 接口；
- 超时实现：`stdexec::when_any(call_sender, timer_sender)`，注意 loser 的取消行为；
- 卡点最常见 3 个：① Asio `awaitable<T>` 与自写 `task<T>` 桥接（见 H-1 as_awaitable）；
  ② PendingMap 的线程安全（建议整个 RPC 跑在同一个 io_context）；
  ③ 连接断开后清理（必须 `pending.fail_all(ConnectionLost)`）。

## 进阶任务（可选）

- 重试策略：超时后自动重试最多 2 次，每次单独受超时控制；
- 请求级 cancellation：client 超时后通过 TCP 发送 cancel 帧，server 主动 destroy handler；
- 用 traced_awaitable 收集 latencies，输出 p50 / p99 / max；
- 对比实现：纯 sender-receiver 版（不用 task），观察代码行数与可读性差异；
- transport 层从 TCP 替换为 Unix domain socket 或 in-memory channel。
