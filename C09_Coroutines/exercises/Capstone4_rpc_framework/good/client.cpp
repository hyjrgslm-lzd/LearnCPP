#include "rpc/client.hpp"

#include <utility>

namespace rpc {

queued_writer::queued_writer(std::shared_ptr<tcp::socket> socket, std::atomic<int>& in_flight)
    : socket_(std::move(socket)), in_flight_(in_flight) {}

void queued_writer::send(std::string data) {
    queue_.push_back(std::move(data));
    if (!writing_) {
        writing_ = true;
        ++in_flight_;
        auto self = shared_from_this();
        asio::co_spawn(socket_->get_executor(), self->write_loop(),
            [self](std::exception_ptr) { --self->in_flight_; });
    }
}

asio::awaitable<void> queued_writer::write_loop() {
    while (!queue_.empty()) {
        auto data = std::move(queue_.front());
        queue_.pop_front();
        asio::error_code ec;
        co_await asio::async_write(*socket_, asio::buffer(data), asio::redirect_error(asio::use_awaitable, ec));
        if (ec) break;
    }
    writing_ = false;
}

RpcClient::RpcClient(asio::io_context& io)
    : io_(io),
      socket_(std::make_shared<tcp::socket>(io)),
      writer_(std::make_shared<queued_writer>(socket_, in_flight_)) {}

asio::awaitable<void> RpcClient::connect(std::string host, std::uint16_t port) {
    co_await socket_->async_connect(tcp::endpoint{asio::ip::make_address(host), port}, asio::use_awaitable);
    ++in_flight_;
    asio::co_spawn(io_, read_loop(), [this](std::exception_ptr) { --in_flight_; });
}

asio::awaitable<std::expected<Response, RpcError>> RpcClient::call(
    Request req,
    std::chrono::milliseconds timeout,
    int retries) {
    for (int attempt = 0;; ++attempt) {
        req.req_id = ++next_id_;
        auto wire = serialize(req);
        if (wire.empty()) co_return std::unexpected(RpcError::SerializationError);

        auto state = std::make_shared<pending_state>(io_);
        pending_.emplace(req.req_id, state);
        writer_->send(std::move(wire));

        state->timer.expires_after(timeout);
        asio::error_code ec;
        co_await state->timer.async_wait(asio::redirect_error(asio::use_awaitable, ec));
        if (ec == asio::error::operation_aborted && state->result) {
            auto result = *state->result;
            if (!result) co_return std::unexpected(result.error());
            if (result->status == "ok") co_return result;
            if (result->status == "unknown_method") co_return std::unexpected(RpcError::UnknownMethod);
            co_return std::unexpected(RpcError::ServerError);
        }

        pending_.erase(req.req_id);
        if (auto cancel = encode_cancel(req.req_id)) writer_->send(std::move(*cancel));
        if (!req.idempotent || attempt >= retries) co_return std::unexpected(RpcError::Timeout);
    }
}

void RpcClient::shutdown() {
    fail_all(RpcError::ConnectionLost);
    asio::error_code ignored;
    socket_->close(ignored);
}

int RpcClient::in_flight() const noexcept {
    return in_flight_.load();
}

asio::awaitable<void> RpcClient::read_loop() {
    for (;;) {
        auto body = co_await read_frame(socket_);
        if (!body) {
            fail_all(body.error());
            co_return;
        }
        auto resp = parse_response(*body);
        if (!resp) {
            fail_all(resp.error());
            co_return;
        }
        auto it = pending_.find(resp->req_id);
        if (it == pending_.end()) continue;
        it->second->result = *resp;
        it->second->timer.cancel();
        pending_.erase(it);
    }
}

void RpcClient::fail_all(RpcError value) {
    for (auto& [_, state] : pending_) {
        state->result = std::unexpected(value);
        state->timer.cancel();
    }
    pending_.clear();
}

} // namespace rpc
