// =====================================================================
// Teaching lazy_task<T>.
//
// A lazy_task owns one coroutine frame and can be consumed once. sync_wait
// starts the root coroutine once, then waits for final_suspend to signal
// completion; it does not blindly resume through arbitrary suspension.
// =====================================================================
#ifndef COROUTINE_STUDY_LAZY_TASK_HPP
#define COROUTINE_STUDY_LAZY_TASK_HPP

#include <concepts>
#include <condition_variable>
#include <coroutine>
#include <exception>
#include <mutex>
#include <new>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace coroutine_study {

inline thread_local bool lazy_task_fail_next_allocation = false;
inline thread_local std::size_t lazy_task_last_allocation_size = 0;

template <typename T>
struct lazy_task;

template <class T>
T sync_wait(lazy_task<T>&& task);
void sync_wait(lazy_task<void>&& task);

namespace detail {

struct sync_wait_state {
    std::mutex mutex;
    std::condition_variable cv;
    bool done = false;
};

inline void notify_sync_wait(void* p) noexcept {
    auto& state = *static_cast<sync_wait_state*>(p);
    std::lock_guard lock(state.mutex);
    state.done = true;
    // A spurious wake must not destroy this stack state before notify finishes.
    state.cv.notify_one();
}

struct lazy_promise_base {
    std::coroutine_handle<> continuation{};
    void* completion_state{};
    void (*notify_completion)(void*) noexcept {};
    bool started = false;
    std::exception_ptr exception;

    static void* operator new(std::size_t size) noexcept {
        lazy_task_last_allocation_size = size;
        if (std::exchange(lazy_task_fail_next_allocation, false)) return nullptr;
        return ::operator new(size, std::nothrow);
    }

    static void operator delete(void* p, std::size_t) noexcept {
        ::operator delete(p);
    }

    std::suspend_always initial_suspend() noexcept { return {}; }

    struct final_awaiter {
        bool await_ready() noexcept { return false; }

        template <class Promise>
        std::coroutine_handle<> await_suspend(std::coroutine_handle<Promise> h) noexcept {
            auto& p = h.promise();
            if (p.continuation) return p.continuation;
            if (p.notify_completion) p.notify_completion(p.completion_state);
            return std::noop_coroutine();
        }

        void await_resume() noexcept {}
    };

    final_awaiter final_suspend() noexcept { return {}; }
    void unhandled_exception() noexcept { exception = std::current_exception(); }
};

} // namespace detail

template <typename T>
struct lazy_task {
    struct promise_type : detail::lazy_promise_base {
        std::optional<T> result;

        static lazy_task get_return_object_on_allocation_failure() noexcept {
            return lazy_task{};
        }

        lazy_task get_return_object() noexcept {
            return lazy_task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        template <class U>
        requires std::constructible_from<T, U>
        void return_value(U&& v) {
            result.emplace(std::forward<U>(v));
        }
    };

    using handle_t = std::coroutine_handle<promise_type>;

    lazy_task() = default;
    explicit lazy_task(handle_t h) noexcept : h_(h) {}
    lazy_task(lazy_task&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    lazy_task& operator=(lazy_task&& o) noexcept {
        if (this != &o) {
            reset();
            h_ = std::exchange(o.h_, {});
        }
        return *this;
    }
    lazy_task(const lazy_task&) = delete;
    lazy_task& operator=(const lazy_task&) = delete;
    ~lazy_task() { reset(); }

    bool valid() const noexcept { return static_cast<bool>(h_); }
    bool done() const noexcept { return !h_ || h_.done(); }

    void start() {
        if (!h_) throw std::bad_alloc{};
        if (std::exchange(h_.promise().started, true)) {
            throw std::logic_error("lazy_task can only be started once");
        }
        if (!h_.done()) h_.resume();
    }

    T sync_wait() && { return coroutine_study::sync_wait(std::move(*this)); }
    T get() && { return coroutine_study::sync_wait(std::move(*this)); }
    T sync_wait() & = delete;
    T get() & = delete;

    struct awaiter {
        handle_t callee{};

        awaiter() = default;
        explicit awaiter(handle_t h) noexcept : callee(h) {}
        awaiter(awaiter&& o) noexcept : callee(std::exchange(o.callee, {})) {}
        awaiter(const awaiter&) = delete;
        awaiter& operator=(const awaiter&) = delete;
        ~awaiter() { if (callee) callee.destroy(); }

        bool await_ready() const noexcept { return !callee; }

        std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) {
            auto& p = callee.promise();
            if (std::exchange(p.started, true)) {
                throw std::logic_error("lazy_task can only be awaited before start");
            }
            p.continuation = caller;
            return callee;
        }

        T await_resume() {
            lazy_task owned{std::exchange(callee, {})};
            if (!owned.h_) throw std::bad_alloc{};
            auto& p = owned.h_.promise();
            if (p.exception) std::rethrow_exception(p.exception);
            if (!p.result) throw std::logic_error("lazy_task completed without a value");
            return std::move(*p.result);
        }
    };

    awaiter operator co_await() && noexcept {
        return awaiter{std::exchange(h_, {})};
    }
    awaiter operator co_await() & = delete;
    awaiter operator co_await() const& = delete;

private:
    template <class U>
    friend U sync_wait(lazy_task<U>&&);
    friend void sync_wait(lazy_task<void>&&);

    void reset() noexcept {
        if (h_) std::exchange(h_, {}).destroy();
    }

    handle_t h_{};
};

template <>
struct lazy_task<void> {
    struct promise_type : detail::lazy_promise_base {
        static lazy_task get_return_object_on_allocation_failure() noexcept {
            return lazy_task{};
        }

        lazy_task get_return_object() noexcept {
            return lazy_task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        void return_void() noexcept {}
    };

    using handle_t = std::coroutine_handle<promise_type>;

    lazy_task() = default;
    explicit lazy_task(handle_t h) noexcept : h_(h) {}
    lazy_task(lazy_task&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    lazy_task& operator=(lazy_task&& o) noexcept {
        if (this != &o) {
            reset();
            h_ = std::exchange(o.h_, {});
        }
        return *this;
    }
    lazy_task(const lazy_task&) = delete;
    lazy_task& operator=(const lazy_task&) = delete;
    ~lazy_task() { reset(); }

    bool valid() const noexcept { return static_cast<bool>(h_); }
    bool done() const noexcept { return !h_ || h_.done(); }

    void start() {
        if (!h_) throw std::bad_alloc{};
        if (std::exchange(h_.promise().started, true)) {
            throw std::logic_error("lazy_task can only be started once");
        }
        if (!h_.done()) h_.resume();
    }

    void sync_wait() && { coroutine_study::sync_wait(std::move(*this)); }
    void get() && { coroutine_study::sync_wait(std::move(*this)); }
    void sync_wait() & = delete;
    void get() & = delete;

    struct awaiter {
        handle_t callee{};

        awaiter() = default;
        explicit awaiter(handle_t h) noexcept : callee(h) {}
        awaiter(awaiter&& o) noexcept : callee(std::exchange(o.callee, {})) {}
        awaiter(const awaiter&) = delete;
        awaiter& operator=(const awaiter&) = delete;
        ~awaiter() { if (callee) callee.destroy(); }

        bool await_ready() const noexcept { return !callee; }

        std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) {
            auto& p = callee.promise();
            if (std::exchange(p.started, true)) {
                throw std::logic_error("lazy_task can only be awaited before start");
            }
            p.continuation = caller;
            return callee;
        }

        void await_resume() {
            auto h = std::exchange(callee, {});
            if (!h) throw std::bad_alloc{};
            auto& p = h.promise();
            if (p.exception) {
                auto ex = p.exception;
                h.destroy();
                std::rethrow_exception(ex);
            }
            h.destroy();
        }
    };

    awaiter operator co_await() && noexcept {
        return awaiter{std::exchange(h_, {})};
    }
    awaiter operator co_await() & = delete;
    awaiter operator co_await() const& = delete;

private:
    template <class U>
    friend U sync_wait(lazy_task<U>&&);
    friend void sync_wait(lazy_task<void>&&);

    void reset() noexcept {
        if (h_) std::exchange(h_, {}).destroy();
    }

    handle_t h_{};
};

template <class T>
T sync_wait(lazy_task<T>&& task) {
    lazy_task<T> owned = std::move(task);
    if (!owned.h_) throw std::bad_alloc{};

    detail::sync_wait_state state;
    auto& p = owned.h_.promise();
    if (p.started) throw std::logic_error("sync_wait requires an unstarted lazy_task");
    p.completion_state = &state;
    p.notify_completion = detail::notify_sync_wait;
    owned.start();

    std::unique_lock lock(state.mutex);
    state.cv.wait(lock, [&] { return state.done; });
    lock.unlock();

    if (p.exception) std::rethrow_exception(p.exception);
    if (!p.result) throw std::logic_error("lazy_task completed without a value");
    return std::move(*p.result);
}

inline void sync_wait(lazy_task<void>&& task) {
    lazy_task<void> owned = std::move(task);
    if (!owned.h_) throw std::bad_alloc{};

    detail::sync_wait_state state;
    auto& p = owned.h_.promise();
    if (p.started) throw std::logic_error("sync_wait requires an unstarted lazy_task");
    p.completion_state = &state;
    p.notify_completion = detail::notify_sync_wait;
    owned.start();

    std::unique_lock lock(state.mutex);
    state.cv.wait(lock, [&] { return state.done; });
    lock.unlock();

    if (p.exception) std::rethrow_exception(p.exception);
}

} // namespace coroutine_study

#endif // COROUTINE_STUDY_LAZY_TASK_HPP
