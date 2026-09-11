#include "rpc/server.hpp"

#include <stdexcept>
#include <utility>

namespace rpc {

namespace {

[[noreturn]] void todo(const char* name) {
    throw std::logic_error(std::string{"TODO: implement "} + name);
}

} // namespace

task<Response> handler_add(Request) {
    todo("rpc::handler_add");
}

task<Response> handler_delay_add(Request) {
    todo("rpc::handler_delay_add");
}

task<Response> handler_error_method(Request) {
    todo("rpc::handler_error_method");
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
    todo("rpc::RpcServer::start");
}

void RpcServer::stop() {
    asio::error_code ignored;
    acceptor_.close(ignored);
}

int RpcServer::in_flight() const noexcept {
    return in_flight_.load();
}

asio::awaitable<void> RpcServer::accept_loop() {
    co_return;
}

asio::awaitable<void> RpcServer::handle_connection(std::shared_ptr<tcp::socket>) {
    co_return;
}

asio::awaitable<void> RpcServer::handle_request(std::shared_ptr<queued_writer>, Request, std::shared_ptr<cancel_state>) {
    co_return;
}

asio::awaitable<Response> RpcServer::dispatch(const Request&, std::shared_ptr<cancel_state>) {
    todo("rpc::RpcServer::dispatch");
}

} // namespace rpc
