// =============================================================================
// src/server.cpp —— RPC server（accept loop + handler dispatch + drain）
//
// 对应文档：13-第三阶段结课-RPC框架.md
//   §"必做任务 5 / 6 / 7"
//
// Starter 设计要点：
//   - acceptor 用 asio::awaitable 写 accept_loop；
//   - 每个后台协程由 co_spawn completion handler 维护 in_flight；
//   - handle_connection 内部循环：读请求 -> 查表 -> 调 handler -> 写响应；
//   - method 不存在时统一返回 Response{status:"unknown_method"}，不崩溃；
//   - server handler 是协程，可继续 co_await（如 delay_add）；
//   - 不使用 asio::detached，不使用全局可变状态。
//
// 关键类型形状（13-RPC §"Server 接口形状"）：
//   class RpcServer {
//       using Handler = std::function<task<Response>(Request)>;
//       std::unordered_map<std::string, Handler> handlers_;
//       in_flight_scope scope_;
//       task<void> serve(uint16_t port);
//   };
// =============================================================================

#include "rpc/protocol.hpp"
#include "rpc/scope.hpp"
#include "rpc/task.hpp"

#include <chrono>
#include <functional>
#include <string>
#include <unordered_map>

namespace rpc {

// ============ Handler 类型别名 ============
using Handler = std::function<task<Response>(Request)>;

// ============ 三个示例 handler ============

// add: 同步计算，但仍以协程返回 —— 验证 lazy task 的最小 round trip
inline task<Response> handler_add(Request req) {
    int sum = 0;
    for (int x : req.args) sum += x;
    co_return Response{req.req_id, sum, "ok"};
}

// delay_add: 异步等待后返回 —— 验证 co_await asio_timer
inline task<Response> handler_delay_add(Request req) {
    // TODO[必做]: 用 asio::steady_timer + asio::use_awaitable 等待 args[0] 毫秒
    //            真实实现示例：
    //              auto ex = co_await asio::this_coro::executor;
    //              asio::steady_timer timer{ex, std::chrono::milliseconds{delay_ms}};
    //              co_await timer.async_wait(asio::use_awaitable);
    //            这里先保留占位逻辑，让 Starter smoke 可编译。
    int delay_ms = req.args.empty() ? 0 : req.args[0];
    (void)delay_ms;
    int sum = 0;
    for (int x : req.args) sum += x;
    co_return Response{req.req_id, sum, "ok"};
}

// error_method: 故意触发 error completion
inline task<Response> handler_error_method(Request req) {
    // TODO[必做]: 抛出异常或返回 status="error"；
    //            client 端通过 deserialize 后看到 status != "ok" 即视为错误
    co_return Response{req.req_id, 0, "error"};
}

// ============ RpcServer ============
class RpcServer {
    std::unordered_map<std::string, Handler> handlers_;
    in_flight_scope                         scope_;

public:
    RpcServer() {
        // TODO[必做]: 在构造器或 init() 中注册 add / delay_add / error_method
        register_handler("add",          &handler_add);
        register_handler("delay_add",    &handler_delay_add);
        register_handler("error_method", &handler_error_method);
    }

    void register_handler(std::string method, Handler h) {
        handlers_.emplace(std::move(method), std::move(h));
    }

    // accept 循环：每个连接 spawn 到 scope，由 scope 收束
    task<void> serve(std::uint16_t port) {
        (void)port;
        // TODO[必做]: 用 asio::ip::tcp::acceptor 接收连接：
        //   for (;;) {
        //       auto sock = co_await acceptor.async_accept(asio::use_awaitable);
        //       auto shared = std::make_shared<tcp::socket>(std::move(sock));
        //       scope_.on_spawn();
        //       asio::co_spawn(ex, handle_connection(shared),
        //           [this](std::exception_ptr) { scope_.on_complete(); });
        //   }
        // completion handler 是所有权边界；不要用 asio::detached。
        co_return;
    }

    // 单连接 handler：循环读请求 -> 调度 -> 写响应
    task<void> handle_connection(/* std::shared_ptr<asio::ip::tcp::socket> sock */) {
        // TODO[必做]: 完整实现：
        //   while (true) {
        //       auto req_str = co_await async_read_until(sock, '\n', asio::use_awaitable);
        //       auto req     = parse_request(req_str);
        //       if (!req) break;  // connection lost / parse failed
        //       auto it = handlers_.find(req->method);
        //       Response resp;
        //       if (it == handlers_.end()) {
        //           resp = {req->req_id, 0, "unknown_method"};
        //       } else {
        //           try { resp = co_await it->second(*req); }
        //           catch (...) { resp = {req->req_id, 0, "error"}; }
        //       }
        //       auto wire = serialize(resp);
        //       co_await asio::async_write(*sock, asio::buffer(wire), asio::use_awaitable);
        //   }
        co_return;
    }

    auto& scope() noexcept { return scope_; }
};

} // namespace rpc

// 真正的 main 在 src/main.cpp 中；本文件只构建 server 侧实现。
// 不在此处定义 main()，否则与 main.cpp 重符号。
