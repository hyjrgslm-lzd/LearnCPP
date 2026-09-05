#pragma once

#include <condition_variable>
#include <coroutine>
#include <atomic>
#include <deque>
#include <exception>
#include <memory>
#include <mutex>
#include <optional>
#include <stop_token>
#include <stdexcept>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace mini_ref {

template <typename... Ts>
struct completion_signatures {
    using value_types = std::tuple<Ts...>;
};

template <typename T = void>
class task;

template <typename T>
struct task_promise_base {
    std::coroutine_handle<> continuation;
    void* completion_state{};
    std::coroutine_handle<> (*on_completed)(void*) noexcept = nullptr;
    bool started{};
    bool consumed{};
    std::exception_ptr error;

    std::suspend_always initial_suspend() noexcept { return {}; }
    void unhandled_exception() noexcept { error = std::current_exception(); }

    struct final_awaiter {
        bool await_ready() const noexcept { return false; }
        template <typename Promise>
        std::coroutine_handle<> await_suspend(std::coroutine_handle<Promise> h) const noexcept {
            auto& p = h.promise();
            // Completion may let another thread destroy this suspended frame.
            // Read everything needed before publishing completion.
            const auto callback = p.on_completed;
            void* state = p.completion_state;
            const auto continuation = p.continuation;
            if (callback) return callback(state);
            return continuation ? continuation : std::noop_coroutine();
        }
        void await_resume() const noexcept {}
    };

    final_awaiter final_suspend() noexcept { return {}; }
};

template <typename T>
class task {
public:
    struct promise_type : task_promise_base<T> {
        std::optional<T> value;
        task get_return_object() noexcept {
            return task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        template <typename U>
        void return_value(U&& v) { value.emplace(std::forward<U>(v)); }
    };

    using value_type = T;
    using completion_signatures = mini_ref::completion_signatures<T>;

    explicit task(std::coroutine_handle<promise_type> h = {}) noexcept : h_(h) {}
    task(task&& other) noexcept : h_(std::exchange(other.h_, {})) {}
    task& operator=(task&& other) noexcept {
        if (this != &other) {
            if (h_) h_.destroy();
            h_ = std::exchange(other.h_, {});
        }
        return *this;
    }
    task(const task&) = delete;
    task& operator=(const task&) = delete;
    ~task() { if (h_) h_.destroy(); }

    bool await_ready() const noexcept { return !h_ || h_.done(); }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) {
        if (std::exchange(h_.promise().started, true)) throw std::logic_error{"task already started"};
        h_.promise().continuation = caller;
        return h_;
    }
    T await_resume() {
        if (!h_ || !h_.done() || std::exchange(h_.promise().consumed, true)) {
            throw std::logic_error{"task result can only be consumed once after completion"};
        }
        if (h_.promise().error) std::rethrow_exception(h_.promise().error);
        return std::move(*h_.promise().value);
    }
    void start() {
        if (!h_) return;
        if (std::exchange(h_.promise().started, true)) throw std::logic_error{"task already started"};
        h_.resume();
    }
    void set_continuation(std::coroutine_handle<> continuation) noexcept {
        h_.promise().continuation = continuation;
    }

private:
    std::coroutine_handle<promise_type> h_;

    template <typename U>
    friend std::optional<std::tuple<U>> sync_wait(task<U>);
    friend std::optional<std::tuple<>> sync_wait(task<void>);
};

template <>
class task<void> {
public:
    struct promise_type : task_promise_base<void> {
        task get_return_object() noexcept {
            return task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        void return_void() noexcept {}
    };

    using value_type = void;
    using completion_signatures = mini_ref::completion_signatures<>;

    explicit task(std::coroutine_handle<promise_type> h = {}) noexcept : h_(h) {}
    task(task&& other) noexcept : h_(std::exchange(other.h_, {})) {}
    task& operator=(task&& other) noexcept {
        if (this != &other) {
            if (h_) h_.destroy();
            h_ = std::exchange(other.h_, {});
        }
        return *this;
    }
    task(const task&) = delete;
    task& operator=(const task&) = delete;
    ~task() { if (h_) h_.destroy(); }

    bool await_ready() const noexcept { return !h_ || h_.done(); }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) {
        if (std::exchange(h_.promise().started, true)) throw std::logic_error{"task already started"};
        h_.promise().continuation = caller;
        return h_;
    }
    void await_resume() {
        if (!h_ || !h_.done() || std::exchange(h_.promise().consumed, true)) {
            throw std::logic_error{"task result can only be consumed once after completion"};
        }
        if (h_.promise().error) std::rethrow_exception(h_.promise().error);
    }
    void start() {
        if (!h_) return;
        if (std::exchange(h_.promise().started, true)) throw std::logic_error{"task already started"};
        h_.resume();
    }
    void set_continuation(std::coroutine_handle<> continuation) noexcept {
        h_.promise().continuation = continuation;
    }

private:
    std::coroutine_handle<promise_type> h_;

    template <typename, typename>
    friend class when_all_op;
    template <typename, typename>
    friend class when_any_op;
    template <typename U>
    friend std::optional<std::tuple<U>> sync_wait(task<U>);
    friend std::optional<std::tuple<>> sync_wait(task<void>);
};

namespace detail {

struct sync_wait_state {
    std::mutex mutex;
    std::condition_variable cv;
    bool done{};

    static std::coroutine_handle<> complete(void* context) noexcept {
        auto& state = *static_cast<sync_wait_state*>(context);
        std::lock_guard lock{state.mutex};
        state.done = true;
        // Keep the state alive through notify_one: the waiter needs this lock.
        state.cv.notify_one();
        return std::noop_coroutine();
    }

    void wait() {
        std::unique_lock lock{mutex};
        cv.wait(lock, [&] { return done; });
    }
};

} // namespace detail

template <typename T>
std::optional<std::tuple<T>> sync_wait(task<T> t) {
    if (!t.h_ || t.h_.promise().started) throw std::logic_error{"sync_wait requires an unstarted task"};
    detail::sync_wait_state state;
    auto& promise = t.h_.promise();
    promise.completion_state = &state;
    promise.on_completed = detail::sync_wait_state::complete;
    t.start();
    state.wait();
    if (promise.error) std::rethrow_exception(promise.error);
    return std::tuple<T>{std::move(*promise.value)};
}

inline std::optional<std::tuple<>> sync_wait(task<void> t) {
    if (!t.h_ || t.h_.promise().started) throw std::logic_error{"sync_wait requires an unstarted task"};
    detail::sync_wait_state state;
    auto& promise = t.h_.promise();
    promise.completion_state = &state;
    promise.on_completed = detail::sync_wait_state::complete;
    t.start();
    state.wait();
    if (promise.error) std::rethrow_exception(promise.error);
    return std::tuple<>{};
}

template <typename T>
class generator {
public:
    struct promise_type {
        std::optional<T> current;
        std::exception_ptr error;
        generator get_return_object() noexcept {
            return generator{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        template <typename U>
        std::suspend_always yield_value(U&& value) {
            current.emplace(std::forward<U>(value));
            return {};
        }
        void return_void() noexcept {}
        void unhandled_exception() noexcept { error = std::current_exception(); }
    };

    class iterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;

        iterator() = default;
        explicit iterator(std::coroutine_handle<promise_type> h) : h_(h) {}
        iterator& operator++() {
            h_.resume();
            if (h_.done()) {
                if (h_.promise().error) std::rethrow_exception(h_.promise().error);
                h_ = {};
            }
            return *this;
        }
        const T& operator*() const noexcept { return *h_.promise().current; }
        bool operator==(std::default_sentinel_t) const noexcept { return !h_; }

    private:
        std::coroutine_handle<promise_type> h_;
    };

    explicit generator(std::coroutine_handle<promise_type> h = {}) : h_(h) {}
    generator(generator&& other) noexcept : h_(std::exchange(other.h_, {})) {}
    generator(const generator&) = delete;
    ~generator() { if (h_) h_.destroy(); }

    iterator begin() {
        if (!h_) return {};
        h_.resume();
        if (h_.done()) {
            if (h_.promise().error) std::rethrow_exception(h_.promise().error);
            return {};
        }
        return iterator{h_};
    }
    std::default_sentinel_t end() noexcept { return {}; }

private:
    std::coroutine_handle<promise_type> h_;
};

template <typename T>
struct shared_state {
    std::mutex mutex;
    bool started{};
    bool done{};
    std::optional<T> value;
    std::exception_ptr error;
    std::vector<std::coroutine_handle<>> waiters;
    std::optional<task<T>> source;
    std::shared_ptr<task<void>> runner;
};

template <typename T>
class shared_task {
public:
    explicit shared_task(task<T> source) : state_(std::make_shared<shared_state<T>>()) {
        state_->source.emplace(std::move(source));
    }

    struct awaiter {
        std::shared_ptr<shared_state<T>> state;

        bool await_ready() const {
            std::lock_guard lock{state->mutex};
            return state->done;
        }
        bool await_suspend(std::coroutine_handle<> caller) {
            bool start = false;
            {
                std::lock_guard lock{state->mutex};
                if (state->done) return false;
                state->waiters.push_back(caller);
                start = !std::exchange(state->started, true);
            }
            if (start) {
                state->runner = std::make_shared<task<void>>(run(std::weak_ptr<shared_state<T>>{state}));
                state->runner->start();
            }
            return true;
        }
        T await_resume() const {
            if (state->error) std::rethrow_exception(state->error);
            return *state->value;
        }

        static task<void> run(std::weak_ptr<shared_state<T>> weak) {
            auto state = weak.lock();
            if (!state) co_return;
            try {
                state->value.emplace(co_await std::move(*state->source));
            } catch (...) {
                state->error = std::current_exception();
            }
            std::vector<std::coroutine_handle<>> waiters;
            {
                std::lock_guard lock{state->mutex};
                state->done = true;
                waiters.swap(state->waiters);
            }
            for (auto h : waiters) h.resume();
        }
    };

    awaiter operator co_await() const { return awaiter{state_}; }

private:
    std::shared_ptr<shared_state<T>> state_;
};

template <typename T>
shared_task<T> share(task<T> source) {
    return shared_task<T>{std::move(source)};
}

template <typename T1, typename T2>
class when_all_op {
public:
    when_all_op(task<T1> a, task<T2> b) : a_(std::move(a)), b_(std::move(b)) {}
    bool await_ready() const noexcept { return false; }
    bool await_suspend(std::coroutine_handle<> parent) {
        parent_ = parent;
        left_ = run_left();
        right_ = run_right();
        left_.h_.promise().completion_state = this;
        left_.h_.promise().on_completed = complete;
        right_.h_.promise().completion_state = this;
        right_.h_.promise().on_completed = complete;
        left_.start();
        right_.start();
        std::lock_guard lock{mutex_};
        waiting_ = remaining_ != 0;
        return waiting_;
    }
    std::tuple<T1, T2> await_resume() {
        std::lock_guard lock{mutex_};
        if (error_) std::rethrow_exception(error_);
        return {std::move(*left_value_), std::move(*right_value_)};
    }

private:
    static std::coroutine_handle<> complete(void* context) noexcept {
        auto& op = *static_cast<when_all_op*>(context);
        std::lock_guard lock{op.mutex_};
        // Called only from a runner's final_suspend, after its locals are gone.
        if (--op.remaining_ == 0 && op.waiting_) return op.parent_;
        return std::noop_coroutine();
    }

    task<void> run_left() {
        try {
            auto value = co_await std::move(a_);
            std::lock_guard lock{mutex_};
            left_value_.emplace(std::move(value));
        } catch (...) {
            std::lock_guard lock{mutex_};
            if (!error_) error_ = std::current_exception();
        }
    }
    task<void> run_right() {
        try {
            auto value = co_await std::move(b_);
            std::lock_guard lock{mutex_};
            right_value_.emplace(std::move(value));
        } catch (...) {
            std::lock_guard lock{mutex_};
            if (!error_) error_ = std::current_exception();
        }
    }

    std::mutex mutex_;
    task<T1> a_;
    task<T2> b_;
    task<void> left_;
    task<void> right_;
    std::optional<T1> left_value_;
    std::optional<T2> right_value_;
    std::exception_ptr error_;
    std::coroutine_handle<> parent_;
    int remaining_{2};
    bool waiting_{};
};

template <typename T1, typename T2>
auto when_all(task<T1> a, task<T2> b) -> task<std::tuple<T1, T2>> {
    co_return co_await when_all_op<T1, T2>{std::move(a), std::move(b)};
}

template <typename... Tasks>
struct when_all_result;

template <typename T1, typename T2>
struct when_all_result<task<T1>, task<T2>> {
    using completion_signatures = mini_ref::completion_signatures<std::tuple<T1, T2>>;
};

template <typename T1, typename T2>
class when_any_op {
public:
    when_any_op(task<T1> a, task<T2> b, std::stop_source stop)
        : a_(std::move(a)), b_(std::move(b)), stop_(std::move(stop)) {}
    bool await_ready() const noexcept { return false; }
    bool await_suspend(std::coroutine_handle<> parent) {
        parent_ = parent;
        left_ = run_left();
        right_ = run_right();
        left_.h_.promise().completion_state = this;
        left_.h_.promise().on_completed = complete;
        right_.h_.promise().completion_state = this;
        right_.h_.promise().on_completed = complete;
        right_.start();
        left_.start();
        std::lock_guard lock{mutex_};
        waiting_ = remaining_ != 0;
        return waiting_;
    }
    std::variant<T1, T2> await_resume() {
        std::lock_guard lock{mutex_};
        if (error_ && !winner_) std::rethrow_exception(error_);
        return std::move(*winner_);
    }

private:
    static std::coroutine_handle<> complete(void* context) noexcept {
        auto& op = *static_cast<when_any_op*>(context);
        std::lock_guard lock{op.mutex_};
        if (--op.remaining_ == 0 && op.waiting_) return op.parent_;
        return std::noop_coroutine();
    }

    task<void> run_left() {
        try {
            auto value = co_await std::move(a_);
            bool won = false;
            {
                std::lock_guard lock{mutex_};
                if (!winner_) {
                    winner_.emplace(std::in_place_index<0>, std::move(value));
                    won = true;
                }
            }
            // Stop callbacks can resume the other branch synchronously.
            if (won) stop_.request_stop();
        } catch (...) {
            std::lock_guard lock{mutex_};
            if (!winner_ && !error_) error_ = std::current_exception();
        }
    }
    task<void> run_right() {
        try {
            auto value = co_await std::move(b_);
            bool won = false;
            {
                std::lock_guard lock{mutex_};
                if (!winner_) {
                    winner_.emplace(std::in_place_index<1>, std::move(value));
                    won = true;
                }
            }
            if (won) stop_.request_stop();
        } catch (...) {
            std::lock_guard lock{mutex_};
            if (!winner_ && !error_) error_ = std::current_exception();
        }
    }

    std::mutex mutex_;
    task<T1> a_;
    task<T2> b_;
    std::stop_source stop_;
    task<void> left_;
    task<void> right_;
    std::optional<std::variant<T1, T2>> winner_;
    std::exception_ptr error_;
    std::coroutine_handle<> parent_;
    int remaining_{2};
    bool waiting_{};
};

template <typename T1, typename T2>
auto when_any(task<T1> a, task<T2> b, std::stop_source stop = {}) -> task<std::variant<T1, T2>> {
    co_return co_await when_any_op<T1, T2>{std::move(a), std::move(b), std::move(stop)};
}

class run_loop {
public:
    auto schedule() {
        struct awaiter {
            run_loop* loop;
            bool await_ready() noexcept { return false; }
            void await_suspend(std::coroutine_handle<> h) {
                std::lock_guard lock{loop->mutex_};
                loop->queue_.push_back(h);
            }
            void await_resume() noexcept {}
        };
        return awaiter{this};
    }

    bool run_one() {
        std::coroutine_handle<> h;
        {
            std::lock_guard lock{mutex_};
            if (queue_.empty()) return false;
            h = queue_.front();
            queue_.pop_front();
        }
        h.resume();
        return true;
    }

    void run() {
        while (run_one()) {}
    }

private:
    std::mutex mutex_;
    std::deque<std::coroutine_handle<>> queue_;
};

using stop_token = std::stop_token;
using stop_source = std::stop_source;

class task_scope {
public:
    task_scope() = default;
    task_scope(const task_scope&) = delete;
    task_scope& operator=(const task_scope&) = delete;

    template <typename T>
    void spawn(task<T> t) {
        {
            std::lock_guard lock{mutex_};
            ++in_flight_;
        }
        threads_.emplace_back([this, task = std::move(t)]() mutable {
            try {
                (void)sync_wait(std::move(task));
            } catch (...) {
                std::lock_guard lock{mutex_};
                if (!error_) error_ = std::current_exception();
            }
            {
                std::lock_guard lock{mutex_};
                --in_flight_;
            }
            cv_.notify_all();
        });
    }

    void wait_empty() {
        {
            std::unique_lock lock{mutex_};
            cv_.wait(lock, [&] { return in_flight_ == 0; });
        }
        for (auto& thread : threads_) {
            if (thread.joinable()) thread.join();
        }
        threads_.clear();
        if (error_) std::rethrow_exception(error_);
    }

    int in_flight() const {
        std::lock_guard lock{mutex_};
        return in_flight_;
    }

    ~task_scope() noexcept {
        try {
            wait_empty();
        } catch (...) {
        }
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    int in_flight_{};
    std::exception_ptr error_;
    std::vector<std::thread> threads_;
};

template <typename T>
auto as_awaitable(task<T> t) {
    return std::move(t);
}

template <typename... Ts>
auto just(Ts... values) -> task<std::tuple<Ts...>> {
    co_return std::tuple<Ts...>{std::move(values)...};
}

} // namespace mini_ref
