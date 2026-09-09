// =============================================================================
// src/client.cpp —— RPC client（call() / pending map / deadline / drain）
//
// 对应文档：13-第三阶段结课-RPC框架.md  §"必做任务 3 / 4 / 7"
//
// Starter 设计要点：
//   - 单连接复用，多请求共享同一连接；
//   - request_id 自增，pending map 路由响应；
//   - call() 接口：asio::awaitable<std::expected<Response, RpcError>>；
//   - 超时用 asio::steady_timer + pending cancel；
//   - co_spawn 必须使用 completion handler 或 asio::use_future 拥有结果；
//   - 连接断开：pending map 中所有等待者一次性 ConnectionLost 完成。
//
// 关键类型形状（13-RPC §"Client 接口形状"）：
//   class RpcClient {
//       Connection conn_;
//       PendingMap pending_;
//       std::atomic<uint32_t> next_id_{0};
//       in_flight_scope scope_;
//       task<std::expected<Response, RpcError>> call(Request, ms timeout);
//   };
// =============================================================================

#include "rpc/protocol.hpp"
#include "rpc/scope.hpp"
#include "rpc/task.hpp"

#include <chrono>
#include <expected>
#include <mutex>
#include <unordered_map>

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
    // void register_request(uint32_t id, waiter);
    // void complete_request(uint32_t id, Response);
    // void cancel_request  (uint32_t id, RpcError::Timeout);
    // void fail_all        (RpcError::ConnectionLost);

    std::size_t size() {
        std::lock_guard<std::mutex> lk(mtx_);
        return map_.size();
    }
};

// ============ Connection：transport 层 ============
class Connection {
    // TODO[必做]: 维护一个 asio::ip::tcp::socket 与 queued_writer；
    //              提供 async_send(string) -> asio::awaitable<void>；
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
    in_flight_scope             scope_;

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

        // 5. 等待响应或超时。建议结构：
        //   - deadline timer 到点后 cancel_request(id, Timeout)；
        //   - read_loop 收到响应后 complete_request(id, Response)；
        //   - 两边都通过同一个 waiter 只完成一次。
        // TODO[必做]: 使用 asio::steady_timer 管理 deadline。
        //
        // 6. 后台 read_loop 启动方式示例：
        //   scope_.on_spawn();
        //   asio::co_spawn(ex, conn_.read_loop(pending_),
        //       [this](std::exception_ptr) { scope_.on_complete(); });
        //   或用 asio::use_future 保存 future，再在 shutdown 中 get()。
        //   禁止 asio::detached，因为练习要观察 in_flight 归零。
        // TODO[必做]:

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
