#include "rpc/protocol.hpp"

#include <charconv>
#include <cctype>

namespace rpc {
namespace {

bool u32(std::string_view s, std::uint32_t& v) {
    auto [p, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
    return ec == std::errc{} && p == s.data() + s.size();
}

bool integer(std::string_view s, int& v) {
    auto [p, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
    return ec == std::errc{} && p == s.data() + s.size();
}

std::vector<std::string_view> fields(std::string_view s) {
    std::vector<std::string_view> out;
    for (std::size_t begin = 0; begin <= s.size();) {
        auto end = s.find('|', begin);
        if (end == std::string_view::npos) end = s.size();
        out.push_back(s.substr(begin, end - begin));
        begin = end + 1;
        if (end == s.size()) break;
    }
    return out;
}

std::string body(std::string_view wire) {
    std::uint32_t n{};
    if (wire.size() >= 8 && u32(wire.substr(0, 8), n) && wire.size() == n + 8) {
        return std::string{wire.substr(8)};
    }
    return std::string{wire};
}

std::string args_text(const std::vector<int>& args) {
    std::string out;
    for (std::size_t i = 0; i < args.size(); ++i) {
        if (i) out += ',';
        out += std::to_string(args[i]);
    }
    return out;
}

std::expected<std::vector<int>, RpcError> args(std::string_view s) {
    std::vector<int> out;
    if (s.empty()) return out;
    for (std::size_t begin = 0; begin <= s.size();) {
        auto end = s.find(',', begin);
        if (end == std::string_view::npos) end = s.size();
        int v{};
        if (!integer(s.substr(begin, end - begin), v)) {
            return std::unexpected(RpcError::SerializationError);
        }
        out.push_back(v);
        begin = end + 1;
        if (end == s.size()) break;
    }
    return out;
}

} // namespace

std::expected<std::string, RpcError> frame(std::string b) {
    if (b.size() > max_frame_size) return std::unexpected(RpcError::SerializationError);
    auto n = std::to_string(b.size());
    return std::string(8 - n.size(), '0') + n + b;
}

std::string serialize(const Request& r) {
    return frame("Q|" + std::to_string(r.req_id) + "|" + (r.idempotent ? "1" : "0") + "|" +
                 r.method + "|" + args_text(r.args)).value_or(std::string{});
}

std::string serialize(const Response& r) {
    return frame("R|" + std::to_string(r.req_id) + "|" + std::to_string(r.result) + "|" + r.status)
        .value_or(std::string{});
}

std::expected<std::string, RpcError> encode_cancel(std::uint32_t id) {
    return frame("C|" + std::to_string(id));
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
    std::uint32_t n{};
    if (!u32({header, 8}, n) || n > max_frame_size) co_return std::unexpected(RpcError::SerializationError);
    std::string b(n, '\0');
    co_await asio::async_read(*socket, asio::buffer(b), asio::redirect_error(asio::use_awaitable, ec));
    if (ec) co_return std::unexpected(RpcError::ConnectionLost);
    co_return b;
}

std::expected<Request, RpcError> parse_request(std::string_view wire) {
    auto text = body(wire);
    auto f = fields(text);
    std::uint32_t id{};
    if (f.size() != 5 || f[0] != "Q" || (f[2] != "0" && f[2] != "1") || !u32(f[1], id)) {
        return std::unexpected(RpcError::SerializationError);
    }
    auto a = args(f[4]);
    if (!a) return std::unexpected(a.error());
    return Request{id, std::string{f[3]}, std::move(*a), f[2] == "1"};
}

std::expected<Response, RpcError> parse_response(std::string_view wire) {
    auto text = body(wire);
    auto f = fields(text);
    std::uint32_t id{};
    int result{};
    if (f.size() != 4 || f[0] != "R" || !u32(f[1], id) || !integer(f[2], result)) {
        return std::unexpected(RpcError::SerializationError);
    }
    return Response{id, result, std::string{f[3]}};
}

std::expected<std::uint32_t, RpcError> parse_cancel(std::string_view wire) {
    auto text = body(wire);
    auto f = fields(text);
    std::uint32_t id{};
    if (f.size() != 2 || f[0] != "C" || !u32(f[1], id)) {
        return std::unexpected(RpcError::SerializationError);
    }
    return id;
}

} // namespace rpc
