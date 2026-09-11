#pragma once

#include "rpc/client.hpp"
#include "rpc/protocol.hpp"
#include "rpc/task.hpp"

#include <asio.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace rpc {

using Handler = std::function<task<Response>(Request)>;

task<Response> handler_add(Request req);
task<Response> handler_delay_add(Request req);
task<Response> handler_error_method(Request req);

class RpcServer {
public:
    explicit RpcServer(asio::io_context& io);

    std::uint16_t port() const;
    void register_handler(std::string method, Handler h);
    void start();
    void stop();
    int in_flight() const noexcept;

private:
    struct cancel_state {
        bool cancelled{};
    };

    asio::awaitable<void> accept_loop();
    asio::awaitable<void> handle_connection(std::shared_ptr<tcp::socket> socket);
    asio::awaitable<void> handle_request(std::shared_ptr<queued_writer> writer,
                                         Request req,
                                         std::shared_ptr<cancel_state> cancel);
    asio::awaitable<Response> dispatch(const Request& req, std::shared_ptr<cancel_state> cancel);

    asio::io_context& io_;
    tcp::acceptor acceptor_;
    std::unordered_map<std::string, Handler> handlers_;
    std::atomic<int> in_flight_{0};
};

} // namespace rpc
