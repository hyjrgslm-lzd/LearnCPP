#include "rpc/protocol.hpp"

#include <stdexcept>

namespace rpc {

namespace {

[[noreturn]] void todo(const char* name) {
    throw std::logic_error(std::string{"TODO: implement "} + name);
}

} // namespace

std::expected<std::string, RpcError> frame(std::string) {
    todo("rpc::frame");
}

std::string serialize(const Request&) {
    todo("rpc::serialize(Request)");
}

std::string serialize(const Response&) {
    todo("rpc::serialize(Response)");
}

std::expected<std::string, RpcError> encode_cancel(std::uint32_t) {
    todo("rpc::encode_cancel");
}

asio::awaitable<std::expected<std::string, RpcError>> read_frame(std::shared_ptr<tcp::socket>) {
    todo("rpc::read_frame");
}

std::expected<Request, RpcError> parse_request(std::string_view) {
    todo("rpc::parse_request");
}

std::expected<Response, RpcError> parse_response(std::string_view) {
    todo("rpc::parse_response");
}

std::expected<std::uint32_t, RpcError> parse_cancel(std::string_view) {
    todo("rpc::parse_cancel");
}

} // namespace rpc
