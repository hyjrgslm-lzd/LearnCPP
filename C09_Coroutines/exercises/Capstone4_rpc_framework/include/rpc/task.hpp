// =============================================================================
// rpc/task.hpp —— Capstone4 的协程返回类型契约
//
// 对应文档：13-第三阶段结课-RPC框架.md（Client/Server handler 的协程载体）
//
// Capstone4 专注纯 Asio RPC。自写 task<T>、sender/awaitable bridge 和
// symmetric transfer 放在 H 模块与 Capstone5 中练，这里只保留一个别名，
// 让 client/server 的接口形状直接暴露 Asio 的实际工程模型。
// =============================================================================

#pragma once

#include <asio.hpp>

namespace rpc {

template <typename T = void>
using task = asio::awaitable<T>;

} // namespace rpc
