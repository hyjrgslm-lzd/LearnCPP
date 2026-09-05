#include "rpc_ref/rpc.hpp"

#include <array>
#include <cstdlib>
#include <exception>
#include <future>
#include <memory>
#include <string>
#include <thread>
#include <utility>

using result_t = std::expected<rpc_ref::response, rpc_ref::error>;
using future_t = std::future<result_t>;
using namespace std::chrono_literals;

template <class Fn>
static void post_and_wait(asio::io_context& io, Fn&& fn) {
    auto done = std::make_shared<std::promise<void>>();
    auto future = done->get_future();
    asio::post(io, [done, fn = std::forward<Fn>(fn)]() mutable {
        try {
            fn();
            done->set_value();
        } catch (...) {
            done->set_exception(std::current_exception());
        }
    });
    future.get();
}

static void protocol_checks() {
    if (rpc_ref::parse_request("Q|1|1|add|x")) std::abort();
    if (rpc_ref::parse_response("R|bad|0|ok")) std::abort();
    if (rpc_ref::frame(std::string(rpc_ref::max_frame_size + 1, 'x'))) std::abort();

    asio::io_context io;
    auto guard = asio::make_work_guard(io);
    asio::ip::tcp::acceptor acceptor{io, {asio::ip::tcp::v4(), 0}};
    acceptor.listen();
    auto server_socket = std::make_shared<asio::ip::tcp::socket>(io);
    asio::ip::tcp::socket client_socket{io};
    auto accept = asio::co_spawn(io, [&]() -> asio::awaitable<void> {
        *server_socket = co_await acceptor.async_accept(asio::use_awaitable);
    }, asio::use_future);
    auto connect = asio::co_spawn(io, [&]() -> asio::awaitable<void> {
        co_await client_socket.async_connect(
            {asio::ip::make_address("127.0.0.1"), acceptor.local_endpoint().port()}, asio::use_awaitable);
    }, asio::use_future);
    std::jthread runner{[&] { io.run(); }};
    connect.get();
    accept.get();

    auto read = asio::co_spawn(io, rpc_ref::read_frame(server_socket), asio::use_future);
    const std::string bad_header = "bad!!!!!";
    asio::co_spawn(io, [&]() -> asio::awaitable<void> {
        co_await asio::async_write(client_socket, asio::buffer(bad_header), asio::use_awaitable);
    }, asio::use_future).get();
    auto result = read.get();
    if (result || result.error() != rpc_ref::error::protocol) std::abort();

    post_and_wait(io, [&] {
        asio::error_code ignored;
        client_socket.close(ignored);
        server_socket->close(ignored);
        acceptor.close(ignored);
    });
    guard.reset();
    runner.join();
}

static void reference_server_checks() {
    asio::io_context io;
    auto guard = asio::make_work_guard(io);
    rpc_ref::server server{io};
    server.start();

    rpc_ref::client client{io};
    auto connect = asio::co_spawn(io, client.connect("127.0.0.1", server.port()), asio::use_future);
    std::jthread runner{[&] { io.run(); }};
    connect.get();

    using result_t = std::expected<rpc_ref::response, rpc_ref::error>;
    using future_t = std::future<result_t>;
    std::array<future_t, 6> results{
        asio::co_spawn(io, client.call({0, "add", {1, 2}, true}, 1s), asio::use_future),
        asio::co_spawn(io, client.call({0, "add", {10, 20}, true}, 1s), asio::use_future),
        asio::co_spawn(io, client.call({0, "add", {100, 200}, true}, 1s), asio::use_future),
        asio::co_spawn(io, client.call({0, "delay_add", {500}, true}, 100ms, 1), asio::use_future),
        asio::co_spawn(io, client.call({0, "error_method", {}, false}, 1s), asio::use_future),
        asio::co_spawn(io, client.call({0, "missing", {}, false}, 1s), asio::use_future),
    };

    int ok = 0;
    int timeout = 0;
    int server_error = 0;
    int unknown = 0;
    for (auto& future : results) {
        auto result = future.get();
        if (result && result->status == "ok") ++ok;
        if (!result && result.error() == rpc_ref::error::timeout) ++timeout;
        if (!result && result.error() == rpc_ref::error::server_error) ++server_error;
        if (!result && result.error() == rpc_ref::error::unknown_method) ++unknown;
    }

    post_and_wait(io, [&] {
        client.shutdown();
        server.stop();
    });
    guard.reset();
    runner.join();
    if (ok != 3 || timeout != 1 || server_error != 1 || unknown != 1) std::abort();
    if (client.in_flight() != 0 || server.in_flight() != 0) std::abort();
}

static result_t call_against_raw_server(std::string response_body, bool close_without_response = false) {
    asio::io_context io;
    auto guard = asio::make_work_guard(io);
    asio::ip::tcp::acceptor acceptor{io, {asio::ip::tcp::v4(), 0}};
    acceptor.listen();

    rpc_ref::client client{io};
    auto server_done = asio::co_spawn(io, [&]() -> asio::awaitable<void> {
        auto socket = co_await acceptor.async_accept(asio::use_awaitable);
        auto shared_socket = std::make_shared<asio::ip::tcp::socket>(std::move(socket));
        auto request = co_await rpc_ref::read_frame(shared_socket);
        if (!request) std::abort();
        if (close_without_response) {
            co_return;
        }
        auto wire = rpc_ref::frame(std::move(response_body));
        if (!wire) std::abort();
        co_await asio::async_write(*shared_socket, asio::buffer(*wire), asio::use_awaitable);
    }, asio::use_future);
    auto connect = asio::co_spawn(io, client.connect("127.0.0.1", acceptor.local_endpoint().port()), asio::use_future);
    std::jthread runner{[&] { io.run(); }};
    connect.get();

    auto result = asio::co_spawn(io, client.call({0, "add", {1, 2}, false}, 5s), asio::use_future).get();
    server_done.get();
    post_and_wait(io, [&] {
        client.shutdown();
        asio::error_code ignored;
        acceptor.close(ignored);
    });
    guard.reset();
    runner.join();
    if (client.in_flight() != 0) std::abort();
    return result;
}

static void pending_error_checks() {
    auto lost = call_against_raw_server({}, true);
    if (lost || lost.error() != rpc_ref::error::connection_lost) std::abort();

    auto protocol = call_against_raw_server("not-a-response");
    if (protocol || protocol.error() != rpc_ref::error::protocol) std::abort();
}

static void encoding_failure_check() {
    asio::io_context io;
    auto guard = asio::make_work_guard(io);
    rpc_ref::client client{io};
    std::jthread runner{[&] { io.run(); }};

    auto result = asio::co_spawn(io,
        client.call({0, std::string(rpc_ref::max_frame_size, 'x'), {}, false}, 1s),
        asio::use_future).get();
    if (result || result.error() != rpc_ref::error::protocol) std::abort();

    post_and_wait(io, [&] { client.shutdown(); });
    guard.reset();
    runner.join();
    if (client.in_flight() != 0) std::abort();
}

int main() {
    protocol_checks();
    reference_server_checks();
    pending_error_checks();
    encoding_failure_check();
}
