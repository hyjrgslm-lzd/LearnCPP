// =====================================================================
// 最小 30 行 lazy_task<T>。模块 A-2 起共用。
//
// 设计要点：
//   - initial_suspend = suspend_always —— 创建时不执行（lazy 语义）
//   - final_suspend   = suspend_always —— 帧不会自动销毁，由 sync_wait 显式 destroy
//   - return_value 用模板版本支持 move-only T 与完美转发
//   - sync_wait 循环 resume 直到 done()，能正确处理协程体内部多次挂起点
//   - operator co_await 让 lazy_task 自身可被另一个协程 co_await（B-2 顺序组合用）
//
// 不依赖任何第三方库，仅标准库。
// =====================================================================
#ifndef COROUTINE_STUDY_LAZY_TASK_HPP
#define COROUTINE_STUDY_LAZY_TASK_HPP

#include <coroutine>
#include <exception>
#include <optional>
#include <utility>
#include <cassert>

namespace coroutine_study {

template <typename T>
struct lazy_task {
    struct promise_type {
        std::optional<T>      result_;
        std::exception_ptr    exception_;
        std::coroutine_handle<> continuation_{};   // co_await 链上"在等我的人"

        lazy_task get_return_object() {
            return lazy_task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() noexcept { return {}; }

        // final_suspend 走 symmetric transfer：如果有 caller 在等我，就把控制权交给它
        struct final_awaiter {
            bool await_ready() noexcept { return false; }
            std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> h) noexcept {
                auto cont = h.promise().continuation_;
                return cont ? cont : std::noop_coroutine();
            }
            void await_resume() noexcept {}
        };
        final_awaiter final_suspend() noexcept { return {}; }

        // 模板版本支持 move-only T 与完美转发
        template <class U = T>
        void return_value(U&& v) { result_.emplace(std::forward<U>(v)); }

        void unhandled_exception() noexcept { exception_ = std::current_exception(); }
    };

    using handle_t = std::coroutine_handle<promise_type>;
    handle_t h_{};

    explicit lazy_task(handle_t h) noexcept : h_(h) {}
    lazy_task(lazy_task&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    lazy_task& operator=(lazy_task&& o) noexcept {
        if (this != &o) {
            if (h_) h_.destroy();
            h_ = std::exchange(o.h_, {});
        }
        return *this;
    }
    lazy_task(const lazy_task&)            = delete;
    lazy_task& operator=(const lazy_task&) = delete;
    ~lazy_task() { if (h_) h_.destroy(); }

    // 同步等待完成并取走值（教学用，不涉及调度器）
    T sync_wait() {
        assert(h_ && "sync_wait called on empty lazy_task");
        while (!h_.done()) h_.resume();
        auto& p = h_.promise();
        if (p.exception_) std::rethrow_exception(p.exception_);
        assert(p.result_.has_value());
        return std::move(*p.result_);
    }
    T get() { return sync_wait(); }

    // 让 lazy_task 可以被另一个 lazy_task co_await
    struct awaiter {
        handle_t callee_;
        bool await_ready() noexcept { return !callee_ || callee_.done(); }
        std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) noexcept {
            callee_.promise().continuation_ = caller;
            return callee_;   // symmetric transfer：直接跳到 callee 开始执行
        }
        T await_resume() {
            auto& p = callee_.promise();
            if (p.exception_) std::rethrow_exception(p.exception_);
            return std::move(*p.result_);
        }
    };
    awaiter operator co_await() &  noexcept { return awaiter{h_}; }
    awaiter operator co_await() && noexcept { return awaiter{h_}; }
};

// ----------------------------------------------------------------
// lazy_task<void> 偏特化（Capstone1 / C3 spawn / detach 用）
// ----------------------------------------------------------------
template <>
struct lazy_task<void> {
    struct promise_type {
        std::exception_ptr      exception_;
        std::coroutine_handle<> continuation_{};

        lazy_task get_return_object() {
            return lazy_task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() noexcept { return {}; }

        struct final_awaiter {
            bool await_ready() noexcept { return false; }
            std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> h) noexcept {
                auto cont = h.promise().continuation_;
                return cont ? cont : std::noop_coroutine();
            }
            void await_resume() noexcept {}
        };
        final_awaiter final_suspend() noexcept { return {}; }

        void return_void() noexcept {}
        void unhandled_exception() noexcept { exception_ = std::current_exception(); }
    };

    using handle_t = std::coroutine_handle<promise_type>;
    handle_t h_{};

    lazy_task() = default;
    explicit lazy_task(handle_t h) noexcept : h_(h) {}
    lazy_task(lazy_task&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    lazy_task& operator=(lazy_task&& o) noexcept {
        if (this != &o) { if (h_) h_.destroy(); h_ = std::exchange(o.h_, {}); }
        return *this;
    }
    lazy_task(const lazy_task&) = delete;
    lazy_task& operator=(const lazy_task&) = delete;
    ~lazy_task() { if (h_) h_.destroy(); }

    void sync_wait() {
        assert(h_ && "sync_wait called on empty lazy_task<void>");
        while (!h_.done()) h_.resume();
        if (h_.promise().exception_) std::rethrow_exception(h_.promise().exception_);
    }
    void get() { sync_wait(); }

    struct awaiter {
        handle_t callee_;
        bool await_ready() noexcept { return !callee_ || callee_.done(); }
        std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) noexcept {
            callee_.promise().continuation_ = caller;
            return callee_;
        }
        void await_resume() {
            if (callee_.promise().exception_) std::rethrow_exception(callee_.promise().exception_);
        }
    };
    awaiter operator co_await() &  noexcept { return awaiter{h_}; }
    awaiter operator co_await() && noexcept { return awaiter{h_}; }
};

}  // namespace coroutine_study

#endif  // COROUTINE_STUDY_LAZY_TASK_HPP
