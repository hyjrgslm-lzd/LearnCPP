// =============================================================================
// rpc/stop_token.hpp —— 协作式取消（stop_token）
//
// 对应文档：13-第三阶段结课-RPC框架.md
//   §"Cancellation 传播路径" / §"必做任务 - 取消"
//
// 设计要点：
//   - 直接 typedef 标准库类型，避免重复造轮；
//   - 在 environment 中以 get_stop_token 暴露，receiver 可查询；
//   - 工程上，Cancellation 传播路径：
//       client 超时 -> stop_source.request_stop()
//       -> stop_token 通过 environment 传到 server handler
//       -> 在 handler 关键点 co_await with_stop_check
//       -> set_stopped 通道收束。
// =============================================================================

#pragma once

#include <stop_token>
#include <atomic>

namespace rpc {

using stop_source = std::stop_source;
using stop_token  = std::stop_token;
// std::stop_callback 是类模板，必须带回调类型实参；用 alias template 转发，
// 用法：rpc::stop_callback_for_t<MyCallback> cb{tok, MyCallback{}};
template <class F>
using stop_callback_for_t = std::stop_callback<F>;

// TODO[必做]: 实现 stop_token-aware awaitable 包装：
// template <typename Inner>
// struct with_stop_check {
//     Inner inner_;
//     stop_token tok_;
//     bool await_ready();
//     auto await_suspend(std::coroutine_handle<> h);
//     decltype(auto) await_resume();
// };

} // namespace rpc
