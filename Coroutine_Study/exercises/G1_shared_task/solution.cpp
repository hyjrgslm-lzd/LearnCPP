#include <coroutine>
#include <exception>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>
#include "coroutine_study/exercise_check.hpp"

using coroutine_study::check;

struct void_task {
    struct promise_type {
        void_task get_return_object() noexcept { return void_task{std::coroutine_handle<promise_type>::from_promise(*this)}; }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() { throw; }
    };
    explicit void_task(std::coroutine_handle<promise_type> h) : h_(h) {}
    void_task(void_task&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    ~void_task() { if (h_) h_.destroy(); }
    void start() { if (h_ && !h_.done()) h_.resume(); }
    bool done() const noexcept { return h_ && h_.done(); }
    std::coroutine_handle<promise_type> h_;
};

int producer_runs = 0;

template <class T>
class shared_task {
    struct control_block;
public:
    struct promise_type {
        std::weak_ptr<control_block> owner;
        shared_task get_return_object();
        std::suspend_always initial_suspend() noexcept { return {}; }
        struct final_awaiter {
            bool await_ready() noexcept { return false; }
            std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> h) noexcept {
                auto cb = h.promise().owner.lock();
                if (!cb) return std::noop_coroutine();
                cb->completed = true;
                auto waiters = std::move(cb->waiters);
                for (std::size_t i = 1; i < waiters.size(); ++i) waiters[i].resume();
                return waiters.empty() ? std::noop_coroutine() : waiters[0];
            }
            void await_resume() noexcept {}
        };
        final_awaiter final_suspend() noexcept { return {}; }
        void return_value(T v) noexcept { value.emplace(std::move(v)); }
        void unhandled_exception() noexcept { error = std::current_exception(); }
        std::optional<T> value;
        std::exception_ptr error;
    };

    shared_task() = default;
    explicit shared_task(std::shared_ptr<control_block> cb) : cb_(std::move(cb)) {}
    void resume() const { if (cb_ && cb_->h && !cb_->h.done()) cb_->h.resume(); }
    struct awaiter {
        std::shared_ptr<control_block> cb;
        bool await_ready() const noexcept { return cb->completed; }
        std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) {
            cb->waiters.push_back(caller);
            if (!std::exchange(cb->started, true)) return cb->h;
            return std::noop_coroutine();
        }
        T await_resume() {
            auto& p = cb->h.promise();
            if (p.error) std::rethrow_exception(p.error);
            return *p.value;
        }
    };
    awaiter operator co_await() const noexcept { return awaiter{cb_}; }
private:
    struct control_block {
        explicit control_block(std::coroutine_handle<promise_type> h) : h(h) {}
        ~control_block() { if (h) h.destroy(); }
        std::coroutine_handle<promise_type> h;
        bool started = false;
        bool completed = false;
        std::vector<std::coroutine_handle<>> waiters;
    };
    std::shared_ptr<control_block> cb_;
};

template <class T>
shared_task<T> shared_task<T>::promise_type::get_return_object() {
    auto h = std::coroutine_handle<promise_type>::from_promise(*this);
    auto cb = std::make_shared<control_block>(h);
    owner = cb;
    return shared_task{std::move(cb)};
}

shared_task<int> slow_shared() {
    ++producer_runs;
    co_await std::suspend_always{};
    co_return 42;
}

void_task waiter(shared_task<int> st, int& out) { out = co_await st; }

int main() {
    auto st = slow_shared();
    int a = 0;
    int b = 0;
    auto wa = waiter(st, a);
    auto wb = waiter(st, b);
    wa.start();
    wb.start();
    check(!wa.done() && !wb.done(), "both waiters are suspended on one shared frame");
    check(producer_runs == 1, "producer starts once even with multiple awaiters");
    st.resume();
    check(wa.done() && wb.done(), "final_suspend wakes all waiters");
    check(a == 42 && b == 42, "cached result is visible to every waiter");
}
