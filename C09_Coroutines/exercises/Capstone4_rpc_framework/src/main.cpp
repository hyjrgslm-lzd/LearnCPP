#include "rpc/client.hpp"
#include "rpc/server.hpp"

#include <asio.hpp>

#include <cstdlib>
#include <exception>
#include <future>
#include <iostream>
#include <memory>
#include <thread>
#include <utility>

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

int main() {
    try {
        asio::io_context io;
        auto guard = asio::make_work_guard(io);
        rpc::RpcServer server{io};
        server.start();

        rpc::RpcClient client{io};
        auto connect = asio::co_spawn(io, client.connect("127.0.0.1", server.port()), asio::use_future);
        std::jthread runner{[&] { io.run(); }};
        connect.get();

        auto result = asio::co_spawn(io, client.call({0, "add", {1, 2, 3}, true}, 1s), asio::use_future).get();
        post_and_wait(io, [&] {
            client.shutdown();
            server.stop();
        });
        guard.reset();
        runner.join();

        if (!result || result->result != 6 || client.in_flight() != 0 || server.in_flight() != 0) {
            std::cerr << "Capstone4_rpc_framework: RPC smoke failed\n";
            return 2;
        }
        std::cout << "Capstone4_rpc_framework: RPC smoke passed\n";
        return 0;
    } catch (const std::logic_error& ex) {
        std::cerr << "Capstone4_rpc_framework: " << ex.what() << '\n';
        return 2;
    }
}
