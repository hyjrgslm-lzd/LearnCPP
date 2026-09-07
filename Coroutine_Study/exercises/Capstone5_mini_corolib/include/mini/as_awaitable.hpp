// =============================================================================
// mini/as_awaitable.hpp —— stdexec sender -> awaitable 桥接
//
// 对应文档：14-第三阶段结课-mini协程库实现.md，stdexec as_awaitable 桥接层。
//
// 设计要点（参照模块 H-1）：
//   - bridge_receiver 的 set_value/error/stopped 保存结果并完成 phase 转换；
//   - operation_state 必须存活到 sender 完成，可放在 awaitable state 中；
//   - phase 使用 starting/suspended/completed/abandoned 握手；
//   - start 同步完成时 await_suspend 返回 false；
//   - 异步完成时 completion 根据 phase 决定是否 resume caller；
//   - waiter 已销毁后进入 abandoned，后续 completion 不恢复 caller。
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
#include <type_traits>
#include <utility>
#include <variant>

// 实现 TODO 时启用 stdexec 头文件；对应练习目标由 CMake 连接 stdexec。
// #include <stdexec/execution.hpp>

namespace mini {

// 简化版：仅支持 set_value(T) 单值 sender
template <typename Sender, typename T = int>
struct sender_awaitable {
    Sender sender_;
    std::variant<std::monostate, T, std::exception_ptr> result_{};
    std::coroutine_handle<>                              caller_{};

    // TODO：将 sender、result、caller、phase 与 operation 放进独立共享状态；
    // receiver 持有该状态，使其存活到 completion 回调退出。
    // operation 类型由 stdexec::connect_result_t<Sender, bridge_receiver> 决定。
    // std::optional<connect_result_t<...>> op_;

    bool await_ready() noexcept { return false; }

    bool await_suspend(std::coroutine_handle<> h) {
        caller_ = h;
        // TODO[必做]:
        //   auto keep = state_; // 独立状态的拥有引用，覆盖 start/发布 caller 的窗口。
        //   keep->phase = starting;
        //   keep->op.emplace(stdexec::connect(std::move(keep->sender), bridge_receiver{keep}));
        //   stdexec::start(*keep->op);
        //   keep->caller = h;
        //   若 receiver 已在 start 内同步完成，phase 已经是 completed，返回 false；
        //   若仍是 starting，CAS 为 suspended 并返回 true；
        //   析构时若 phase 是 suspended，CAS 为 abandoned。
        //
        // 本实现的同步完成路径只记录 completed，由返回 false 继续当前协程。
        // CAS 发布 suspended 后，completion 可以恢复并销毁 awaiter；随后只使用 keep。
        return true;
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
//       std::shared_ptr<bridge_state> state; // 完成状态独立于等待者的生命周期。
//       template <typename T>
//       friend void tag_invoke(set_value_t, bridge_receiver&& self, T&& v) {
//           auto keep = std::move(self.state);
//           keep->result_.emplace<1>(std::forward<T>(v));
//           // complete(): starting->completed 只记录同步完成；
//           // suspended->completed 才 resume caller；
//           // abandoned->completed 不恢复已销毁 waiter。
//       }
//       friend void tag_invoke(set_error_t, bridge_receiver&& self, std::exception_ptr ep) noexcept {
//           auto keep = std::move(self.state);
//           keep->result_.emplace<2>(std::move(ep));
//           // 同上：通过 phase 决定是否恢复 caller。
//       }
//       friend void tag_invoke(set_stopped_t, bridge_receiver&& self) noexcept {
//           // TODO: 标记 stopped，并通过 phase 决定是否恢复 caller。
//       }
//       friend auto tag_invoke(get_env_t, const bridge_receiver&) noexcept {
//           return stdexec::empty_env{};
//       }
//   };

} // namespace mini
