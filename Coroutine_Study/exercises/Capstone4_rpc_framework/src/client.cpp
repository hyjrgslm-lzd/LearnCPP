// =============================================================================
// src/client.cpp —— RPC client（call() / pending map / 超时 / 取消）
//
// 对应文档：13-第三阶段结课-RPC框架.md  §"必做任务 3 / 4 / 7"
//
// 设计要点：
//   - 单连接复用，多请求共享同一连接；
//   - request_id 自增，pending map 路由响应；
//   - call() 接口：task<std::expected<Response, RpcError>>；
//   - 超时用 when_any(call_sender, timer_sender)；超时后取消 in-flight；
//   - 不允许 detach，所有请求由 scope 拥有；
//   - 连接断开：pending map 中所有等待者一次性 ConnectionLost 完成。
//
// 关键类型形状（13-RPC §"Client 接口形状"）：
//   class RpcClient {
//       Connection conn_;
//       PendingMap pending_;
//       std::atomic<uint32_t> next_id_{0};
//       async_scope scope_;
//       task<std::expected<Response, RpcError>> call(Request, ms timeout);
//   };
// =============================================================================

#include "rpc/protocol.hpp"
#include "rpc/scope.hpp"
#include "rpc/stop_token.hpp"
#include "rpc/task.hpp"

#include <atomic>
#include <chrono>
#include <expected>
#include <iostream>
#include <mutex>
#include <unordered_map>

// 真实实现需要：
// #include <stdexec/execution.hpp>
// #include <asio.hpp>

namespace rpc {

// ============ PendingMap：request_id -> 等待者 ============
//
// 等待者用一个 std::variant<callback> 表达：
//   - set_value(Response) -> 唤醒 client.call() 协程；
//   - set_error(RpcError) -> ConnectionLost / Timeout；
//   - set_stopped() -> stop_token request_stop。
class PendingMap {
    std::mutex                                            mtx_;
    std::unordered_map<std::uint32_t,
        /* TODO: 等待者类型，可选 callback / promise<Response> */ int> map_;

public:
    // TODO[必做]: 实现：
    // void register_request(uint32_t id, /* waiter */);
    // void complete_request(uint32_t id, Response resp);
    // void cancel_request  (uint32_t id, RpcError err);
    // void fail_all        (RpcError err); // 连接断开时调用

    std::size_t size() {
        std::lock_guard<std::mutex> lk(mtx_);
        return map_.size();
    }
};

// ============ Connection：transport 层 ============
class Connection {
    // TODO[必做]: 维护一个 asio::ip::tcp::socket 与发送队列；
    //              提供 async_send(string) -> sender；
    //              内部启动读循环：
    //                while (true) co_await read_one_response();
    //                每个响应通过 PendingMap.complete_request 路由给等待者。
public:
    task<void> read_loop(PendingMap& /*pending*/) {
        // 读到一帧 -> parse_response -> pending.complete_request(id, resp)
        // 若 read 抛 EOF -> pending.fail_all(ConnectionLost) -> co_return
        co_return;
    }
};

// ============ RpcClient ============
class RpcClient {
    Connection                  conn_;
    PendingMap                  pending_;
    std::atomic<std::uint32_t>  next_id_{0};
    async_scope                 scope_;

public:
    // 发起一个 RPC 调用，超时默认 3s
    task<std::expected<Response, RpcError>> call(
            Request req,
            std::chrono::milliseconds timeout = std::chrono::milliseconds{3000}) {
        // 1. 分配 request_id
        req.req_id = ++next_id_;

        // 2. 序列化
        auto wire = serialize(req);

        // 3. 注册到 pending map
        // TODO[必做]: pending_.register_request(req.req_id, /* waiter */);

        // 4. 异步发送
        // TODO[必做]: co_await conn_.async_send(wire);

        // 5. 等待响应或超时（when_any 二选一）
        // TODO[必做]:
        //   auto winner = co_await stdexec::when_any(
        //       wait_response_sender(req.req_id),
        //       timer_sender(timeout)
        //   );
        //   if (winner == 0) co_return resp;
        //   if (winner == 1) {
        //       pending_.cancel_request(req.req_id, RpcError::Timeout);
        //       co_return std::unexpected(RpcError::Timeout);
        //   }

        (void)wire;
        (void)timeout;
        co_return std::unexpected(RpcError::ServerError);
    }

    auto& scope() noexcept { return scope_; }

    // shutdown：取消所有 pending、等待 scope 清空
    task<void> shutdown() {
        // TODO[必做]: pending_.fail_all(ConnectionLost);
        //            scope_.wait_empty();
        co_return;
    }
};

} // namespace rpc
