// Small teaching runtime helpers for the coroutine exercises.
#ifndef COROUTINE_STUDY_RUNTIME_HPP
#define COROUTINE_STUDY_RUNTIME_HPP

#include "coroutine_study/lazy_task.hpp"

#include <atomic>
#include <condition_variable>
#include <coroutine>
#include <deque>
#include <exception>
#include <mutex>
#include <optional>
#include <stop_token>
#include <thread>
#include <utility>
#include <vector>

namespace coroutine_study {

struct manual_coroutine {
    struct promise_type {
        manual_coroutine get_return_object() noexcept {
            return manual_coroutine{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() { throw; }
    };

    explicit manual_coroutine(std::coroutine_handle<promise_type> h) noexcept : h_(h) {}
    manual_coroutine(manual_coroutine&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    manual_coroutine(const manual_coroutine&) = delete;
    ~manual_coroutine() { if (h_) h_.destroy(); }

    bool step() {
        if (!h_ || h_.done()) return false;
        h_.resume();
        return !h_.done();
    }

private:
    std::coroutine_handle<promise_type> h_{};
};

class run_loop {
public:
    struct schedule_awaiter {
        run_loop* loop{};

        bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<> h) const { loop->push(h); }
        void await_resume() const noexcept {}
    };

    schedule_awaiter schedule() noexcept { return schedule_awaiter{this}; }

    void run() {
        for (;;) {
            std::coroutine_handle<> h;
            {
                std::unique_lock lock(mutex_);
                cv_.wait(lock, [&] { return stopped_ || !queue_.empty(); });
                if (queue_.empty()) return;
                h = queue_.front();
                queue_.pop_front();
            }
            if (h && !h.done()) h.resume();
        }
    }

    bool run_one() {
        std::coroutine_handle<> h;
        {
            std::lock_guard lock(mutex_);
            if (queue_.empty()) return false;
            h = queue_.front();
            queue_.pop_front();
        }
        if (h && !h.done()) h.resume();
        return true;
    }

    void finish() {
        {
            std::lock_guard lock(mutex_);
            stopped_ = true;
        }
        cv_.notify_all();
    }

private:
    void push(std::coroutine_handle<> h) {
        {
            std::lock_guard lock(mutex_);
            queue_.push_back(h);
        }
        cv_.notify_one();
    }

    std::mutex mutex_;
    std::condition_variable cv_;
    std::deque<std::coroutine_handle<>> queue_;
    bool stopped_ = false;
};

class task_scope {
public:
    task_scope() = default;
    task_scope(const task_scope&) = delete;
    task_scope& operator=(const task_scope&) = delete;
    ~task_scope() noexcept {
        request_stop();
        join_no_throw();
    }

    std::stop_token get_token() const noexcept { return stop_source_.get_token(); }
    void request_stop() noexcept { stop_source_.request_stop(); }

    void spawn(lazy_task<void> task) {
        threads_.emplace_back([this, task = std::move(task)]() mutable {
            try {
                sync_wait(std::move(task));
            } catch (...) {
                {
                    std::lock_guard lock(mutex_);
                    if (!exception_) exception_ = std::current_exception();
                }
                request_stop();
            }
        });
    }

    void join() {
        join_no_throw();
        if (exception_) std::rethrow_exception(exception_);
    }

private:
    void join_no_throw() noexcept {
        for (auto& t : threads_) {
            if (t.joinable()) t.join();
        }
        threads_.clear();
    }

    std::mutex mutex_;
    std::stop_source stop_source_;
    std::exception_ptr exception_;
    std::vector<std::jthread> threads_;
};

template <class T>
struct when_any_result {
    std::size_t index{};
    T value{};
};

template <class T>
when_any_result<T> when_any_cancel_join(
    std::stop_source& stop_source,
    lazy_task<T> first,
    lazy_task<T> second
) {
    struct shared_state {
        std::mutex mutex;
        std::condition_variable cv;
        std::optional<when_any_result<T>> result;
        std::exception_ptr exception;
    } state;

    auto run_one = [&](std::size_t index, lazy_task<T> task) mutable {
        try {
            T value = sync_wait(std::move(task));
            {
                std::lock_guard lock(state.mutex);
                if (!state.result && !state.exception) {
                    state.result.emplace(when_any_result<T>{index, std::move(value)});
                    stop_source.request_stop();
                }
            }
            state.cv.notify_one();
        } catch (...) {
            {
                std::lock_guard lock(state.mutex);
                if (!state.result && !state.exception) {
                    state.exception = std::current_exception();
                    stop_source.request_stop();
                }
            }
            state.cv.notify_one();
        }
    };

    std::jthread t0(run_one, 0, std::move(first));
    std::jthread t1(run_one, 1, std::move(second));

    std::unique_lock lock(state.mutex);
    state.cv.wait(lock, [&] { return state.result || state.exception; });
    if (state.exception) std::rethrow_exception(state.exception);
    return std::move(*state.result);
}

} // namespace coroutine_study

#endif // COROUTINE_STUDY_RUNTIME_HPP
