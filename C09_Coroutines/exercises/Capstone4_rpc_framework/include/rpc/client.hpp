#pragma once

#include "rpc/protocol.hpp"

#include <asio.hpp>

#include <atomic>
#include <chrono>
#include <deque>
#include <expected>
#include <memory>
#include <optional>
#include <unordered_map>

namespace rpc {

class queued_writer : public std::enable_shared_from_this<queued_writer> {
public:
    queued_writer(std::shared_ptr<tcp::socket> socket, std::atomic<int>& in_flight);
    void send(std::string data);

private:
    asio::awaitable<void> write_loop();

    std::shared_ptr<tcp::socket> socket_;
    std::atomic<int>& in_flight_;
    std::deque<std::string> queue_;
    bool writing_{};
};

class RpcClient {
public:
    explicit RpcClient(asio::io_context& io);

    asio::awaitable<void> connect(std::string host, std::uint16_t port);
    asio::awaitable<std::expected<Response, RpcError>> call(
        Request req,
        std::chrono::milliseconds timeout = std::chrono::milliseconds{3000},
        int retries = 0);
    void shutdown();
    int in_flight() const noexcept;

private:
    struct pending_state {
        explicit pending_state(asio::io_context& io) : timer(io) {}
        asio::steady_timer timer;
        std::optional<std::expected<Response, RpcError>> result;
    };

    asio::awaitable<void> read_loop();
    void fail_all(RpcError value);

    asio::io_context& io_;
    std::shared_ptr<tcp::socket> socket_;
    std::atomic<int> in_flight_{0};
    std::shared_ptr<queued_writer> writer_;
    std::uint32_t next_id_{0};
    std::unordered_map<std::uint32_t, std::shared_ptr<pending_state>> pending_;
};

} // namespace rpc
