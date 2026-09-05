#pragma once

#include <asio.hpp>

#include <atomic>
#include <charconv>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <deque>
#include <expected>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace rpc_ref {

using asio::ip::tcp;
using namespace std::chrono_literals;

constexpr std::size_t max_frame_size = 4096;

enum class error { timeout, server_error, connection_lost, unknown_method, protocol };

struct request {
    std::uint32_t id{};
    std::string method;
    std::vector<int> args;
    bool idempotent{};
};

struct response {
    std::uint32_t id{};
    int result{};
    std::string status;
};

inline bool parse_u32(std::string_view text, std::uint32_t& out) {
    auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), out);
    return ec == std::errc{} && ptr == text.data() + text.size();
}

inline bool parse_int(std::string_view text, int& out) {
    auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), out);
    return ec == std::errc{} && ptr == text.data() + text.size();
}

inline std::vector<std::string_view> split_fields(std::string_view text) {
    std::vector<std::string_view> fields;
    std::size_t begin = 0;
    while (begin <= text.size()) {
        auto end = text.find('|', begin);
        if (end == std::string_view::npos) end = text.size();
        fields.push_back(text.substr(begin, end - begin));
        begin = end + 1;
        if (end == text.size()) break;
    }
    return fields;
}

inline std::string join_args(const std::vector<int>& args) {
    std::string out;
    for (std::size_t i = 0; i < args.size(); ++i) {
        if (i) out += ',';
        out += std::to_string(args[i]);
    }
    return out;
}

inline std::expected<std::vector<int>, error> split_args(std::string_view text) {
    std::vector<int> values;
    if (text.empty()) return values;
    std::size_t begin = 0;
    while (begin <= text.size()) {
        auto end = text.find(',', begin);
        if (end == std::string_view::npos) end = text.size();
        int value{};
        if (!parse_int(text.substr(begin, end - begin), value)) return std::unexpected(error::protocol);
        values.push_back(value);
        begin = end + 1;
        if (end == text.size()) break;
    }
    return values;
}

inline std::expected<std::string, error> frame(std::string body) {
    if (body.size() > max_frame_size) return std::unexpected(error::protocol);
    auto len = std::to_string(body.size());
    return std::string(8 - len.size(), '0') + len + body;
}

inline std::expected<std::string, error> encode(const request& req) {
    return frame("Q|" + std::to_string(req.id) + "|" + (req.idempotent ? "1" : "0") + "|" +
                 req.method + "|" + join_args(req.args));
}

inline std::expected<std::string, error> encode_cancel(std::uint32_t id) {
    return frame("C|" + std::to_string(id));
}

inline std::expected<std::string, error> encode(const response& resp) {
    return frame("R|" + std::to_string(resp.id) + "|" + std::to_string(resp.result) + "|" + resp.status);
}

inline asio::awaitable<std::expected<std::string, error>> read_frame(std::shared_ptr<tcp::socket> socket) {
    char header[8]{};
    asio::error_code ec;
    co_await asio::async_read(*socket, asio::buffer(header), asio::redirect_error(asio::use_awaitable, ec));
    if (ec) co_return std::unexpected(error::connection_lost);

    for (char c : header) {
        if (!std::isdigit(static_cast<unsigned char>(c))) co_return std::unexpected(error::protocol);
    }

    std::uint32_t length{};
    if (!parse_u32(std::string_view{header, 8}, length) || length > max_frame_size) {
        co_return std::unexpected(error::protocol);
    }

    std::string body(length, '\0');
    co_await asio::async_read(*socket, asio::buffer(body), asio::redirect_error(asio::use_awaitable, ec));
    if (ec) co_return std::unexpected(error::connection_lost);
    co_return body;
}

inline std::expected<request, error> parse_request(std::string_view body) {
    auto f = split_fields(body);
    if (f.size() != 5 || f[0] != "Q" || (f[2] != "0" && f[2] != "1")) return std::unexpected(error::protocol);
    std::uint32_t id{};
    if (!parse_u32(f[1], id)) return std::unexpected(error::protocol);
    auto args = split_args(f[4]);
    if (!args) return std::unexpected(error::protocol);
    return request{id, std::string{f[3]}, std::move(*args), f[2] == "1"};
}

inline std::expected<std::uint32_t, error> parse_cancel(std::string_view body) {
    auto f = split_fields(body);
    if (f.size() != 2 || f[0] != "C") return std::unexpected(error::protocol);
    std::uint32_t id{};
    if (!parse_u32(f[1], id)) return std::unexpected(error::protocol);
    return id;
}

inline std::expected<response, error> parse_response(std::string_view body) {
    auto f = split_fields(body);
    if (f.size() != 4 || f[0] != "R") return std::unexpected(error::protocol);
    std::uint32_t id{};
    int result{};
    if (!parse_u32(f[1], id) || !parse_int(f[2], result)) return std::unexpected(error::protocol);
    return response{id, result, std::string{f[3]}};
}

class queued_writer : public std::enable_shared_from_this<queued_writer> {
public:
    queued_writer(std::shared_ptr<tcp::socket> socket, std::atomic<int>& in_flight)
        : socket_(std::move(socket)), in_flight_(in_flight) {}

    void send(std::string data) {
        queue_.push_back(std::move(data));
        if (!writing_) {
            writing_ = true;
            ++in_flight_;
            auto self = shared_from_this();
            asio::co_spawn(socket_->get_executor(), self->write_loop(),
                [self](std::exception_ptr) { --self->in_flight_; });
        }
    }

private:
    asio::awaitable<void> write_loop() {
        while (!queue_.empty()) {
            auto data = std::move(queue_.front());
            queue_.pop_front();
            asio::error_code ec;
            co_await asio::async_write(*socket_, asio::buffer(data), asio::redirect_error(asio::use_awaitable, ec));
            if (ec) break;
        }
        writing_ = false;
    }

    std::shared_ptr<tcp::socket> socket_;
    std::atomic<int>& in_flight_;
    std::deque<std::string> queue_;
    bool writing_{};
};

class server {
public:
    explicit server(asio::io_context& io) : io_(io), acceptor_(io, tcp::endpoint(tcp::v4(), 0)) {}

    std::uint16_t port() const { return acceptor_.local_endpoint().port(); }

    void start() {
        acceptor_.listen();
        ++in_flight_;
        asio::co_spawn(io_, accept_loop(), [this](std::exception_ptr) { --in_flight_; });
    }

    void stop() {
        asio::error_code ignored;
        acceptor_.close(ignored);
    }

    int in_flight() const noexcept { return in_flight_.load(); }

private:
    struct cancel_state {
        bool cancelled{};
    };

    asio::awaitable<void> accept_loop() {
        for (;;) {
            asio::error_code ec;
            auto socket = co_await acceptor_.async_accept(asio::redirect_error(asio::use_awaitable, ec));
            if (ec) co_return;
            auto shared_socket = std::make_shared<tcp::socket>(std::move(socket));
            ++in_flight_;
            asio::co_spawn(io_, handle_connection(shared_socket),
                [this](std::exception_ptr) { --in_flight_; });
        }
    }

    asio::awaitable<void> handle_connection(std::shared_ptr<tcp::socket> socket) {
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
            if (auto it = cancel->find(req->id); it != cancel->end()) it->second->cancelled = true;
            (*cancel)[req->id] = state;
            ++in_flight_;
            asio::co_spawn(io_, handle_request(writer, *req, state),
                [this, cancel, id = req->id, state](std::exception_ptr) {
                    if (auto it = cancel->find(id); it != cancel->end() && it->second == state) {
                        cancel->erase(it);
                    }
                    --in_flight_;
                });
        }
    }

    asio::awaitable<void> handle_request(std::shared_ptr<queued_writer> writer,
                                         request req,
                                         std::shared_ptr<cancel_state> cancel) {
        auto resp = co_await dispatch(req, cancel);
        if (cancel->cancelled) co_return;
        if (auto wire = encode(resp)) writer->send(std::move(*wire));
    }

    asio::awaitable<response> dispatch(const request& req, std::shared_ptr<cancel_state> cancel) {
        response resp{req.id, 0, "ok"};
        if (req.method == "add" || req.method == "delay_add") {
            if (req.method == "delay_add") {
                auto delay = req.args.empty() ? 0 : req.args.front();
                for (int elapsed = 0; elapsed < delay && !cancel->cancelled; elapsed += 10) {
                    asio::steady_timer timer{io_, 10ms};
                    co_await timer.async_wait(asio::use_awaitable);
                }
                if (cancel->cancelled) co_return resp;
            }
            for (int x : req.args) resp.result += x;
        } else if (req.method == "error_method") {
            resp.status = "error";
        } else {
            resp.status = "unknown_method";
        }
        co_return resp;
    }

    asio::io_context& io_;
    tcp::acceptor acceptor_;
    std::atomic<int> in_flight_{0};
};

class client {
public:
    explicit client(asio::io_context& io)
        : io_(io),
          socket_(std::make_shared<tcp::socket>(io)),
          writer_(std::make_shared<queued_writer>(socket_, in_flight_)) {}

    asio::awaitable<void> connect(std::string host, std::uint16_t port) {
        co_await socket_->async_connect(
            tcp::endpoint{asio::ip::make_address(host), port}, asio::use_awaitable);
        ++in_flight_;
        asio::co_spawn(io_, read_loop(), [this](std::exception_ptr) { --in_flight_; });
    }

    asio::awaitable<std::expected<response, error>> call(request req,
                                                        std::chrono::milliseconds timeout,
                                                        int retries = 0) {
        for (int attempt = 0;; ++attempt) {
            req.id = ++next_id_;
            auto wire = encode(req);
            if (!wire) co_return std::unexpected(wire.error());
            auto state = std::make_shared<pending_state>(io_);
            pending_.emplace(req.id, state);
            writer_->send(std::move(*wire));

            state->timer.expires_after(timeout);
            asio::error_code ec;
            co_await state->timer.async_wait(asio::redirect_error(asio::use_awaitable, ec));
            if (ec == asio::error::operation_aborted && state->result) {
                auto result = *state->result;
                if (!result) co_return std::unexpected(result.error());
                if (result->status == "ok") co_return result;
                if (result->status == "unknown_method") co_return std::unexpected(error::unknown_method);
                co_return std::unexpected(error::server_error);
            }

            pending_.erase(req.id);
            if (auto cancel = encode_cancel(req.id)) writer_->send(std::move(*cancel));
            if (!req.idempotent || attempt >= retries) co_return std::unexpected(error::timeout);
        }
    }

    void shutdown() {
        fail_all(error::connection_lost);
        asio::error_code ignored;
        socket_->close(ignored);
    }

    int in_flight() const noexcept { return in_flight_.load(); }

private:
    struct pending_state {
        explicit pending_state(asio::io_context& io) : timer(io) {}
        asio::steady_timer timer;
        std::optional<std::expected<response, error>> result;
    };

    asio::awaitable<void> read_loop() {
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
            auto it = pending_.find(resp->id);
            if (it == pending_.end()) continue;
            it->second->result = *resp;
            it->second->timer.cancel();
            pending_.erase(it);
        }
    }

    void fail_all(error value) {
        for (auto& [_, state] : pending_) {
            state->result = std::unexpected(value);
            state->timer.cancel();
        }
        pending_.clear();
    }

    asio::io_context& io_;
    std::shared_ptr<tcp::socket> socket_;
    std::atomic<int> in_flight_{0};
    std::shared_ptr<queued_writer> writer_;
    std::uint32_t next_id_{0};
    std::unordered_map<std::uint32_t, std::shared_ptr<pending_state>> pending_;
};

} // namespace rpc_ref
