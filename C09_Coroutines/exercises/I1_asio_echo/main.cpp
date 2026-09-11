#include <asio.hpp>

#include <array>
#include <chrono>
#include <future>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

using asio::awaitable;
using asio::use_awaitable;
using asio::ip::tcp;

awaitable<void> echo_session(tcp::socket socket)
{
    std::array<char, 1024> buffer{};
    (void)socket;
    (void)buffer;

    // TODO: loop on async_read_some(as_tuple(use_awaitable)).
    // TODO: echo successful reads with async_write.
    // TODO: treat EOF/cancel as normal exit, not as a fatal exception.
    co_return;
}

awaitable<void> listener(tcp::acceptor& acceptor, asio::cancellation_signal& stop)
{
    (void)acceptor;
    (void)stop;

    // TODO: co_await async_accept(bind_cancellation_slot(stop.slot(), as_tuple(use_awaitable))).
    // TODO: co_spawn one echo_session per accepted socket on this_coro::executor.
    // Safe starter: no accept is posted yet, so running this target never blocks.
    co_return;
}

awaitable<bool> roundtrip(unsigned short port, asio::cancellation_signal& stop)
{
    tcp::socket socket{co_await asio::this_coro::executor};
    asio::steady_timer timer{co_await asio::this_coro::executor};
    timer.expires_after(std::chrono::milliseconds{250});
    timer.async_wait([&](std::error_code ec) {
        if (!ec) socket.cancel();
    });

    std::error_code ec;
    co_await socket.async_connect({asio::ip::make_address("127.0.0.1"), port},
                                  asio::redirect_error(use_awaitable, ec));
    if (ec) co_return false;

    constexpr std::string_view payload = "coroutine-asio-echo";
    co_await asio::async_write(socket, asio::buffer(payload), asio::redirect_error(use_awaitable, ec));
    if (ec) co_return false;

    std::array<char, payload.size()> reply{};
    co_await asio::async_read(socket, asio::buffer(reply), asio::redirect_error(use_awaitable, ec));
    stop.emit(asio::cancellation_type::all);
    co_return !ec && std::string_view(reply.data(), reply.size()) == payload;
}

int main()
{
    asio::io_context ctx;
    asio::cancellation_signal stop;
    tcp::acceptor acceptor{ctx, {tcp::v4(), 0}};
    auto port = acceptor.local_endpoint().port();

    auto server = asio::co_spawn(ctx, listener(acceptor, stop), asio::use_future);
    auto client = asio::co_spawn(ctx, roundtrip(port, stop), asio::use_future);
    ctx.run();

    bool ok = false;
    try {
        ok = client.get();
        server.get();
    } catch (...) {
        ok = false;
    }
    if (!ok) {
        std::cout << "student check failed: no bounded loopback echo observed\n";
        return 1;
    }
    std::cout << "I1 student check passed.\n";
}
