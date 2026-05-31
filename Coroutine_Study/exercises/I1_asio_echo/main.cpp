// I-1 Asio + awaitable 回声服务器
// 文档参考：11-模块I-真实异步IO与并发框架.md 「练习 I-1」
// 官方参考：
//   - Asio C++20 coroutines: https://think-async.com/Asio/asio-1.30.2/doc/asio/overview/composition/cpp20_coroutines.html
//   - Asio cancellation:     https://think-async.com/Asio/asio-1.30.2/doc/asio/overview/core/cancellation.html
//   - Asio parallel_group examples (asio/src/examples/cpp20/coroutines/parallel_group.cpp)
//   - Lewis Baker "C++ Coroutines and Asio" CppCon 2022
//
// 目标：用 co_spawn + awaitable<void> 搭建可运行的回声服务器，监听 12345 端口；
//      用 cancellation_signal 实现 Enter 优雅关停；
//      用 asio::as_tuple(use_awaitable) 让 (ec, n) 取代 try/catch；
//      用 bind_cancellation_slot(slot, asio::as_tuple(use_awaitable)) 包装
//      整个 completion token，把 cancel 信号注入 async 操作。
//
// 关键点：Asio awaitable 协程已有内置 cancellation_slot（由 co_spawn 注入并随
//        co_await 传播）。生产代码中通常在 co_spawn 处一次绑定，本骨架同时演示
//        了"在 async 操作上手动 bind"以便对照。

#include <asio.hpp>
#include <asio/experimental/parallel_group.hpp>
#include <asio/experimental/awaitable_operators.hpp>

#include <array>
#include <cstdio>
#include <iostream>
#include <thread>

using asio::awaitable;
using asio::co_spawn;
using asio::detached;
using asio::use_awaitable;
using asio::ip::tcp;

// ============ session：单客户端回声循环 ============
awaitable<void> echo_session(tcp::socket socket)
{
    try {
        std::array<char, 1024> buf;
        for (;;) {
            // as_tuple(use_awaitable)：返回 tuple<error_code, size_t>
            // 不再走异常路径处理普通错误（EOF/对端关闭）
            auto [ec, n] = co_await socket.async_read_some(
                asio::buffer(buf),
                asio::as_tuple(use_awaitable)
            );
            if (ec) {
                if (ec != asio::error::eof) {
                    std::fprintf(stderr, "[session] read error: %s\n",
                                 ec.message().c_str());
                }
                co_return;
            }
            // 写回
            co_await asio::async_write(
                socket, asio::buffer(buf, n),
                use_awaitable
            );
        }
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[session] exception: %s\n", e.what());
    }
    co_return;
}

// ============ listener：接收连接 + 取消支持 ============
awaitable<void> listener(tcp::acceptor& acceptor,
                         asio::cancellation_signal& cancel_signal)
{
    for (;;) {
        // 关键：bind_cancellation_slot 包装整个 completion token，
        //      把 cancel slot 注入到这一次 async_accept 上
        auto slot = cancel_signal.slot();
        auto [ec, sock] = co_await acceptor.async_accept(
            asio::bind_cancellation_slot(slot, asio::as_tuple(use_awaitable))
        );

        if (ec) {
            if (ec == asio::error::operation_aborted) {
                std::printf("[listener] cancelled, shutting down\n");
                co_return;
            }
            std::fprintf(stderr, "[listener] accept error: %s\n",
                         ec.message().c_str());
            continue;
        }

        std::printf("[listener] new connection from %s:%u\n",
                    sock.remote_endpoint().address().to_string().c_str(),
                    sock.remote_endpoint().port());

        // 为每个客户端 fire-and-forget 一个 echo_session
        co_spawn(co_await asio::this_coro::executor,
                 echo_session(std::move(sock)),
                 detached);
    }
}

int main(int argc, char* argv[])
{
    unsigned short port = 12345;
    if (argc > 1) port = static_cast<unsigned short>(std::atoi(argv[1]));

    try {
        asio::io_context ctx;
        asio::cancellation_signal cancel_signal;

        tcp::acceptor acceptor(ctx, tcp::endpoint(tcp::v4(), port));
        std::printf("[main] echo server listening on %u\n", port);
        std::printf("[main] press Enter to shut down\n");

        co_spawn(ctx, listener(acceptor, cancel_signal), detached);

        // 关停线程：等用户敲 Enter 后 emit cancel
        std::thread shutdown_thread([&] {
            std::cin.get();
            std::printf("[main] shutdown requested\n");
            asio::post(ctx, [&] {
                cancel_signal.emit(asio::cancellation_type::all);
                acceptor.close();   // 兜底：关闭 acceptor 让 io_context 退出
            });
        });

        ctx.run();
        shutdown_thread.join();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[main] fatal: %s\n", e.what());
        return 1;
    }

    // TODO [必做]：用 nc/telnet 连接 localhost:12345 测试多客户端并发回声。
    // TODO [必做]：在笔记里画 io_context::run() 单线程多协程调度循环。
    // TODO [进阶]：把单线程 io_context 换成 asio::thread_pool，找出哪里需要锁。
    // TODO [进阶]：用 parallel_group + steady_timer 实现 30s 空闲超时关闭连接。
    // TODO [进阶]：把 echo_session 也注入 cancellation_slot，实现连级取消。

    return 0;
}
