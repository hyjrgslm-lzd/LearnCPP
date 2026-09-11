#include "rpc/protocol.hpp"

#include <charconv>

namespace rpc {

std::string serialize(const Request& req) {
    return "Q|" + std::to_string(req.req_id) + "|" + (req.idempotent ? "1" : "0") + "|" +
           req.method + "\n";
}

std::string serialize(const Response& resp) {
    return "R|" + std::to_string(resp.req_id) + "|" + std::to_string(resp.result) + "|" +
           resp.status + "\n";
}

std::expected<std::string, RpcError> frame(std::string body) {
    return body + "\n";
}

std::expected<std::string, RpcError> encode_cancel(std::uint32_t req_id) {
    return "C|" + std::to_string(req_id) + "\n";
}

asio::awaitable<std::expected<std::string, RpcError>> read_frame(std::shared_ptr<tcp::socket>) {
    co_return std::unexpected(RpcError::ConnectionLost);
}

std::expected<Request, RpcError> parse_request(std::string_view) {
    return std::unexpected(RpcError::SerializationError);
}

std::expected<Response, RpcError> parse_response(std::string_view) {
    return std::unexpected(RpcError::SerializationError);
}

std::expected<std::uint32_t, RpcError> parse_cancel(std::string_view) {
    return std::unexpected(RpcError::SerializationError);
}

} // namespace rpc
