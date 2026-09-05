#include <coroutine>
#include <exception>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <utility>
#include "coroutine_study/exercise_check.hpp"

using coroutine_study::check;

struct manual_event {
    std::coroutine_handle<> first;
    std::coroutine_handle<> second;
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) noexcept {
        if (!first) first = h;
        else if (!second) second = h;
    }
    void await_resume() noexcept {}
    void set() noexcept {
        auto a = std::exchange(first, {});
        auto b = std::exchange(second, {});
        if (a) a.resume();
        if (b) b.resume();
    }
};

template <class T>
struct task {
    struct promise_type {
        std::optional<T> value;
        std::exception_ptr error;
        std::coroutine_handle<> continuation;
        task get_return_object() noexcept { return task{std::coroutine_handle<promise_type>::from_promise(*this)}; }
        std::suspend_always initial_suspend() noexcept { return {}; }
        struct final_awaiter {
            bool await_ready() noexcept { return false; }
            std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> h) noexcept {
                return h.promise().continuation ? h.promise().continuation : std::noop_coroutine();
            }
            void await_resume() noexcept {}
        };
        final_awaiter final_suspend() noexcept { return {}; }
        void return_value(T v) noexcept { value.emplace(std::move(v)); }
        void unhandled_exception() noexcept { error = std::current_exception(); }
    };
    explicit task(std::coroutine_handle<promise_type> h) : h_(h) {}
    task(task&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    task(const task&) = delete;
    ~task() { if (h_) h_.destroy(); }
    void start() { if (h_ && !h_.done()) h_.resume(); }
    bool done() const noexcept { return h_ && h_.done(); }
    bool await_ready() const noexcept { return false; }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) noexcept {
        h_.promise().continuation = caller;
        return h_;
    }
    T await_resume() {
        auto& p = h_.promise();
        if (p.error) std::rethrow_exception(p.error);
        return std::move(*p.value);
    }
    std::coroutine_handle<promise_type> h_;
};

struct all_state {
    std::coroutine_handle<> parent;
    int remaining = 2;
    std::optional<int> a;
    std::optional<int> b;
    std::exception_ptr first_error;
    bool launching = false;
};

struct runner_task {
    struct promise_type {
        all_state* state = nullptr;
        runner_task get_return_object() noexcept { return runner_task{std::coroutine_handle<promise_type>::from_promise(*this)}; }
        std::suspend_always initial_suspend() noexcept { return {}; }
        struct final_awaiter {
            bool await_ready() noexcept { return false; }
            std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> h) noexcept {
                auto* s = h.promise().state;
                if (s && --s->remaining == 0 && !s->launching) return s->parent;
                return std::noop_coroutine();
            }
            void await_resume() noexcept {}
        };
        final_awaiter final_suspend() noexcept { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() noexcept {
            if (state && !state->first_error) state->first_error = std::current_exception();
        }
    };
    explicit runner_task(std::coroutine_handle<promise_type> h) : h_(h) {}
    runner_task(runner_task&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    runner_task(const runner_task&) = delete;
    ~runner_task() { if (h_) h_.destroy(); }
    void start() { if (h_ && !h_.done()) h_.resume(); }
    std::coroutine_handle<promise_type> h_;
};

struct bind_state {
    all_state* state;
    bool await_ready() const noexcept { return false; }
    bool await_suspend(std::coroutine_handle<runner_task::promise_type> h) const noexcept {
        h.promise().state = state;
        return false;
    }
    void await_resume() const noexcept {}
};

runner_task runner0(all_state& s, task<int> child) {
    co_await bind_state{&s};
    try { s.a = co_await std::move(child); } catch (...) { if (!s.first_error) s.first_error = std::current_exception(); }
}

runner_task runner1(all_state& s, task<int> child) {
    co_await bind_state{&s};
    try { s.b = co_await std::move(child); } catch (...) { if (!s.first_error) s.first_error = std::current_exception(); }
}

struct when_all_2_awaitable {
    task<int> left;
    task<int> right;
    all_state state;
    std::optional<runner_task> r0;
    std::optional<runner_task> r1;
    bool await_ready() const noexcept { return false; }
    bool await_suspend(std::coroutine_handle<> parent) {
        state.parent = parent;
        state.launching = true;
        r0.emplace(runner0(state, std::move(left)));
        r1.emplace(runner1(state, std::move(right)));
        r0->start();
        r1->start();
        state.launching = false;
        return state.remaining != 0;
    }
    std::tuple<int, int> await_resume() {
        if (state.first_error) std::rethrow_exception(state.first_error);
        return {*state.a, *state.b};
    }
};

when_all_2_awaitable when_all_2(task<int> a, task<int> b) { return {std::move(a), std::move(b)}; }

int started = 0;
int completed = 0;
task<int> branch(manual_event& ev, int value) { ++started; co_await ev; ++completed; co_return value; }
task<int> fail_branch(manual_event& ev) { ++started; co_await ev; ++completed; throw std::runtime_error("boom"); co_return 0; }
task<int> sync_branch(int value) { ++started; ++completed; co_return value; }
task<int> parent_ok(manual_event& ev) { auto [a, b] = co_await when_all_2(branch(ev, 1), branch(ev, 2)); co_return a + b; }
task<int> parent_error(manual_event& ev) { auto pair = co_await when_all_2(fail_branch(ev), branch(ev, 2)); co_return std::get<0>(pair); }
task<int> parent_sync() { auto [a, b] = co_await when_all_2(sync_branch(3), sync_branch(4)); co_return a + b; }

int run_reference() {
    manual_event ev;
    auto p = parent_ok(ev);
    p.start();
    check(started == 2 && completed == 0, "when_all starts both children before either completes");
    check(!p.done(), "parent waits for shared remaining counter");
    ev.set();
    ev.set();
    check(p.done(), "last child final_suspend resumes parent by handle transfer");
    check(p.await_resume() == 3 && completed == 2, "results are stored and settled after all children finish");

    manual_event ev2;
    auto e = parent_error(ev2);
    e.start();
    ev2.set();
    bool caught = false;
    try { (void)e.await_resume(); } catch (const std::runtime_error&) { caught = true; }
    check(caught && completed == 4, "first exception propagates only after both branches settle");

    auto s = parent_sync();
    s.start();
    check(s.done() && s.await_resume() == 7, "synchronously completing children do not hang");
    return 0;
}

int main() {
    try {
        return run_reference();
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
