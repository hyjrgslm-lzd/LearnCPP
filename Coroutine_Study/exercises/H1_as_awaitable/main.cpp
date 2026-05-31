// H-1 as_awaitable 桥
// 文档参考：10-模块H-协程与sender_receiver桥接.md 「练习 H-1」
// 官方参考：
//   - P2300R10 std::execution（connect/start/completion channels）
//   - P3175R0 as_awaitable
//   - stdexec __connect_awaitable.hpp / __sender_for_each_awaitable.hpp
//   - LWG #4339 / #4356 as_awaitable 修正
//
// 目标：亲手实现 as_awaitable(sender) -> awaitable 桥接通道。
//      bridge_receiver 的三条 completion channel（set_value/set_error/set_stopped）
//      分别 resume 协程，await_resume 把三种完成翻译回协程的 return/throw/特殊标记。

#include <stdexec/execution.hpp>

#include <coroutine>
#include <cstdio>
#include <exception>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

namespace ex = stdexec;

// ============ bridge_receiver ============
// 关键作用：sender completion → 存储结果 → resume 协程
// 使用具名 emplace<value_type> / emplace<exception_ptr> / emplace<stopped_tag>，
// 避免 variant 重排导致的静默错位。
struct stopped_tag {};

// 起手：value_type 硬编码为 int（用于 co_await stdexec::just(42)）。
// 进阶任务：从 Sender::completion_signatures 推导。
using value_type = int;
using storage_variant = std::variant<
    std::monostate, value_type, std::exception_ptr, stopped_tag
>;

template <typename Promise>
struct bridge_receiver {
    using receiver_concept = ex::receiver_t;

    std::coroutine_handle<Promise> coro_;
    storage_variant* storage_;

    // ---- value channel ----
    template <typename... Vs>
    friend void tag_invoke(ex::set_value_t, bridge_receiver&& self,
                           Vs&&... values) noexcept
    {
        // 单值情形：直接 emplace<value_type>。
        // 多值情形：见进阶任务（用 std::tuple 包装）。
        if constexpr (sizeof...(Vs) == 1) {
            self.storage_->template emplace<value_type>(
                static_cast<value_type>(std::forward<Vs>(values))...);
        } else {
            static_assert(sizeof...(Vs) == 1,
                          "minimal bridge_receiver supports single value only");
        }
        self.coro_.resume();
    }

    // ---- error channel ----
    template <typename E>
    friend void tag_invoke(ex::set_error_t, bridge_receiver&& self,
                           E&& error) noexcept
    {
        if constexpr (std::is_same_v<std::decay_t<E>, std::exception_ptr>) {
            self.storage_->template emplace<std::exception_ptr>(
                std::forward<E>(error));
        } else {
            try {
                throw std::forward<E>(error);
            } catch (...) {
                self.storage_->template emplace<std::exception_ptr>(
                    std::current_exception());
            }
        }
        self.coro_.resume();
    }

    // ---- stopped channel ----
    friend void tag_invoke(ex::set_stopped_t, bridge_receiver&& self) noexcept {
        self.storage_->template emplace<stopped_tag>(stopped_tag{});
        self.coro_.resume();
    }

    // ---- environment ----
    friend auto tag_invoke(ex::get_env_t, const bridge_receiver& /*self*/) noexcept {
        return ex::empty_env{};
    }
};

// ============ sender_awaitable ============
// 包装 bridge_receiver 和 operation_state，是 co_await 真正交互的对象
template <typename Sender, typename Promise>
struct sender_awaitable {
    Sender sender_;
    std::coroutine_handle<Promise> coro_;
    storage_variant storage_{};

    // operation_state 必须与 sender_awaitable 同生命周期
    using receiver_t = bridge_receiver<Promise>;
    using op_state_t = ex::connect_result_t<Sender, receiver_t>;
    std::optional<op_state_t> op_state_;

    bool await_ready() noexcept { return false; }

    void await_suspend(std::coroutine_handle<Promise> h) noexcept {
        coro_ = h;
        // 在 optional 中就地构造 op_state；bridge_receiver 持有 storage 指针
        op_state_.emplace(
            ex::connect(std::move(sender_), receiver_t{h, &storage_})
        );
        ex::start(*op_state_);
        // 协程挂起；bridge_receiver 在 sender 完成时 resume
    }

    value_type await_resume() {
        if (std::holds_alternative<value_type>(storage_)) {
            return std::move(std::get<value_type>(storage_));
        }
        if (std::holds_alternative<std::exception_ptr>(storage_)) {
            std::rethrow_exception(std::get<std::exception_ptr>(storage_));
        }
        if (std::holds_alternative<stopped_tag>(storage_)) {
            throw std::runtime_error("sender stopped");
        }
        throw std::logic_error("sender_awaitable resumed without completion");
    }
};

// ============ as_awaitable 工厂 ============
template <typename Sender, typename Promise>
auto as_awaitable(Sender&& sender, std::coroutine_handle<Promise> h) {
    return sender_awaitable<std::remove_cvref_t<Sender>, std::remove_cvref_t<Promise>>{
        std::forward<Sender>(sender), h
    };
}

// ============ my_task<int>：装上 await_transform 接入 sender ============
struct my_task {
    struct promise_type {
        int result_value{};
        std::exception_ptr result_exception;

        my_task get_return_object() noexcept {
            return my_task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend()   noexcept { return {}; }
        void return_value(int v) noexcept { result_value = v; }
        void unhandled_exception() noexcept { result_exception = std::current_exception(); }

        // 关键：把任意 sender 翻译为 awaitable
        template <typename Sender>
        auto await_transform(Sender&& s) {
            return as_awaitable(
                std::forward<Sender>(s),
                std::coroutine_handle<promise_type>::from_promise(*this)
            );
        }
    };

    std::coroutine_handle<promise_type> h_;
    explicit my_task(std::coroutine_handle<promise_type> h) noexcept : h_(h) {}
    my_task(my_task&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    ~my_task() { if (h_) h_.destroy(); }

    int run() {
        h_.resume();
        while (!h_.done()) h_.resume();
        auto& p = h_.promise();
        if (p.result_exception) std::rethrow_exception(p.result_exception);
        return p.result_value;
    }
};

// ============ 测试场景 ============
my_task value_demo() {
    int x = co_await ex::just(42);
    int y = co_await ex::just(10);
    co_return x + y;        // 期望 52
}

my_task error_demo() {
    try {
        int v = co_await ex::just_error(
            std::make_exception_ptr(std::runtime_error("whoops"))
        );
        (void)v;
        co_return 0;
    } catch (const std::runtime_error& e) {
        std::printf("[error_demo] caught: %s\n", e.what());
        co_return -1;
    }
}

my_task stopped_demo() {
    try {
        int v = co_await ex::just_stopped();
        (void)v;
        co_return 0;
    } catch (const std::exception& e) {
        std::printf("[stopped_demo] caught: %s\n", e.what());
        co_return -2;
    }
}

int main()
{
    std::printf("===== H-1: as_awaitable bridge =====\n\n");

    {
        std::printf("--- 测试 1：value channel ---\n");
        int r = value_demo().run();
        std::printf("  result = %d (expect 52)\n", r);
    }
    {
        std::printf("\n--- 测试 2：error channel ---\n");
        int r = error_demo().run();
        std::printf("  result = %d (expect -1)\n", r);
    }
    {
        std::printf("\n--- 测试 3：stopped channel ---\n");
        int r = stopped_demo().run();
        std::printf("  result = %d (expect -2)\n", r);
    }

    // TODO [必做]：在笔记中画出 8 步桥接调用链：
    //   co_await sender → await_transform → as_awaitable → sender_awaitable
    //   → await_suspend(connect+start) → 挂起 → set_value/error/stopped
    //   → resume → await_resume → 协程体继续
    // TODO [进阶]：从 completion_signatures 推导 value_type，处理多值。
    // TODO [进阶]：为 bridge_receiver::get_env 转发 promise 的 env。
    // TODO [进阶]：处理 sender 同步完成的 symmetric transfer 优化。

    std::printf("\n===== Done =====\n");
    return 0;
}
