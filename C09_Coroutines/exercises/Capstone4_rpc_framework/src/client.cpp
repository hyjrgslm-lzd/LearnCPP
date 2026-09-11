#include "rpc/client.hpp"

#include <stdexcept>
#include <utility>

namespace rpc {

namespace {

[[noreturn]] void todo(const char* name) {
    throw std::logic_error(std::string{"TODO: implement "} + name);
}

} // namespace

queued_writer::queued_writer(std::shared_ptr<tcp::socket> socket, std::atomic<int>& in_flight)
    : socket_(std::move(socket)), in_flight_(in_flight) {}

void queued_writer::send(std::string) {
    todo("rpc::queued_writer::send");
}

asio::awaitable<void> queued_writer::write_loop() {
    co_return;
}

RpcClient::RpcClient(asio::io_context& io)
    : io_(io),
      socket_(std::make_shared<tcp::socket>(io)),
      writer_(std::make_shared<queued_writer>(socket_, in_flight_)) {}

asio::awaitable<void> RpcClient::connect(std::string, std::uint16_t) {
    todo("rpc::RpcClient::connect");
}

asio::awaitable<std::expected<Response, RpcError>> RpcClient::call(
    Request,
    std::chrono::milliseconds,
    int) {
    todo("rpc::RpcClient::call");
}

void RpcClient::shutdown() {
    pending_.clear();
    asio::error_code ignored;
    if (socket_) socket_->close(ignored);
}

int RpcClient::in_flight() const noexcept {
    return in_flight_.load();
}

asio::awaitable<void> RpcClient::read_loop() {
    co_return;
}

void RpcClient::fail_all(RpcError) {
    pending_.clear();
}

} // namespace rpc
