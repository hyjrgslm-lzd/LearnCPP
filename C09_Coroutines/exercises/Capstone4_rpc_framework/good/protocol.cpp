#include "rpc/protocol.hpp"

#include <charconv>
#include <cctype>
#include <string>
#include <vector>

namespace rpc {

namespace {

bool parse_u32(std::string_view text, std::uint32_t& out) {
    auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), out);
    return ec == std::errc{} && ptr == text.data() + text.size();
}

bool parse_int(std::string_view text, int& out) {
    auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), out);
    return ec == std::errc{} && ptr == text.data() + text.size();
}

std::vector<std::string_view> split_fields(std::string_view text) {
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

std::string join_args(const std::vector<int>& args) {
    std::string out;
    for (std::size_t i = 0; i < args.size(); ++i) {
        if (i) out += ',';
        out += std::to_string(args[i]);
    }
    return out;
}

std::expected<std::vector<int>, RpcError> split_args(std::string_view text) {
    std::vector<int> values;
    if (text.empty()) return values;
    std::size_t begin = 0;
    while (begin <= text.size()) {
        auto end = text.find(',', begin);
        if (end == std::string_view::npos) end = text.size();
        int value{};
        if (!parse_int(text.substr(begin, end - begin), value)) {
            return std::unexpected(RpcError::SerializationError);
        }
        values.push_back(value);
        begin = end + 1;
        if (end == text.size()) break;
    }
    return values;
}

std::string body_from_wire(std::string_view wire) {
    if (wire.size() < 8) return std::string{wire};
    std::uint32_t length{};
    if (!parse_u32(wire.substr(0, 8), length) || wire.size() != length + 8) {
        return std::string{wire};
    }
    return std::string{wire.substr(8)};
}

} // namespace

std::expected<std::string, RpcError> frame(std::string body) {
    if (body.size() > max_frame_size) return std::unexpected(RpcError::SerializationError);
    auto len = std::to_string(body.size());
    return std::string(8 - len.size(), '0') + len + body;
}

std::string serialize(const Request& req) {
    auto wire = frame("Q|" + std::to_string(req.req_id) + "|" + (req.idempotent ? "1" : "0") + "|" +
                      req.method + "|" + join_args(req.args));
    return wire.value_or(std::string{});
}

std::string serialize(const Response& resp) {
    auto wire = frame("R|" + std::to_string(resp.req_id) + "|" + std::to_string(resp.result) + "|" + resp.status);
    return wire.value_or(std::string{});
}

std::expected<std::string, RpcError> encode_cancel(std::uint32_t req_id) {
    return frame("C|" + std::to_string(req_id));
}

asio::awaitable<std::expected<std::string, RpcError>> read_frame(std::shared_ptr<tcp::socket> socket) {
    char header[8]{};
    asio::error_code ec;
    co_await asio::async_read(*socket, asio::buffer(header), asio::redirect_error(asio::use_awaitable, ec));
    if (ec) co_return std::unexpected(RpcError::ConnectionLost);

    for (char c : header) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            co_return std::unexpected(RpcError::SerializationError);
        }
    }

    std::uint32_t length{};
    if (!parse_u32(std::string_view{header, 8}, length) || length > max_frame_size) {
        co_return std::unexpected(RpcError::SerializationError);
    }

    std::string body(length, '\0');
    co_await asio::async_read(*socket, asio::buffer(body), asio::redirect_error(asio::use_awaitable, ec));
    if (ec) co_return std::unexpected(RpcError::ConnectionLost);
    co_return body;
}

std::expected<Request, RpcError> parse_request(std::string_view wire) {
    auto text = body_from_wire(wire);
    auto f = split_fields(text);
    if (f.size() != 5 || f[0] != "Q" || (f[2] != "0" && f[2] != "1")) {
        return std::unexpected(RpcError::SerializationError);
    }
    std::uint32_t id{};
    if (!parse_u32(f[1], id)) return std::unexpected(RpcError::SerializationError);
    auto args = split_args(f[4]);
    if (!args) return std::unexpected(args.error());
    return Request{id, std::string{f[3]}, std::move(*args), f[2] == "1"};
}

std::expected<Response, RpcError> parse_response(std::string_view wire) {
    auto text = body_from_wire(wire);
    auto f = split_fields(text);
    if (f.size() != 4 || f[0] != "R") return std::unexpected(RpcError::SerializationError);
    std::uint32_t id{};
    int result{};
    if (!parse_u32(f[1], id) || !parse_int(f[2], result)) {
        return std::unexpected(RpcError::SerializationError);
    }
    return Response{id, result, std::string{f[3]}};
}

std::expected<std::uint32_t, RpcError> parse_cancel(std::string_view wire) {
    auto text = body_from_wire(wire);
    auto f = split_fields(text);
    if (f.size() != 2 || f[0] != "C") return std::unexpected(RpcError::SerializationError);
    std::uint32_t id{};
    if (!parse_u32(f[1], id)) return std::unexpected(RpcError::SerializationError);
    return id;
}

} // namespace rpc
