#include <asio.hpp>

#include <array>
#include <iostream>

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

int main()
{
    asio::io_context ctx;
    asio::cancellation_signal stop;
    tcp::acceptor acceptor{ctx};

    (void)use_awaitable;
    (void)stop;
    (void)acceptor;

    std::cout << "I1 starter skeleton compiled. Implement the TODOs, then compare with reference.\n";
}
