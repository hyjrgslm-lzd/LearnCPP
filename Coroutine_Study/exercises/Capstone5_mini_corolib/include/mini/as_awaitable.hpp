// =============================================================================
// mini/as_awaitable.hpp —— stdexec sender -> awaitable 桥接
//
// 对应文档：14-第三阶段结课-mini协程库实现.md  §"第九层"
//
// 设计要点（参照模块 H-1）：
//   - bridge_receiver 的 set_value/error/stopped 都 resume coroutine_handle；
//   - operation_state 必须存活到 sender 完成 -> 内联存储于 sender_awaitable；
//   - 同步完成路径上 await_suspend 应返回 void（避免恢复后重新挂起）。
//
// 用法：
//   mini::task<int> f() {
//       int x = co_await mini::as_awaitable(stdexec::just(42));
//       co_return x;
//   }
// =============================================================================

#pragma once

#include <coroutine>
#include <exception>
#include <optional>
#include <utility>
#include <variant>

// 用户编译此 header 时若没链 stdexec，会缺定义 —— CMakeLists 已链好
// #include <stdexec/execution.hpp>

namespace mini {

// 简化版：仅支持 set_value(T) 单值 sender
template <typename Sender, typename T = int>
struct sender_awaitable {
    Sender sender_;
    std::variant<std::monostate, T, std::exception_ptr> result_{};
    std::coroutine_handle<>                              caller_{};

    // 内联存储 op_state —— 真实实现用 std::aligned_storage_t + placement new
    // 类型由 stdexec::connect_result_t<Sender, bridge_receiver> 决定
    // 这里仅占位
    // std::optional<connect_result_t<...>> op_;

    bool await_ready() noexcept { return false; }

    void await_suspend(std::coroutine_handle<> h) {
        caller_ = h;
        // TODO[必做]:
        //   op_.emplace(stdexec::connect(std::move(sender_), bridge_receiver{this}));
        //   stdexec::start(*op_);
        //
        // 注意：如果 sender 同步完成，set_value 会在此函数返回前已 resume —— 此时
        // 应返回 void（或 noop_coroutine），不要返回 bool。
    }

    T await_resume() {
        if (result_.index() == 2) std::rethrow_exception(std::get<2>(result_));
        return std::move(std::get<1>(result_));
    }
};

template <typename Sender>
auto as_awaitable(Sender&& s) {
    return sender_awaitable<std::decay_t<Sender>>{std::forward<Sender>(s)};
}

// bridge_receiver 接口形状
//   struct bridge_receiver {
//       sender_awaitable* parent_;
//       template <typename T>
//       friend void tag_invoke(set_value_t, bridge_receiver&& self, T&& v) {
//           self.parent_->result_.emplace<1>(std::forward<T>(v));
//           self.parent_->caller_.resume();
//       }
//       friend void tag_invoke(set_error_t, bridge_receiver&& self, std::exception_ptr ep) noexcept {
//           self.parent_->result_.emplace<2>(std::move(ep));
//           self.parent_->caller_.resume();
//       }
//       friend void tag_invoke(set_stopped_t, bridge_receiver&& self) noexcept {
//           // TODO: 标记 stopped
//           self.parent_->caller_.resume();
//       }
//       friend auto tag_invoke(get_env_t, const bridge_receiver&) noexcept {
//           return stdexec::empty_env{};
//       }
//   };

} // namespace mini
