#include "rpc/client.hpp"
#include "rpc/server.hpp"

#include <array>
#include <cstdlib>
#include <exception>
#include <future>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <utility>

using result_t = std::expected<rpc::Response, rpc::RpcError>;
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

static void require(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "check failed: " << message << '\n';
        std::exit(1);
    }
}

static void protocol_checks() {
    auto encoded = rpc::serialize(rpc::Request{7, "add", {1, 2}, true});
    require(encoded.starts_with("00000013"), "frame header uses 8-byte decimal length");
    auto req = rpc::parse_request(encoded);
    require(req && req->req_id == 7 && req->idempotent && req->args.size() == 2,
            "request round trip keeps id/idempotent/args");
    require(!rpc::parse_request("Q|1|1|add|x"), "bad request args are protocol errors");
    require(!rpc::parse_response("R|bad|0|ok"), "bad response id is a protocol error");
    require(!rpc::frame(std::string(rpc::max_frame_size + 1, 'x')), "oversized body is rejected");

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

    auto read = asio::co_spawn(io, rpc::read_frame(server_socket), asio::use_future);
    asio::co_spawn(io, [&]() -> asio::awaitable<void> {
        co_await asio::async_write(client_socket, asio::buffer(std::string{"bad!!!!!"}), asio::use_awaitable);
    }, asio::use_future).get();
    auto result = read.get();
    require(!result && result.error() == rpc::RpcError::SerializationError,
            "bad wire header returns SerializationError");

    post_and_wait(io, [&] {
        asio::error_code ignored;
        client_socket.close(ignored);
        server_socket->close(ignored);
        acceptor.close(ignored);
    });
    guard.reset();
    runner.join();
}

static void server_checks() {
    asio::io_context io;
    auto guard = asio::make_work_guard(io);
    rpc::RpcServer server{io};
    server.start();

    rpc::RpcClient client{io};
    auto connect = asio::co_spawn(io, client.connect("127.0.0.1", server.port()), asio::use_future);
    std::jthread runner{[&] { io.run(); }};
    connect.get();

    std::array<std::future<result_t>, 6> results{
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
        if (!result && result.error() == rpc::RpcError::Timeout) ++timeout;
        if (!result && result.error() == rpc::RpcError::ServerError) ++server_error;
        if (!result && result.error() == rpc::RpcError::UnknownMethod) ++unknown;
    }

    post_and_wait(io, [&] {
        client.shutdown();
        client.shutdown();
        server.stop();
    });
    guard.reset();
    runner.join();
    require(ok == 3 && timeout == 1 && server_error == 1 && unknown == 1,
            "server maps normal timeout error and unknown outcomes");
    require(client.in_flight() == 0 && server.in_flight() == 0, "client and server drain after shutdown");
}

static result_t call_against_raw_server(std::string response_body, bool close_without_response = false) {
    asio::io_context io;
    auto guard = asio::make_work_guard(io);
    asio::ip::tcp::acceptor acceptor{io, {asio::ip::tcp::v4(), 0}};
    acceptor.listen();

    rpc::RpcClient client{io};
    auto server_done = asio::co_spawn(io, [&]() -> asio::awaitable<void> {
        auto socket = co_await acceptor.async_accept(asio::use_awaitable);
        auto shared_socket = std::make_shared<asio::ip::tcp::socket>(std::move(socket));
        auto request = co_await rpc::read_frame(shared_socket);
        require(request.has_value(), "raw server reads request frame");
        if (close_without_response) co_return;
        auto wire = rpc::frame(std::move(response_body));
        require(wire.has_value(), "raw server frames response");
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
    require(client.in_flight() == 0, "client drains after raw server call");
    return result;
}

static void pending_error_checks() {
    auto lost = call_against_raw_server({}, true);
    require(!lost && lost.error() == rpc::RpcError::ConnectionLost, "connection lost wakes pending call");

    auto protocol = call_against_raw_server("not-a-response");
    require(!protocol && protocol.error() == rpc::RpcError::SerializationError, "bad response is protocol error");
}

static void retry_check() {
    asio::io_context io;
    auto guard = asio::make_work_guard(io);
    asio::ip::tcp::acceptor acceptor{io, {asio::ip::tcp::v4(), 0}};
    acceptor.listen();

    int requests = 0;
    int cancels = 0;
    rpc::RpcClient client{io};
    auto server_done = asio::co_spawn(io, [&]() -> asio::awaitable<void> {
        auto socket = co_await acceptor.async_accept(asio::use_awaitable);
        auto shared_socket = std::make_shared<asio::ip::tcp::socket>(std::move(socket));
        while (requests < 2) {
            auto body = co_await rpc::read_frame(shared_socket);
            require(body.has_value(), "retry server reads request or cancel frame");
            if (body->starts_with("C|")) {
                ++cancels;
                continue;
            }
            auto req = rpc::parse_request(*body);
            require(req.has_value(), "retry request parses");
            ++requests;
            if (requests == 2) {
                auto wire = rpc::serialize(rpc::Response{req->req_id, 3, "ok"});
                co_await asio::async_write(*shared_socket, asio::buffer(wire), asio::use_awaitable);
            }
        }
    }, asio::use_future);
    auto connect = asio::co_spawn(io, client.connect("127.0.0.1", acceptor.local_endpoint().port()), asio::use_future);
    std::jthread runner{[&] { io.run(); }};
    connect.get();

    auto result = asio::co_spawn(io, client.call({0, "add", {1, 2}, true}, 30ms, 1), asio::use_future).get();
    server_done.get();
    post_and_wait(io, [&] {
        client.shutdown();
        asio::error_code ignored;
        acceptor.close(ignored);
    });
    guard.reset();
    runner.join();
    require(result && result->result == 3, "idempotent retry returns second response");
    require(requests == 2 && cancels >= 1, "idempotent retry sends cancel then retries");
    require(client.in_flight() == 0, "client drains after retry");
}

static void encoding_failure_check() {
    asio::io_context io;
    auto guard = asio::make_work_guard(io);
    rpc::RpcClient client{io};
    std::jthread runner{[&] { io.run(); }};

    auto result = asio::co_spawn(io,
        client.call({0, std::string(rpc::max_frame_size, 'x'), {}, false}, 1s),
        asio::use_future).get();
    require(!result && result.error() == rpc::RpcError::SerializationError, "encode failure maps to SerializationError");

    post_and_wait(io, [&] { client.shutdown(); });
    guard.reset();
    runner.join();
    require(client.in_flight() == 0, "client drains after encode failure");
}

int main() {
    try {
        protocol_checks();
        server_checks();
        pending_error_checks();
        retry_check();
        encoding_failure_check();
    } catch (...) {
        std::cerr << "check failed: uncaught exception from implementation\n";
        return 1;
    }
}
