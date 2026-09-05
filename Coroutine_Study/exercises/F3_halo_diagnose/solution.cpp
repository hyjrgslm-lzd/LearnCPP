#include <coroutine>
#include <iostream>
#include <new>
#include <utility>
#include "coroutine_study/exercise_check.hpp"

using coroutine_study::check;

int hook_allocations = 0;
int hook_deallocations = 0;
int direct_completions = 0;

struct hook_task {
    struct promise_type {
        int value = 0;
        static void* operator new(std::size_t n) {
            ++hook_allocations;
            return ::operator new(n);
        }
        static void operator delete(void* p, std::size_t) noexcept {
            ++hook_deallocations;
            ::operator delete(p);
        }
        hook_task get_return_object() noexcept { return hook_task{std::coroutine_handle<promise_type>::from_promise(*this)}; }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_value(int v) noexcept { value = v; }
        void unhandled_exception() { throw; }
    };
    explicit hook_task(std::coroutine_handle<promise_type> h) noexcept : h_(h) {}
    hook_task(hook_task&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    hook_task(const hook_task&) = delete;
    ~hook_task() { if (h_) h_.destroy(); }
    int run() { h_.resume(); return h_.promise().value; }
    std::coroutine_handle<promise_type> h_;
};

hook_task escaping_frame() { co_return 7; }

struct direct_task {
    struct promise_type {
        direct_task get_return_object() noexcept { return {}; }
        std::suspend_never initial_suspend() noexcept { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void return_void() noexcept { ++direct_completions; }
        void unhandled_exception() { throw; }
    };
};

direct_task non_escaping_candidate() { co_return; }

int main() {
    {
        auto t = escaping_frame();
        check(t.run() == 7, "escaping coroutine frame produces a stable value");
    }
    non_escaping_candidate();
    std::cout << "[halo-fixture] escaping hook allocations=" << hook_allocations
              << " deallocations=" << hook_deallocations << '\n';
    std::cout << "[halo-fixture] non-escaping direct completions=" << direct_completions << '\n';
    check(hook_allocations == hook_deallocations, "escaping allocation hook is paired after task destruction");
    check(direct_completions == 1, "non-escaping candidate completed synchronously");
}
