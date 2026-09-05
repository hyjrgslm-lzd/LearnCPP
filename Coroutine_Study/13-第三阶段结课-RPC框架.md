# 13 第三阶段结课：RPC 框架

## 这份文件怎么用

模块 H 到 J 让你掌握了协程与 sender-receiver 桥接、真实异步 I/O 对接、以及协程陷阱诊断。这份文件负责把这三项能力组合成一个完整但不庞大的工程实体：一个最小但骨架完整的 RPC 框架。

本文件包含 1 个结课项目：

1. `mini RPC 框架`（仓库稳定 ID：`Capstone4_rpc_framework`；数字 4 是结课项目 ID，不随章节重排改变）

完成后，你将有一条端到端的协程-网络-并发工作链，从 client 发出请求到 server 协程处理再到响应返回，中间包含长度帧、request id 路由、超时、取消帧、重试和 shutdown drain 等生产级关注点。

仓库内有两条路径：

- `Coroutine_Study/exercises/Capstone4_rpc_framework/src` 是学生 starter，只保证 compile-only，不输出假成功结果。
- `Coroutine_Study/exercises/Capstone4_rpc_framework/reference` 是可运行 reference，覆盖长度帧、request id、pending map、deadline、协作式 cancel frame、幂等请求 retry、server error、disconnect fail_all 和 shutdown drain。

Reference 验收命令：

```powershell
cmake -S Coroutine_Study/exercises -B Coroutine_Study/exercises/build/capstone4-asio -DCOROUTINE_STUDY_ENABLE_ASIO=ON -DCOROUTINE_STUDY_BUILD_REFERENCE=ON -DCOROUTINE_STUDY_FETCH_DEPS=ON
cmake --build Coroutine_Study/exercises/build/capstone4-asio --config Release --target Capstone4_rpc_framework_reference
ctest --test-dir Coroutine_Study/exercises/build/capstone4-asio -C Release -R Capstone4_rpc_framework_reference --output-on-failure
```

---

## 结课项目 4：mini RPC 框架

### 项目目标

Reference 版本统一使用纯 Asio `awaitable` 实现最小 RPC 框架；自写 `task<T>` 与 sender/stdexec 桥接在模块 H 和 Capstone5 中练。这个项目的重点不是协议完备性，而是让你同时管理：

- 协程化的 client 调用（`co_await rpc.call(req)` 拿 response）
- 协程化的 server handler
- 超时、取消、重试的组合编排
- 用具名 completion handler 与 `in_flight` 计数收束所有后台协程
- sender-receiver 与协程桥接在相邻模块中的边界：本项目只保留纯 Asio 路径，避免同时混两套异步模型

### 固定题面

实现一个"远程计算服务"的最小 RPC 系统。协议固定为长度前缀帧：client 发送 `{req_id, idempotent, method, args}`，server 返回 `{req_id, result, status}`。status 至少包含 `"ok"`、`"error"`、`"unknown_method"`；client 本地超时返回 `RpcError::Timeout`。

你至少要实现以下组件：

1. **Protocol 层**：消息的序列化/反序列化（用最简单的 JSON 或自定义紧凑格式即可，不是重点）
2. **Transport 层**：Asio TCP，单连接复用（多请求共享同一连接）
3. **Client 层**：`rpc.call(Request, timeout, retries) -> asio::awaitable<std::expected<Response, RpcError>>`
4. **Server 层**：接收请求 -> `co_spawn` handler 协程 -> 返回响应
5. **Drain 层**：client/server 用 `in_flight` 计数和显式 `shutdown()/stop()` 验证后台协程全部收束

### 必须包含的设计约束

1. Client 端调用接口必须是 `co_await rpc.call(req, timeout, retries)` 的形式，返回 `std::expected<Response, Error>`。
2. Server handler 必须是协程：接收 `Request`，协程内部可能继续 `co_await` 其他异步资源，最后 `co_return Response`。
3. 至少包含以下三种完成路径：
   - 正常返回（value completion）
   - 超时（timeout -> stopped 或专用的超时错误）
   - 内部错误（error completion，如非法 method name）
4. 必须显式管理所有 in-flight 请求和后台协程：每次 `co_spawn` 都要有 completion handler 更新计数，shutdown 时等待计数归零；禁止 `asio::detached`、裸 `detach()` 和丢弃 future。
5. 不允许使用全局可变状态存储请求计数、连接池或 pending map。
6. 必须画 6 张图（见下方必做任务）。
7. Reference 发起 6 个并发请求：3 个正常 `add`，1 个 `delay_add` 超时，1 个 `error_method`，1 个未知 method。

### 推荐项目骨架

在开始写代码之前，建议先建立以下文件结构：

```
mini_rpc/
  protocol.h        -- Request/Response 类型定义 + 序列化
  connection.h      -- Transport: 连接管理 + 发送/接收
  rpc_client.h      -- Client: call() 接口 + pending map + 超时/重试
  rpc_server.h      -- Server: handler 注册 + accept 循环
  handlers.h        -- add / delay_add / error_method 三个 handler
  main_test.cpp     -- 测试场景: 6 并发请求
```

每个头文件应该是独立的、可单独理解的设计单元。不要把所有东西塞到一个文件里——这不是"快写完"的策略，而是"以后看不懂"的策略。

### 必做任务

1. **画 6 张图**，再写代码：
   - **图 1：Client 调用图** -- 从 `co_await rpc.call(req, timeout, retries)` 到拿到 response 的完整对象链：Asio awaitable -> pending map -> queued writer -> `async_write` / read loop -> timer -> resume。
   - **图 2：Server handler 树** -- 一个协程 handler 可能调用其他子协程，画出调用-被调用关系，标注每个 co_await 点。
   - **图 3：Protocol 状态机** -- 一条 TCP 连接上，请求/响应的交错状态：空闲、等待请求头、读取请求体、分发 handler、发送响应头、发送响应体、空闲（复用连接）。
   - **图 4：Cancellation 传播路径** -- client timer 超时 -> pending map 移除 request -> 发送 cancel frame -> server handler 在检查点观察 cancel state -> 自然返回。
   - **图 5：Drain 拓扑** -- accept loop、connection loop、writer loop、request handler、client read loop 分别如何增加/减少 `in_flight`，shutdown 如何等待归零。
   - **图 6：性能 trace** -- 用 traced_awaitable（模块 J-3）对一次请求的完整路径打 trace：从 `co_await call()` 到 `await_resume` 返回，标注每个阶段的耗时。

2. **实现 Protocol 层**（约 50-80 行）：
   - 定义 `Request { std::string method; std::vector<int> args; }`
   - 定义 `Response { uint32_t req_id; int result; std::string status; }`（status 可以是 `"ok"` / `"error"` / `"unknown_method"`）
   - 实现 `serialize(Request) -> std::string` 和 `deserialize(std::string) -> std::expected<Response, Error>`
   - 最简单的实现：用 `std::format` 和 `std::sscanf` 或手工字符串拼接。本项目的重点不是序列化框架。

3. **实现 Transport 层**（约 80-120 行）：
   - 用 Asio TCP 建立连接
   - 连接复用：一个 connection 对象维护一个发送队列和一个未完成请求映射表（`std::unordered_map<request_id, promise*>` 或等价的 completion 存储）
   - `async_read_response(connection, request_id) -> awaitable`：从 TCP 流中读取对应请求的响应

4. **实现 Client RPC 接口**（约 80-120 行）：
   ```cpp
   asio::awaitable<std::expected<Response, RpcError>> call(Request req,
           std::chrono::milliseconds timeout = 3s);
   ```
   - 内部逻辑：分配 request_id -> 序列化 -> 发送 -> 注册 pending -> `co_await` 响应或超时
   - 超时用 `asio::steady_timer` 与响应等待协作完成
   - 超时后：取消 in-flight 请求、从 pending map 中移除、返回 `RpcError::Timeout`
   - 如果 server 返回 error status，返回 `RpcError::ServerError{message}`

5. **实现 Server handler**（约 60-100 行）：
   - 定义一个 `method_registry`：`std::unordered_map<std::string, std::function<asio::awaitable<Response>(Request)>>`
   - 每个注册的 handler 是一个返回 `asio::awaitable<Response>` 的协程
   - 至少注册 3 个 handler：
     - `add`：`co_return Response{a+b, "ok"}`（同步计算，但以协程形式返回）
     - `delay_add`：`co_await asio_timer(delay); co_return Response{a+b, "ok"}`（异步等待后计算）
     - `error_method`：抛出或返回 error status

6. **实现 Server accept 循环**（约 50-80 行）：
   ```cpp
   asio::awaitable<void> server_loop(asio::ip::tcp::acceptor& acceptor, in_flight_counter& in_flight) {
       while (true) {
           auto socket = co_await acceptor.async_accept(asio::use_awaitable);
           ++in_flight;
           asio::co_spawn(
               co_await asio::this_coro::executor,
               handle_connection(std::move(socket)),
               [&in_flight](std::exception_ptr) { --in_flight; });
       }
   }
   ```
   - 每个连接都用 completion handler 减少 `in_flight`
   - `handle_connection` 内部循环：读请求 -> 查找 handler -> 调用 handler -> 写响应

7. **编写测试场景**：
   - 启动 server 在后台线程
   - Client 同时发起 6 个并发请求（用 `asio::co_spawn(..., asio::use_future)` 收集结果）
   - 其中 1 个请求设置 100ms 超时，而 handler 需要 500ms -> 触发超时路径
   - 其中 1 个请求调用 `error_method` -> 触发 error completion 路径
   - 其中 1 个请求调用未知 method -> 触发 unknown_method 路径
   - 其余 3 个正常完成
   - shutdown 时等待 client/server `in_flight` 计数归零
   - 验证：最终拿到 6 个结果（3 个 ok、1 个 timeout、1 个 server_error、1 个 unknown_method）

8. **验证结构化收束**：在 `io_context` 停止前确保 client/server `in_flight()` 都为 0，没有协程被 detach。

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
    std::string status;  // "ok" | "error" | "unknown_method"
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
    // 异步发送请求，返回 awaitable 完成时表示数据已写出
    auto async_send(const std::string& data) -> asio::awaitable<void>;

    // 异步读取一个响应帧，返回 awaitable<string>
    auto async_read_response() -> asio::awaitable<std::string>;
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
    std::atomic<int> in_flight_{0};  // 观察后台协程是否全部 drain
public:
    // 发起一个 RPC 调用。caller 必须 co_await 返回值。
    asio::awaitable<std::expected<Response, RpcError>> call(Request req,
                   std::chrono::milliseconds timeout = 3s);

    // 关闭 client，取消所有 pending 请求
    void shutdown();
    int in_flight() const noexcept;
};
```

**Server 接口形状**：

```cpp
// rpc_server.h
class RpcServer {
    using Handler = std::function<asio::awaitable<Response>(Request)>;
    std::unordered_map<std::string, Handler> handlers_;
    asio::io_context& io_ctx_;
    std::atomic<int> in_flight_{0};
public:
    // 注册一个 RPC method handler
    void register_handler(std::string method, Handler handler);

    // 启动 accept 循环。内部 co_spawn accept_loop，并用 completion handler 维护 in_flight。
    void start();
    void stop();
    int in_flight() const noexcept;
};
```

**注意**：以上接口形状是示意性的，但 reference 统一使用 `asio::awaitable`。如果你想练 stdexec 或自写 task 桥接，把它作为进阶对比实现，不要混进 reference 契约。

### 观察点

- RPC 框架中，**request_id 是连接复用的关键**。每个连接可以同时承载多个请求-响应对，request_id 用于将响应路由回正确的等待者。
- **Client 和 Server 的协程生命周期方向相反**：client 端协程从外部发起（`co_await rpc.call()`），server 端协程由 accept/read loop `co_spawn` 创建。两者的拥有链不能混。
- **超时和 connection lost 是两个不同的失败模式**。超时后连接可能仍然有效，connection lost 后所有 pending 请求一次性失败。在 PendingMap 中要区分这两种清理。
- **取消不是销毁 handler**。client 超时后可以发送 cancel frame；server handler 在检查点观察 stop/cancel 状态并自然返回。直接 `destroy()` 正在运行或可能恢复的 handler 会绕过栈上对象析构边界，容易造成 use-after-free。
- Reference 用 `in_flight` 计数和 completion handler 管理两种后台工作：client 端 read loop / pending 请求，server 端 accept loop / connection loop / handler / writer loop。计数不是业务逻辑，只是可观察的 shutdown drain 证据。
- cancellation 从 client 到 server 的传播路径是：client timer 超时 -> pending map 移除 -> cancel frame -> server cancel_state -> handler 检查点自然返回。不是强杀协程。

### 常见坑

- 忘记在超时后从 PendingMap 中移除 request_id。如果后续该 request_id 的响应到达，会尝试完成一个已经不存在的等待者。
- Server handler 中 `co_await` 了长时间操作但没有检查 stop_token，导致 client 已超时、server 还在傻等。
- `co_spawn` 后没有 completion handler 或 future 所有者，导致后台协程结束不可观察，shutdown 时无法证明已经 drain。
- 连接断开后 PendingMap 中残留的 request 没有被清理，造成资源泄漏、等待者永不完成，`in_flight()` 也无法归零。
- 把 connection、pending_map、后台协程 completion ownership 混在一个大结构体中互相依赖，导致析构顺序难以推理。应明确 connection 拥有 pending map，`co_spawn` 的 future/completion handler 拥有后台协程完成路径，shutdown 最后等待 `in_flight()` drain。
- call 完成后如果 timer 仍在运行，需要取消 timer；timeout 后如果响应稍后到达，需要从 pending map 中安全忽略。

### 提示

- 先用单连接开发，跑通一次请求-响应全路径，再加入并发和多连接复用。
- request_id 用自增整数即可，不需要 UUID。4 字节足够 4 billion 请求——远超单次测试。
- 序列化格式选择：reference 使用简单 `|` 分隔体加 8 字节长度头，便于观察帧边界；不要把序列化框架变成本题重点。
- 超时实现：reference 使用 `asio::steady_timer`。如果你另写 stdexec 对比版，再单独验证 `when_any` / `with_timeout` 对 loser 的取消行为。
- 测试场景写成一个独立的 `main_test.cpp`，不要嵌在 library 代码里。每次验证都应该从头构造 client 和 server。

### 进阶任务

- 加入重试策略：超时后自动重试最多 2 次，每次重试单独受超时控制。用 `let_error` 或协程内部的 while 循环实现。
- 实现请求级 cancellation：当 client 超时后，通过 TCP 发送一个 cancel 帧给 server，server handler 在定时器、I/O、循环边界等检查点观察取消状态并协作返回。不要主动 `destroy()` 运行中 handler。
- 加入统计收集：用 traced_awaitable 收集每次请求的 latencies，输出 p50 / p99 / max。
- 做一个对比实现：纯 sender-receiver 版（不用协程 task，用 sender 图拼接所有异步操作），对比代码行数和可读性。
- 把 transport 层从 TCP 替换为 Unix domain socket 或 in-memory channel，观察接口抽象层的稳定性。
- Server 端实现一个轻量的"method 不存在"统一兜底——不注册任何 handler 的 method 被调用时不崩溃、不留 pending，而是返回带有描述性 `status: "unknown_method"` 的 Response。这样做的好处是：client 不需要为每个新 method 更新错误处理，错误路径统一。

### 实现过程中你可能会卡住的地方

以下三个点是之前练习者在做这个项目时最常见的阻塞点。如果你卡住了，先检查是不是这些问题：

1. **Asio 后台协程的所有权**：`asio::co_spawn` 不等于自动结构化并发。reference 对每条后台路径都保留 future 或 completion handler，并用 `in_flight` 验证 shutdown drain。

2. **PendingMap 的线程安全**：如果 server handler 在 io_context 的单线程上执行，而 client 端的超时 timer 也在同一个 io_context 上，没有线程安全问题。但如果你的超时 timer 在另一个线程触发，PendingMap 必须加 mutex。一个简单策略：整个 RPC 系统都运行在同一个 io_context 上，避免多线程复杂度。

3. **连接断开后的清理**：TCP 连接意外断开时，PendingMap 中所有该连接的 pending request 都需要被通知（以 `RpcError::ConnectionLost` 完成）。如果你忘了这一条，等待 response 的协程可能永远挂起。

### 验收点

- 6 个并发请求全部收到结果，包含 3 个正常、1 个超时、1 个 server_error、1 个 unknown_method，结果正确。
- 你能指出每张图中最关键的 3 个节点。
- 你没有使用 `asio::detached`、裸 `detach()` 或全局可变状态。
- `shutdown()/stop()` 后 client/server 的 `in_flight()` 都能归零。
- 你能解释：如果你把 `rpc.call()` 的返回值从 Asio awaitable 改为 sender，client 代码需要改哪些地方、哪些不变。
- 你能解释：cancellation 从 client timer 到 server handler 的传播路径中，每个环节分别依赖什么机制（timer / pending map / cancel frame / 自定义检查点）。

### 项目复盘问题

- 在这个项目中，Asio awaitable 承担了什么角色？如果改成 sender-receiver，哪些部分会更清晰，哪些部分会更重？
- 如果没有 completion handler / future / in_flight drain，后台协程的生命周期管理会有多复杂？写出一个反例版本的大致结构。
- 超时和重试的组合逻辑中，哪一部分最容易出现资源泄漏（allocated request_id 永不被回收、handler 协程永不被销毁）？
- protocol 状态机中，哪个状态转换最容易因为并发请求的交错而出 bug？
- 如果要把这套 RPC 框架扩展为支持 streaming（server 推送多条响应），哪些接口和对象关系需要改动？
- 完成这个项目后，你对"协程是表达层工具，不是调度/生命周期的替代品"这句话的理解深了多少？

### 完成后你能说什么

至少把下面几句话说顺：

- 我用纯 Asio `awaitable` 实现了一个完整的最小 RPC 框架，包含 client 端的 `co_await rpc.call(req, timeout, retries)` 获取 response、server 端的协程 handler、以及超时/取消帧/重试。
- Client 端 6 个并发请求中，3 个正常完成、1 个超时、1 个 server_error、1 个 unknown_method，并在 shutdown 后验证 client/server `in_flight()` 都归零。
- 我画了 6 张图：client 调用图、server handler 树、protocol 状态机、cancellation 传播路径、drain 拓扑、性能 trace。每张图都标注了核心对象的拥有关系和生命周期边界。
- 整套系统没有 `asio::detached`、没有全局可变状态、没有裸 `detach()`。所有后台协程都有 completion handler 或 future 所有者。
- 我能解释 cancellation 从 client timer 到 server handler 检查点的完整传播路径，以及每个环节依赖的机制。
- 这个项目让我真正理解了：协程让异步代码读起来像同步，但底层的调度、生命周期、错误传播、取消机制一个都没少——它们只是被明确组织在 Asio executor、对象拥有关系和 completion/drain 协议中，而不是被隐藏。
