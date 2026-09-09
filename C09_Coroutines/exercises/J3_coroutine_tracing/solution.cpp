#include <chrono>
#include <coroutine>
#include <coroutine_study/exercise_check.hpp>
#include <exception>
#include <iostream>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>
#include <variant>

struct Registry {
    struct Info {
        std::string name;
        std::string state;
    };

    mutable std::mutex m;
    std::unordered_map<void*, Info> entries;
    std::unordered_map<void*, std::size_t> ids;
    std::size_t next_id = 1;

    std::size_t ensure_id(void* frame) {
        std::lock_guard lock(m);
        auto [it, inserted] = ids.emplace(frame, next_id);
        if (inserted) ++next_id;
        return it->second;
    }

    void set(void* frame, std::string name, std::string state) {
        std::lock_guard lock(m);
        auto [id_it, inserted] = ids.emplace(frame, next_id);
        (void)id_it;
        if (inserted) ++next_id;
        entries[frame] = {std::move(name), std::move(state)};
    }

    void mark(void* frame, std::string state) {
        std::lock_guard lock(m);
        if (auto it = entries.find(frame); it != entries.end()) it->second.state = std::move(state);
    }

    void erase(void* frame) {
        std::lock_guard lock(m);
        entries.erase(frame);
    }

    std::size_t active() const {
        std::lock_guard lock(m);
        return entries.size();
    }
};

Registry& registry() {
    static Registry r;
    return r;
}

void log_event(std::string_view event, std::string_view name, void* frame) {
#ifndef NDEBUG
    static std::mutex out;
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    auto id = registry().ensure_id(frame);
    std::lock_guard lock(out);
    std::cerr << '[' << us << "us] tid=" << std::this_thread::get_id()
              << ' ' << event << ' ' << name << " coro#" << id << '\n';
#else
    (void)event;
    (void)name;
    (void)frame;
#endif
}

template <typename T>
struct traced_task {
    struct promise_type {
        std::variant<std::monostate, T, std::exception_ptr> result;
        std::coroutine_handle<> continuation;

        traced_task get_return_object() {
            return traced_task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        struct initial_awaiter {
            bool await_ready() noexcept { return false; }
            void await_suspend(std::coroutine_handle<promise_type> h) noexcept {
                registry().set(h.address(), "traced_task", "suspended");
                log_event("INIT", "traced_task", h.address());
            }
            void await_resume() noexcept {}
        };

        initial_awaiter initial_suspend() noexcept { return {}; }

        struct final_awaiter {
            bool await_ready() noexcept { return false; }
            std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> h) noexcept {
                log_event("FINAL", "traced_task", h.address());
                registry().erase(h.address());
                return h.promise().continuation ? h.promise().continuation : std::noop_coroutine();
            }
            void await_resume() noexcept {}
        };

        final_awaiter final_suspend() noexcept { return {}; }

        template <typename U>
        void return_value(U&& value) { result.template emplace<1>(std::forward<U>(value)); }
        void unhandled_exception() noexcept { result.template emplace<2>(std::current_exception()); }
    };

    std::coroutine_handle<promise_type> h;

    explicit traced_task(std::coroutine_handle<promise_type> handle) : h(handle) {}
    traced_task(traced_task&& other) noexcept : h(std::exchange(other.h, {})) {}
    traced_task(const traced_task&) = delete;
    ~traced_task() {
        if (!h) return;
        if (!h.done()) registry().erase(h.address());
        h.destroy();
    }

    bool await_ready() const noexcept { return false; }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) noexcept {
        h.promise().continuation = caller;
        registry().mark(h.address(), "running");
        return h;
    }
    T await_resume() {
        auto& r = h.promise().result;
        if (r.index() == 2) std::rethrow_exception(std::get<2>(r));
        return std::move(std::get<1>(r));
    }
    T sync_wait() {
        h.resume();
        return await_resume();
    }
};

struct inline_resume_int {
    int value;
    bool await_ready() const noexcept { return false; }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> h) const noexcept { return h; }
    int await_resume() const noexcept { return value; }
};

template <typename Inner>
struct traced_awaitable {
    Inner inner;
    const char* name;
    void* frame = nullptr;

    bool await_ready() noexcept(noexcept(inner.await_ready())) { return inner.await_ready(); }

    auto await_suspend(std::coroutine_handle<> h) noexcept(noexcept(inner.await_suspend(h))) {
        frame = h.address();
        registry().mark(frame, "suspended");
        log_event("SUSPEND", name, frame);
        return inner.await_suspend(h);
    }

    decltype(auto) await_resume() {
        registry().mark(frame, "running");
        log_event("RESUME", name, frame);
        return inner.await_resume();
    }
};

template <typename Inner>
traced_awaitable<std::decay_t<Inner>> traced(Inner&& inner, const char* name) {
    return {std::forward<Inner>(inner), name};
}

traced_task<int> task_a() {
    int x = co_await traced(inline_resume_int{1}, "a.step1");
    int y = co_await traced(inline_resume_int{2}, "a.step2");
    co_return x + y;
}

traced_task<int> task_b() {
    int x = co_await traced(task_a(), "b.calls_a");
    int y = co_await traced(inline_resume_int{10}, "b.step2");
    co_return x + y;
}

traced_task<int> task_c() {
    int z = co_await traced(task_b(), "c.calls_b");
    co_return z * 2;
}

traced_task<int> dropped_before_resume() {
    co_return 7;
}

int main() {
    {
        auto dropped = dropped_before_resume();
        (void)dropped;
        coroutine_study::check(registry().active() == 1, "unstarted task not registered");
    }
    coroutine_study::check(registry().active() == 0, "dropping unfinished task leaked registry entry");

    auto t = task_c();
    auto result = t.sync_wait();
    coroutine_study::check(result == 26, "task_c returned wrong result");
    coroutine_study::check(registry().active() == 0, "registry not empty after completion");
    std::cout << "J3 reference: result=" << result << ", registry empty after completion\n";
}
