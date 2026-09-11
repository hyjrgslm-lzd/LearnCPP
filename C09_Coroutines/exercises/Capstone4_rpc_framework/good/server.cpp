#include "rpc/server.hpp"

#include <utility>

namespace rpc {

using namespace std::chrono_literals;

task<Response> handler_add(Request req) {
    int sum = 0;
    for (int x : req.args) sum += x;
    co_return Response{req.req_id, sum, "ok"};
}

task<Response> handler_delay_add(Request req) {
    int sum = 0;
    for (int x : req.args) sum += x;
    co_return Response{req.req_id, sum, "ok"};
}

task<Response> handler_error_method(Request req) {
    co_return Response{req.req_id, 0, "error"};
}

RpcServer::RpcServer(asio::io_context& io)
    : io_(io), acceptor_(io, tcp::endpoint(tcp::v4(), 0)) {
    register_handler("add", &handler_add);
    register_handler("delay_add", &handler_delay_add);
    register_handler("error_method", &handler_error_method);
}

std::uint16_t RpcServer::port() const {
    return acceptor_.local_endpoint().port();
}

void RpcServer::register_handler(std::string method, Handler h) {
    handlers_.emplace(std::move(method), std::move(h));
}

void RpcServer::start() {
    acceptor_.listen();
    ++in_flight_;
    asio::co_spawn(io_, accept_loop(), [this](std::exception_ptr) { --in_flight_; });
}

void RpcServer::stop() {
    asio::error_code ignored;
    acceptor_.close(ignored);
}

int RpcServer::in_flight() const noexcept {
    return in_flight_.load();
}

asio::awaitable<void> RpcServer::accept_loop() {
    for (;;) {
        asio::error_code ec;
        auto socket = co_await acceptor_.async_accept(asio::redirect_error(asio::use_awaitable, ec));
        if (ec) co_return;
        auto shared_socket = std::make_shared<tcp::socket>(std::move(socket));
        ++in_flight_;
        asio::co_spawn(io_, handle_connection(shared_socket), [this](std::exception_ptr) { --in_flight_; });
    }
}

asio::awaitable<void> RpcServer::handle_connection(std::shared_ptr<tcp::socket> socket) {
    auto writer = std::make_shared<queued_writer>(socket, in_flight_);
    auto cancel = std::make_shared<std::unordered_map<std::uint32_t, std::shared_ptr<cancel_state>>>();
    auto cancel_all = [&] {
        for (auto& [_, state] : *cancel) state->cancelled = true;
    };

    for (;;) {
        auto body = co_await read_frame(socket);
        if (!body) {
            cancel_all();
            co_return;
        }
        if (body->starts_with("C|")) {
            if (auto id = parse_cancel(*body); id) {
                if (auto it = cancel->find(*id); it != cancel->end()) it->second->cancelled = true;
            }
            continue;
        }
        auto req = parse_request(*body);
        if (!req) {
            cancel_all();
            co_return;
        }

        auto state = std::make_shared<cancel_state>();
        if (auto it = cancel->find(req->req_id); it != cancel->end()) it->second->cancelled = true;
        (*cancel)[req->req_id] = state;
        ++in_flight_;
        asio::co_spawn(io_, handle_request(writer, *req, state),
            [this, cancel, id = req->req_id, state](std::exception_ptr) {
                if (auto it = cancel->find(id); it != cancel->end() && it->second == state) {
                    cancel->erase(it);
                }
                --in_flight_;
            });
    }
}

asio::awaitable<void> RpcServer::handle_request(std::shared_ptr<queued_writer> writer,
                                                Request req,
                                                std::shared_ptr<cancel_state> cancel) {
    auto resp = co_await dispatch(req, cancel);
    if (cancel->cancelled) co_return;
    auto wire = serialize(resp);
    if (!wire.empty()) writer->send(std::move(wire));
}

asio::awaitable<Response> RpcServer::dispatch(const Request& req, std::shared_ptr<cancel_state> cancel) {
    if (req.method == "delay_add") {
        auto delay = req.args.empty() ? 0 : req.args.front();
        for (int elapsed = 0; elapsed < delay && !cancel->cancelled; elapsed += 10) {
            asio::steady_timer timer{io_, 10ms};
            co_await timer.async_wait(asio::use_awaitable);
        }
        if (cancel->cancelled) co_return Response{req.req_id, 0, "ok"};
    }

    auto it = handlers_.find(req.method);
    if (it == handlers_.end()) co_return Response{req.req_id, 0, "unknown_method"};
    try {
        co_return co_await it->second(req);
    } catch (...) {
        co_return Response{req.req_id, 0, "error"};
    }
}

} // namespace rpc
