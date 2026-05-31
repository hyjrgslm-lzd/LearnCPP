// =============================================================================
// src/server.cpp —— RPC server（accept 循环 + handler 注册 + 协程 dispatch）
//
// 对应文档：13-第三阶段结课-RPC框架.md
//   §"必做任务 5 / 6 / 7"
//
// 设计要点：
//   - acceptor 在 server scope 中 spawn handle_connection；
//   - handle_connection 内部循环：读请求 -> 查表 -> 调 handler -> 写响应；
//   - method 不存在时统一返回 Response{status:"unknown_method"}，不崩溃；
//   - server handler 是协程，可继续 co_await（如 delay_add）；
//   - 不允许 detach；不允许全局可变状态。
//
// 关键类型形状（13-RPC §"Server 接口形状"）：
//   class RpcServer {
//       using Handler = std::function<task<Response>(Request)>;
//       std::unordered_map<std::string, Handler> handlers_;
//       async_scope scope_;
//       task<void> serve(uint16_t port);
//   };
// =============================================================================

#include "rpc/protocol.hpp"
#include "rpc/scope.hpp"
#include "rpc/stop_token.hpp"
#include "rpc/task.hpp"

#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>

// stdexec / asio 头文件按需引入（CMakeLists 链接 stage3 deps）
// #include <stdexec/execution.hpp>
// #include <asio.hpp>

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
    // TODO[必做]: 用 asio::steady_timer + as_awaitable 等待 args[0] 毫秒
    //            参考 H-1 桥接 / 模块 I 的 asio_awaitable_t；
    //            真实实现示例：
    //              co_await timer.async_wait(asio::use_awaitable);
    //            为了 stage3 通用性，这里给出占位逻辑。
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
    async_scope                              scope_;

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
        //   while (!stop_token.stop_requested()) {
        //       auto sock = co_await acceptor.async_accept(asio::use_awaitable);
        //       scope_.spawn(handle_connection(std::move(sock)));
        //   }
        // 提示：use_awaitable 是 Asio 的标准 completion token；
        //       如果你用自写 task<T>，需在 H-1 完成 as_awaitable 适配；
        //       本骨架默认两条路线都写好了。
        co_return;
    }

    // 单连接 handler：循环读请求 -> 调度 -> 写响应
    task<void> handle_connection(/* asio::ip::tcp::socket sock */) {
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
        //       co_await async_write(sock, asio::buffer(wire), asio::use_awaitable);
        //   }
        co_return;
    }

    auto& scope() noexcept { return scope_; }
};

} // namespace rpc

// 真正的 main 在 src/main.cpp 中；本文件只构建 server 侧实现。
// 不在此处定义 main()，否则与 main.cpp 重符号。
