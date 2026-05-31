# 13 第三阶段结课：RPC 框架

## 这份文件怎么用

模块 H 到 J 让你掌握了协程与 sender-receiver 桥接、真实异步 I/O 对接、以及协程陷阱诊断。这份文件负责把这三项能力组合成一个完整但不庞大的工程实体：一个最小但骨架完整的 RPC 框架。

本文件包含 1 个结课项目：

1. `mini RPC 框架`

完成后，你将有一条端到端的协程-网络-并发工作链，从 client 发出请求到 server 协程处理再到响应返回，中间包含超时、取消、重试、结构化收束等生产级关注点。

---

## 结课项目 4：mini RPC 框架

### 项目目标

用 stdexec + 自写 task<T> + asio（或 io_uring）实现一个最小的 RPC 框架。这个项目的重点不是协议完备性，而是让你同时管理：

- 协程化的 client 调用（`co_await rpc.call(req)` 拿 response）
- 协程化的 server handler
- 超时、取消、重试的组合编排
- 结构化并发收束所有 in-flight 请求
- sender-receiver 与协程在同一个工程中的配合

### 固定题面

实现一个"远程计算服务"的最小 RPC 系统。协议固定为：client 发送 `{method: string, args: [int]}`，server 返回 `{result: int, status: string}`。

你至少要实现以下组件：

1. **Protocol 层**：消息的序列化/反序列化（用最简单的 JSON 或自定义紧凑格式即可，不是重点）
2. **Transport 层**：Asio TCP 或 Unix socket，单连接复用（多请求共享同一连接）
3. **Client 层**：`rpc.call(Request) -> task<Response>`，支持超时、重试
4. **Server 层**：接收请求 -> 分发 handler 协程 -> 返回响应
5. **Scope 层**：`async_scope` 管理所有 in-flight 请求，析构前确保收束

### 必须包含的设计约束

1. Client 端调用接口必须是 `co_await rpc.call(req)` 的形式，返回 `Response` 或 `std::expected<Response, Error>`。
2. Server handler 必须是协程：接收 `Request`，协程内部可能继续 `co_await` 其他异步资源，最后 `co_return Response`。
3. 至少包含以下三种完成路径：
   - 正常返回（value completion）
   - 超时（timeout -> stopped 或专用的超时错误）
   - 内部错误（error completion，如非法 method name）
4. 必须使用 `async_scope` 管理所有 in-flight 请求，禁止 detach 任何协程。
5. 不允许使用全局可变状态存储请求计数、连接池或 pending map。
6. 必须画 6 张图（见下方必做任务）。
7. Client 至少发起 5 个并发请求，其中 1 个超时，1 个触发 error completion。

### 推荐项目骨架

在开始写代码之前，建议先建立以下文件结构：

```
mini_rpc/
  protocol.h        -- Request/Response 类型定义 + 序列化
  connection.h      -- Transport: 连接管理 + 发送/接收
  rpc_client.h      -- Client: call() 接口 + pending map + 超时
  rpc_server.h      -- Server: handler 注册 + accept 循环
  handlers.h        -- add / delay_add / error_method 三个 handler
  main_test.cpp     -- 测试场景: 5 并发请求
```

每个头文件应该是独立的、可单独理解的设计单元。不要把所有东西塞到一个文件里——这不是"快写完"的策略，而是"以后看不懂"的策略。

### 必做任务

1. **画 6 张图**，再写代码：
   - **图 1：Client 调用图** -- 从 `co_await rpc.call(req)` 到拿到 response 的完整对象链：task -> awaitable -> connect+sender -> asio async_write/read -> bridge receiver -> resume。
   - **图 2：Server handler 树** -- 一个协程 handler 可能调用其他子协程，画出调用-被调用关系，标注每个 co_await 点。
   - **图 3：Protocol 状态机** -- 一条 TCP 连接上，请求/响应的交错状态：空闲、等待请求头、读取请求体、分发 handler、发送响应头、发送响应体、空闲（复用连接）。
   - **图 4：Cancellation 传播路径** -- 从 client 超时触发 stop_token.request_stop()，到 stop_token 如何通过 environment 传播到 server handler、最终如何终止 handler 执行。
   - **图 5：Scope 拓扑** -- async_scope 拥有哪些 operation_state，scope.on_empty() 或析构等待的依赖关系，以及每个 operation_state 的生命周期结束点。
   - **图 6：性能 trace** -- 用 traced_awaitable（模块 J-3）对一次请求的完整路径打 trace：从 `co_await call()` 到 `await_resume` 返回，标注每个阶段的耗时。

2. **实现 Protocol 层**（约 50-80 行）：
   - 定义 `Request { std::string method; std::vector<int> args; }`
   - 定义 `Response { int result; std::string status; }`（status 可以是 "ok" / "timeout" / "error"）
   - 实现 `serialize(Request) -> std::string` 和 `deserialize(std::string) -> std::expected<Response, Error>`
   - 最简单的实现：用 `std::format` 和 `std::sscanf` 或手工字符串拼接。本项目的重点不是序列化框架。

3. **实现 Transport 层**（约 80-120 行）：
   - 用 Asio TCP 建立连接（如果选 io_uring 路线，用 liburing 的固定文件描述符操作）
   - 连接复用：一个 connection 对象维护一个发送队列和一个未完成请求映射表（`std::unordered_map<request_id, promise*>` 或等价的 completion 存储）
   - `async_read_response(connection, request_id) -> sender / awaitable`：从 TCP 流中读取对应请求的响应

4. **实现 Client RPC 接口**（约 80-120 行）：
   ```cpp
   task<std::expected<Response, RpcError>> call(Request req, 
           std::chrono::milliseconds timeout = 3s);
   ```
   - 内部逻辑：分配 request_id -> 序列化 -> 发送 -> 注册 pending -> `co_await` 响应或超时
   - 超时用 `when_any(call_sender, timer_sender)` 或等价的 with_timeout 组合
   - 超时后：取消 in-flight 请求、从 pending map 中移除、返回 `RpcError::Timeout`
   - 如果 server 返回 error status，返回 `RpcError::ServerError{message}`

5. **实现 Server handler**（约 60-100 行）：
   - 定义一个 `method_registry`：`std::unordered_map<std::string, std::function<task<Response>(Request)>>`
   - 每个注册的 handler 是一个返回 `task<Response>` 的协程
   - 至少注册 3 个 handler：
     - `add`：`co_return Response{a+b, "ok"}`（同步计算，但以协程形式返回）
     - `delay_add`：`co_await asio_timer(delay); co_return Response{a+b, "ok"}`（异步等待后计算）
     - `error_method`：抛出或返回 error status

6. **实现 Server accept 循环**（约 50-80 行）：
   ```cpp
   task<void> server_loop(asio::ip::tcp::acceptor& acceptor, async_scope& scope) {
       while (true) {
           // use_awaitable 为 Asio 标准 completion token；如使用自写 task<T>，
           // 需自行实现一个 completion token 适配器（类似模块 H-1 的 as_awaitable）。
           auto socket = co_await acceptor.async_accept(asio::use_awaitable);
           scope.spawn(handle_connection(std::move(socket)));
       }
   }
   ```
   - 每个连接在 scope 中 spawn，由 scope 管理生命周期
   - `handle_connection` 内部循环：读请求 -> 查找 handler -> 调用 handler -> 写响应

7. **编写测试场景**：
   - 启动 server 在后台线程
   - Client 同时发起 5 个并发请求（用 `when_all` 或 scope.spawn + 计数）
   - 其中 1 个请求设置 100ms 超时，而 handler 需要 500ms -> 触发超时路径
   - 其中 1 个请求调用 `error_method` -> 触发 error completion 路径
   - 其余 3 个正常完成
   - 用 `async_scope` 收束所有 in-flight 请求
   - 验证：最终拿到 5 个结果（3 个 ok、1 个 timeout、1 个 error）

8. **验证结构化收束**：在 client 析构前确保 scope.on_empty() 返回，没有协程被 detach。

### 关键类型参考

以下是最小 RPC 框架的核心类型定义参考。它不是完整的可运行代码，而是每层的"接口形状"——确保你开始编码前，每层都知道自己暴露什么、消费什么。

**Protocol 层接口形状**：

```cpp
// protocol.h
struct Request {
    uint32_t req_id;
    std::string method;
    std::vector<int> args;
};

struct Response {
    uint32_t req_id;
    int result;
    std::string status;  // "ok" | "timeout" | "error"
};

enum class RpcError {
    Timeout,
    ServerError,
    ConnectionLost,
    UnknownMethod
};

// 序列化：将请求编码为 wire format
std::string serialize(const Request& req);
// 反序列化：从 wire format 解码响应。status 非 "ok" 时 result 无效。
std::expected<Response, RpcError> deserialize(const std::string& wire);
```

**Transport 层接口形状**：

```cpp
// connection.h
class Connection {
public:
    // 异步发送请求，返回 sender（或 awaitable）完成时表示数据已写出
    auto async_send(const std::string& data) -> /* sender<void> */;

    // 异步读取一个响应帧，返回 sender<string>
    auto async_read_response() -> /* sender<std::string> */;
};

// 手动管理一个 request_id -> promise 的 pending map
// 每个 in-flight 请求在此注册，完成后从中移除
class PendingMap {
    std::mutex mtx_;
    std::unordered_map<uint32_t, /* promise 或 callback */> map_;
public:
    void register_request(uint32_t id, /* handler */);
    void complete_request(uint32_t id, Response resp);
    void cancel_request(uint32_t id);
};
```

**Client 接口形状**：

```cpp
// rpc_client.h
class RpcClient {
    Connection conn_;
    PendingMap pending_;
    std::atomic<uint32_t> next_id_{0};
    async_scope scope_;  // 管理所有 in-flight request 的生命周期
public:
    // 发起一个 RPC 调用。caller 必须 co_await 返回值。
    task<std::expected<Response, RpcError>> call(Request req,
                   std::chrono::milliseconds timeout = 3s);

    // 等待所有 in-flight 请求完成（由 scope 驱动）
    auto on_empty() { return scope_.on_empty(); }

    // 关闭 client，取消所有 pending 请求
    task<void> shutdown();
};
```

**Server 接口形状**：

```cpp
// rpc_server.h
class RpcServer {
    using Handler = std::function<task<Response>(Request)>;
    std::unordered_map<std::string, Handler> handlers_;
    asio::io_context& io_ctx_;
    async_scope scope_;
public:
    // 注册一个 RPC method handler
    void register_handler(std::string method, Handler handler);

    // 启动 accept 循环。这是一个协程，由调用者 co_await。
    task<void> serve(uint16_t port);
};
```

**注意**：以上接口形状是示意性的。你的具体实现中，`/* sender<void> */` 可能替换为实际的 sender 类型（如 `stdexec::sender auto`、concept 约束的模板类型、或 `asio::awaitable<void>`）。选择哪种方案取决于你的协程 task 如何与 Asio/io_uring 对接。

### 观察点

- RPC 框架中，**request_id 是连接复用的关键**。每个连接可以同时承载多个请求-响应对，request_id 用于将响应路由回正确的等待者。
- **Client 和 Server 的协程生命周期方向相反**：client 端协程从外部发起（`co_await rpc.call()`），server 端协程在 accept 循环中被 scope.spawn 创建。两者的拥有链不能混。
- **超时和 connection lost 是两个不同的失败模式**。超时后连接可能仍然有效，connection lost 后所有 pending 请求一次性失败。在 PendingMap 中要区分这两种清理。
- `async_scope` 在 RPC 框架中同时管理两种东西：client 端的 in-flight 请求等待者和 server 端的 handler 协程。两者在 scope 的抽象下统一为"需要收束的异步操作"。
- cancellation 从 client 到 server 的传播路径是：`stop_source(超时)` -> `stop_token(environment)` -> `asio::cancellation_slot` -> `acceptor cancel` / `handler co_await stop check`。不是单一机制，而是多条机制串联。

### 常见坑

- 忘记在超时后从 PendingMap 中移除 request_id。如果后续该 request_id 的响应到达，会尝试完成一个已经不存在的等待者。
- Server handler 中 `co_await` 了长时间操作但没有检查 stop_token，导致 client 已超时、server 还在傻等。
- `async_scope` 的 `spawn` 后忘记等待 scope 清空就析构了 io_context，导致 scope 内协程访问已销毁的 io 资源。
- 连接断开后 PendingMap 中残留的 request 没有被清理，造成资源泄漏和 scope 永不等空。
- 把 connection、pending_map、scope 三者混在一个大结构体中互相依赖，导致析构顺序难以推理。应该固定：scope 在最外层，pending_map 在 connection 内。
- `when_any(call_sender, timer_sender)` 中，如果 call_sender 完成后 timer_sender 仍在运行，需要取消 timer_sender 否则它的回调可能触达已析构的对象。

### 提示

- 先用单连接开发，跑通一次请求-响应全路径，再加入并发和多连接复用。
- request_id 用自增整数即可，不需要 UUID。4 字节足够 4 billion 请求——远超单次测试。
- 序列化格式选择：JSON（`nlohmann/json` 单头文件）可读性最好，适合调试。但如果你后续要做性能对比，提前抽象 Serializer 接口，方便替换为 binary 格式。
- 超时实现：如果你用 `stdexec::when_any` 或 `stdexec::with_timeout`，确保了解它们对"loser"的取消行为。如果自行实现，一个 `asio::steady_timer` + `co_await` 是最直接的方式。
- 测试场景写成一个独立的 `main_test.cpp`，不要嵌在 library 代码里。每次验证都应该从头构造 client 和 server。

### 进阶任务

- 加入重试策略：超时后自动重试最多 2 次，每次重试单独受超时控制。用 `let_error` 或协程内部的 while 循环实现。
- 实现请求级 cancellation：当 client 超时后，通过 TCP 发送一个 cancel 帧给 server，server 收到后主动调用 `handle.destroy()` 终止 handler 协程，释放资源。
- 加入统计收集：用 traced_awaitable 收集每次请求的 latencies，输出 p50 / p99 / max。
- 做一个对比实现：纯 sender-receiver 版（不用协程 task，用 sender 图拼接所有异步操作），对比代码行数和可读性。
- 把 transport 层从 TCP 替换为 Unix domain socket 或 in-memory channel，观察接口抽象层的稳定性。
- Server 端实现一个轻量的"method 不存在"统一兜底——不注册任何 handler 的 method 被调用时不崩溃、不留 pending，而是返回带有描述性 `status: "unknown_method"` 的 Response。这样做的好处是：client 不需要为每个新 method 更新错误处理，错误路径统一。

### 实现过程中你可能会卡住的地方

以下三个点是之前练习者在做这个项目时最常见的阻塞点。如果你卡住了，先检查是不是这些问题：

1. **Asio 协程与 stdexec task 的对接**：`asio::awaitable<T>` 和 `stdexec::task<T>` 是两个不同的协程类型。你不能在 `stdexec::task` 内部 `co_await` 一个 `asio::awaitable`，除非你写一个桥接层（类似于模块 H-1 的 as_awaitable）。最简单的做法：整个 RPC client/server 统一用一种协程类型（推荐一开始就用 Asio 的 `asio::awaitable` + `co_spawn`，因为 Asio 已经封装好了 TCP 异步操作；后续再替换为 stdexec task）。

2. **PendingMap 的线程安全**：如果 server handler 在 io_context 的单线程上执行，而 client 端的超时 timer 也在同一个 io_context 上，没有线程安全问题。但如果你的超时 timer 在另一个线程触发，PendingMap 必须加 mutex。一个简单策略：整个 RPC 系统都运行在同一个 io_context 上，避免多线程复杂度。

3. **连接断开后的清理**：TCP 连接意外断开时，PendingMap 中所有该连接的 pending request 都需要被通知（以 `RpcError::ConnectionLost` 完成）。如果你忘了这一条，`async_scope` 永远等不到空，程序卡死。

### 验收点

- 5 个并发请求全部收到结果，包含 3 个正常、1 个超时、1 个 error，结果正确。
- 你能指出每张图中最关键的 3 个节点。
- 你没有使用 detach、全局可变状态、裸 thread 互斥。
- `async_scope` 的析构或 `on_empty()` 能正确等待所有 in-flight 请求。
- 你能解释：如果你把 `rpc.call()` 的返回值从 `task<Response>` 改为 sender，client 代码需要改哪些地方、哪些不变。
- 你能解释：cancellation token 从 client 到 server handler 的传播路径中，每个环节分别依赖什么机制（stop_token environment / asio cancellation_slot / 自定义检查点）。

### 项目复盘问题

- 在这个项目中，协程和 sender-receiver 分别承担了什么角色？哪些部分用协程更自然？哪些部分用 sender 组合更清晰？
- 如果不用 `async_scope`，in-flight 请求的生命周期管理会有多复杂？写出一个反例版本的大致结构。
- 超时和重试的组合逻辑中，哪一部分最容易出现资源泄漏（allocated request_id 永不被回收、handler 协程永不被销毁）？
- protocol 状态机中，哪个状态转换最容易因为并发请求的交错而出 bug？
- 如果要把这套 RPC 框架扩展为支持 streaming（server 推送多条响应），哪些接口和对象关系需要改动？
- 完成这个项目后，你对"协程是表达层工具，不是调度/生命周期的替代品"这句话的理解深了多少？

### 完成后你能说什么

至少把下面几句话说顺：

- 我用 stdexec + 自写 task<T> + asio 实现了一个完整的最小 RPC 框架，包含 client 端的 `co_await rpc.call(req)` 获取 response、server 端的协程 handler、以及超时/取消/重试。
- Client 端 5 个并发请求中，3 个正常完成、1 个超时、1 个 error completion，全部由 `async_scope` 结构化收束。
- 我画了 6 张图：client 调用图、server handler 树、protocol 状态机、cancellation 传播路径、scope 拓扑、性能 trace。每张图都标注了核心对象的拥有关系和生命周期边界。
- 整套系统没有 detach、没有全局可变状态、没有裸 thread。所有协程都被 scope 或 sync_wait 拥有。
- 我能解释 cancellation 从 stop_token 到 server handler 的完整传播路径，以及每个环节依赖的机制。
- 这个项目让我真正理解了：协程让异步代码读起来像同步，但底层的调度、生命周期、错误传播、取消取消机制一个都没少——它们只是被正确地组织在类型系统和 scope 拥有关系中，而不是被隐藏。