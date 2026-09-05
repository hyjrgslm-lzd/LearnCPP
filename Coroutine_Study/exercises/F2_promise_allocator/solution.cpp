#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <new>
#include <utility>
#include "coroutine_study/exercise_check.hpp"

using coroutine_study::check;

bool fail_next = false;
std::size_t allocs = 0;
std::size_t frees = 0;
std::uintptr_t last_addr = 0;

struct pool_task {
    struct alignas(64) promise_type {
        static void* operator new(std::size_t size) noexcept {
            if (std::exchange(fail_next, false)) return nullptr;
            void* p = ::operator new(size, std::align_val_t{alignof(promise_type)}, std::nothrow);
            if (p) { ++allocs; last_addr = reinterpret_cast<std::uintptr_t>(p); }
            return p;
        }
        static void operator delete(void* p, std::size_t) noexcept {
            ++frees;
            ::operator delete(p, std::align_val_t{alignof(promise_type)});
        }
        static pool_task get_return_object_on_allocation_failure() noexcept { return pool_task{}; }
        pool_task get_return_object() noexcept { return pool_task{std::coroutine_handle<promise_type>::from_promise(*this)}; }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() { throw; }
    };
    pool_task() = default;
    explicit pool_task(std::coroutine_handle<promise_type> h): h_(h) {}
    pool_task(pool_task&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    ~pool_task(){ if(h_) h_.destroy(); }
    bool valid() const noexcept { return static_cast<bool>(h_); }
    void run(){ h_.resume(); h_.resume(); }
    std::coroutine_handle<promise_type> h_{};
};

pool_task sample() { co_await std::suspend_always{}; co_return; }

int main() {
    { auto t = sample(); check(t.valid(), "normal allocation returns a task"); check(last_addr % alignof(pool_task::promise_type) == 0, "custom allocation satisfies promise alignment"); t.run(); }
    check(allocs == frees, "operator delete pairs with operator new after destroy");
    fail_next = true;
    auto failed = sample();
    check(!failed.valid(), "allocation failure path requires noexcept operator new and get_return_object_on_allocation_failure");
}
