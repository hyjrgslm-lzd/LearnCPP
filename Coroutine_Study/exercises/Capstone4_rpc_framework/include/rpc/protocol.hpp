// =============================================================================
// rpc/protocol.hpp —— Request / Response / Error 类型 + wire-format
//
// 对应文档：13-第三阶段结课-RPC框架.md  §"必做任务 2"
//
// 协议：固定为
//   Request  : { req_id: uint32, method: string, args: [int] }
//   Response : { req_id: uint32, result: int, status: string }
//
// 序列化格式（实现细节，本骨架不规定）：
//   推荐 JSON 单行 + 长度前缀，便于 Asio async_read_until('\n')。
// =============================================================================

#pragma once

#include <cstdint>
#include <expected>
#include <string>
#include <vector>

namespace rpc {

struct Request {
    std::uint32_t      req_id{};
    std::string        method;
    std::vector<int>   args;
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

// TODO[必做]: serialize / deserialize
// 推荐实现签名：
//   std::string serialize(const Request&);
//   std::string serialize(const Response&);
//   std::expected<Request,  RpcError> parse_request (std::string_view wire);
//   std::expected<Response, RpcError> parse_response(std::string_view wire);
//
// 实现层面：
//   - 用 std::format 拼字符串；解析用 std::from_chars + std::string::find；
//   - 不要拉 nlohmann/json —— 第三阶段结课要求只用标准库；
//   - 帧分隔用 '\n'，body 含 '\n' 时先做转义或用长度前缀。

std::string serialize(const Request& req);
std::string serialize(const Response& resp);
std::expected<Request,  RpcError> parse_request (std::string_view wire);
std::expected<Response, RpcError> parse_response(std::string_view wire);

} // namespace rpc
