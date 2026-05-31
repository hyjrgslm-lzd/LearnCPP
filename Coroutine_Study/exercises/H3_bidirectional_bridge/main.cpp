// H-3 双向桥接——my_task 同时是 sender 和 awaitable
// 文档参考：10-模块H-协程与sender_receiver桥接.md 「练习 H-3」
// 官方参考：
//   - P2300R10 sender 协议 + completion_signatures
//   - P3175R0 as_awaitable / task-sender 桥接
//   - P3552R3 std::execution::task<T>
//   - stdexec/exec/task.hpp / __connect_awaitable.hpp
//   - Execution_Study 模块 H-3（sender 侧对等练习）
//
// 目标：让自定义协程 task 既能被 stdexec sync_wait 消费（实现 sender 协议），
//      又能 co_await 任意 stdexec sender（通过 await_transform 桥接）。
//      task 同时是 awaitable 和 sender——双重身份的关键设计点：
//      final_suspend 中同时处理 external_receiver_（sender 侧）和 continuation（协程侧）。

#include <stdexec/execution.hpp>

#include <coroutine>
#include <cstdio>
#include <exception>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

namespace ex = stdexec;

// ============ 桥接 H-1 复用：as_awaitable 简化版 ============
struct stopped_tag {};

template <typename Promise>
struct h1_storage {
    using value_type = int;
    std::variant<std::monostate, int, std::exception_ptr, stopped_tag> v_;
};

template <typename Promise>
struct h1_bridge_receiver {
    using receiver_concept = ex::receiver_t;
    std::coroutine_handle<Promise> coro_;
    h1_storage<Promise>* storage_;

    template <typename V>
    friend void tag_invoke(ex::set_value_t, h1_bridge_receiver&& self, V&& v) noexcept {
        self.storage_->v_.template emplace<int>(static_cast<int>(std::forward<V>(v)));
        self.coro_.resume();
    }
    template <typename E>
    friend void tag_invoke(ex::set_error_t, h1_bridge_receiver&& self, E&& e) noexcept {
        if constexpr (std::is_same_v<std::decay_t<E>, std::exception_ptr>) {
            self.storage_->v_.template emplace<std::exception_ptr>(std::forward<E>(e));
        } else {
            try { throw std::forward<E>(e); }
            catch (...) {
                self.storage_->v_.template emplace<std::exception_ptr>(std::current_exception());
            }
        }
        self.coro_.resume();
    }
    friend void tag_invoke(ex::set_stopped_t, h1_bridge_receiver&& self) noexcept {
        self.storage_->v_.template emplace<stopped_tag>(stopped_tag{});
        self.coro_.resume();
    }
    friend auto tag_invoke(ex::get_env_t, const h1_bridge_receiver&) noexcept {
        return ex::empty_env{};
    }
};

template <typename Sender, typename Promise>
struct h1_sender_awaitable {
    Sender sender_;
    std::coroutine_handle<Promise> coro_;
    h1_storage<Promise> storage_{};

    using receiver_t = h1_bridge_receiver<Promise>;
    using op_state_t = ex::connect_result_t<Sender, receiver_t>;
    std::optional<op_state_t> op_state_;

    bool await_ready() noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) noexcept {
        // h 实际指向派生 promise（如 my_task<T>::promise_type），
        // coroutine_handle 不支持 derived-to-base 隐式转换，从地址重建 base 句柄。
        coro_ = std::coroutine_handle<Promise>::from_address(h.address());
        op_state_.emplace(ex::connect(std::move(sender_), receiver_t{coro_, &storage_}));
        ex::start(*op_state_);
    }
    int await_resume() {
        if (auto* p = std::get_if<int>(&storage_.v_)) return *p;
        if (auto* p = std::get_if<std::exception_ptr>(&storage_.v_))
            std::rethrow_exception(*p);
        if (std::holds_alternative<stopped_tag>(storage_.v_))
            throw std::runtime_error("sender stopped");
        throw std::logic_error("no completion");
    }
};

// ============ my_task<T>：双重身份 ============
//
// 关键设计：promise 同时维护
//   - continuation：协程链上的等待者（co_await my_task 的协程）
//   - external_receiver_：sender 链下游 receiver（sync_wait/when_all 的 receiver）
//
// 当 task 被 sender 路径消费（start 调用）时，continuation 设为 noop_coroutine；
// 当 task 被协程 co_await 时，continuation 指向等待者。
// final_suspend 中先通知 external_receiver_，再 symmetric transfer 到 continuation。
template <typename T>
struct my_task;

namespace detail {

// 类型擦除的 receiver 句柄——final_suspend 不必知道下游 receiver 的具体类型
template <typename T>
struct erased_receiver {
    void* receiver_ptr = nullptr;
    void (*set_value_fn)(void*, T) noexcept = nullptr;
    void (*set_error_fn)(void*, std::exception_ptr) noexcept = nullptr;
    void (*set_stopped_fn)(void*) noexcept = nullptr;

    bool valid() const noexcept { return receiver_ptr != nullptr; }
};

template <typename T, typename Receiver>
erased_receiver<T> make_erased(Receiver* r) noexcept {
    return {
        r,
        [](void* p, T v) noexcept {
            ex::set_value(std::move(*static_cast<Receiver*>(p)), std::move(v));
        },
        [](void* p, std::exception_ptr ep) noexcept {
            ex::set_error(std::move(*static_cast<Receiver*>(p)), std::move(ep));
        },
        [](void* p) noexcept {
            ex::set_stopped(std::move(*static_cast<Receiver*>(p)));
        }
    };
}

template <typename T>
struct task_promise_base {
    T result_value{};
    std::exception_ptr result_exception;
    std::coroutine_handle<> continuation = std::noop_coroutine();
    erased_receiver<T> external_receiver{};

    std::suspend_always initial_suspend() noexcept { return {}; }

    auto final_suspend() noexcept {
        struct final_awaiter {
            bool await_ready() noexcept { return false; }
            std::coroutine_handle<> await_suspend(
                std::coroutine_handle<> h) noexcept
            {
                // 派生 promise（如 my_task<T>::promise_type）→ 基类手动转换：
                // coroutine_handle 不支持 derived-to-base 隐式转换，从地址重建 base 句柄。
                auto base = std::coroutine_handle<task_promise_base>::from_address(h.address());
                auto& p = base.promise();
                // 1) sender 侧：通知下游 receiver
                if (p.external_receiver.valid()) {
                    if (p.result_exception) {
                        p.external_receiver.set_error_fn(
                            p.external_receiver.receiver_ptr,
                            p.result_exception);
                    } else {
                        p.external_receiver.set_value_fn(
                            p.external_receiver.receiver_ptr,
                            std::move(p.result_value));
                    }
                }
                // 2) 协程侧：symmetric transfer 到等待者
                return p.continuation;
            }
            void await_resume() noexcept {}
        };
        return final_awaiter{};
    }

    void return_value(T v) noexcept { result_value = std::move(v); }
    void unhandled_exception() noexcept { result_exception = std::current_exception(); }

    // 协程内 co_await sender → as_awaitable 桥接（复用 H-1）
    template <typename Sender>
        requires ex::sender<Sender>
    auto await_transform(Sender&& s) {
        return h1_sender_awaitable<std::remove_cvref_t<Sender>, task_promise_base>{
            std::forward<Sender>(s),
            std::coroutine_handle<task_promise_base>::from_promise(*this)
        };
    }

    // 协程内 co_await my_task<U>：直接走 awaitable 协议
    // 左值禁用：避免 co_await 持有外部 my_task 引用、外部先析构导致悬空
    template <typename U>
    my_task<U> await_transform(my_task<U>& t)  noexcept = delete;
    template <typename U>
    my_task<U> await_transform(my_task<U>&& t) noexcept { return std::move(t); }
};

} // namespace detail

template <typename T>
struct my_task {
    // ===== sender 协议 =====
    using sender_concept = ex::sender_t;
    using completion_signatures = ex::completion_signatures<
        ex::set_value_t(T),
        ex::set_error_t(std::exception_ptr)
    >;

    struct promise_type : detail::task_promise_base<T> {
        my_task get_return_object() noexcept {
            return my_task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
    };

    std::coroutine_handle<promise_type> h_;

    explicit my_task(std::coroutine_handle<promise_type> h) noexcept : h_(h) {}
    my_task(my_task&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    my_task(const my_task&) = delete;
    ~my_task() { if (h_) h_.destroy(); }

    // ===== awaitable 协议（让其它协程 co_await my_task）=====
    bool await_ready() noexcept { return h_.done(); }
    auto await_suspend(std::coroutine_handle<> caller) noexcept {
        h_.promise().continuation = caller;
        return h_;   // symmetric transfer 启动 task
    }
    T await_resume() {
        auto& p = h_.promise();
        if (p.result_exception) std::rethrow_exception(p.result_exception);
        return std::move(p.result_value);
    }

    // ===== sender::connect =====
    template <typename Receiver>
    struct op_state {
        using operation_state_concept = ex::operation_state_t;

        std::coroutine_handle<promise_type> coro;
        Receiver rcvr;

        friend void tag_invoke(ex::start_t, op_state& self) noexcept {
            // 注入 erased_receiver；continuation 默认 noop（已在 promise 构造时设好）
            auto& p = self.coro.promise();
            p.external_receiver = detail::make_erased<T, Receiver>(&self.rcvr);
            p.continuation = std::noop_coroutine();
            self.coro.resume();
        }
    };

    template <typename Receiver>
    friend auto tag_invoke(ex::connect_t, my_task self, Receiver rcvr) {
        auto h = std::exchange(self.h_, {});
        return op_state<std::remove_cvref_t<Receiver>>{h, std::move(rcvr)};
    }

    // 让 sync_wait 等组合子能查询 env
    friend auto tag_invoke(ex::get_env_t, const my_task&) noexcept {
        return ex::empty_env{};
    }
};

// ============ 测试场景 ============
my_task<int> sender_side() {
    co_return 42;
}

my_task<int> inner() {
    co_return 10;
}
my_task<int> outer() {
    int v = co_await inner();
    co_return v * 2;
}

my_task<int> bridge_side() {
    int v = co_await ex::just(42);
    co_return v + 1;
}

int main()
{
    std::printf("===== H-3: 双向桥接 =====\n\n");

    {
        std::printf("--- 场景 A：my_task 被 sync_wait 消费（sender 侧）---\n");
        auto r = ex::sync_wait(sender_side());
        if (r) {
            auto [v] = *r;
            std::printf("  result = %d (expect 42)\n", v);
        }
    }

    {
        std::printf("\n--- 场景 B：my_task 被 co_await 消费（awaitable 侧）---\n");
        auto r = ex::sync_wait(outer());
        if (r) {
            auto [v] = *r;
            std::printf("  result = %d (expect 20)\n", v);
        }
    }

    {
        std::printf("\n--- 场景 C：my_task 内 co_await stdexec sender（桥接侧）---\n");
        auto r = ex::sync_wait(bridge_side());
        if (r) {
            auto [v] = *r;
            std::printf("  result = %d (expect 43)\n", v);
        }
    }

    // TODO [必做]：在笔记中画双向桥接对象图——sync_wait 路径 vs co_await 路径
    //             各自走过的对象 + 时序。
    // TODO [必做]：解释为什么 final_suspend 必须先通知 external_receiver
    //             再 symmetric transfer 到 continuation。
    // TODO [进阶]：让 my_task<void> 也能编译（特化 task_promise_base）。
    // TODO [进阶]：让 my_task 参与 then/when_all 等组合链。

    std::printf("\n===== Done =====\n");
    return 0;
}
