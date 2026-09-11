// =============================================================================
// rpc/protocol.hpp —— Request / Response / Error 类型 + wire-format
//
// 对应文档：13-第三阶段结课-RPC框架.md  §"必做任务 2"
//
// 协议：8 字节十进制长度头 + body。
//   Request  : Q|id|idempotent|method|arg0,arg1
//   Cancel   : C|id
//   Response : R|id|result|status
// =============================================================================

#pragma once

#include <asio.hpp>

#include <cstdint>
#include <cstddef>
#include <expected>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace rpc {

using asio::ip::tcp;

constexpr std::size_t max_frame_size = 4096;

struct Request {
    std::uint32_t      req_id{};
    std::string        method;
    std::vector<int>   args;
    bool               idempotent{};
};

struct Response {
    std::uint32_t req_id{};
    int           result{};
    std::string   status;  // "ok" | "timeout" | "error" | "unknown_method"
};

enum class RpcError {
    Timeout,
    ServerError,
    ConnectionLost,
    UnknownMethod,
    SerializationError,
};

std::string serialize(const Request& req);
std::string serialize(const Response& resp);
std::expected<std::string, RpcError> frame(std::string body);
std::expected<std::string, RpcError> encode_cancel(std::uint32_t req_id);
asio::awaitable<std::expected<std::string, RpcError>> read_frame(std::shared_ptr<tcp::socket> socket);

std::expected<Request,  RpcError> parse_request (std::string_view wire);
std::expected<Response, RpcError> parse_response(std::string_view wire);
std::expected<std::uint32_t, RpcError> parse_cancel(std::string_view wire);

} // namespace rpc
