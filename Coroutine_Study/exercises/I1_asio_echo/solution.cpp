#include <asio.hpp>
#include <coroutine_study/exercise_check.hpp>

#include <array>
#include <future>
#include <iostream>
#include <string>
#include <string_view>
#include <stdexcept>
#include <vector>

using asio::awaitable;
using asio::co_spawn;
using asio::use_awaitable;
using asio::ip::tcp;

awaitable<void> echo_session(tcp::socket socket)
{
    std::array<char, 1024> buf{};
    for (;;) {
        auto [read_ec, n] = co_await socket.async_read_some(
            asio::buffer(buf), asio::as_tuple(use_awaitable));
        if (read_ec) {
            co_return;
        }

        auto [write_ec, written] = co_await asio::async_write(
            socket, asio::buffer(buf, n), asio::as_tuple(use_awaitable));
        if (write_ec) {
            co_return;
        }
        coroutine_study::check(written == n, "short echo write");
    }
}

awaitable<void> listener(
    tcp::acceptor& acceptor,
    asio::cancellation_signal& stop,
    std::vector<std::future<void>>& sessions)
{
    for (;;) {
        auto [ec, socket] = co_await acceptor.async_accept(
            asio::bind_cancellation_slot(stop.slot(), asio::as_tuple(use_awaitable)));
        if (ec == asio::error::operation_aborted) {
            co_return;
        }
        if (ec) {
            throw std::runtime_error("accept failed: " + ec.message());
        }
        sessions.push_back(co_spawn(
            co_await asio::this_coro::executor,
            echo_session(std::move(socket)),
            asio::use_future));
    }
}

awaitable<void> client(unsigned short port, tcp::acceptor& acceptor, asio::cancellation_signal& stop)
{
    tcp::socket socket{co_await asio::this_coro::executor};
    co_await socket.async_connect({asio::ip::make_address("127.0.0.1"), port}, use_awaitable);

    constexpr std::string_view payload = "coroutine-asio-echo";
    co_await asio::async_write(socket, asio::buffer(payload), use_awaitable);

    std::array<char, payload.size()> reply{};
    co_await asio::async_read(socket, asio::buffer(reply), use_awaitable);
    coroutine_study::check(std::string_view(reply.data(), reply.size()) == payload, "echo payload mismatch");

    socket.close();
    stop.emit(asio::cancellation_type::all);
    acceptor.cancel();
}

int main()
{
    asio::io_context ctx;
    asio::cancellation_signal stop;
    tcp::acceptor acceptor{ctx, {tcp::v4(), 0}};
    const auto port = acceptor.local_endpoint().port();
    std::vector<std::future<void>> sessions;

    auto listener_done = co_spawn(ctx, listener(acceptor, stop, sessions), asio::use_future);
    auto client_done = co_spawn(ctx, client(port, acceptor, stop), asio::use_future);
    ctx.run();
    client_done.get();
    listener_done.get();
    for (auto& session : sessions) session.get();

    std::cout << "I1 reference passed: loopback echo and accept cancellation observed\n";
}
