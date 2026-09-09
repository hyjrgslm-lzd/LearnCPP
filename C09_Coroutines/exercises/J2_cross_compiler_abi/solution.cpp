#include <atomic>
#include <coroutine>
#include <coroutine_study/exercise_check.hpp>
#include <exception>
#include <iostream>
#include <type_traits>
#include <utility>
#include <variant>

template <typename T>
struct task {
    struct promise_type {
        static inline std::atomic<int> allocations{0};

        std::variant<std::monostate, T, std::exception_ptr> result;
        std::coroutine_handle<> continuation;

        static void* operator new(std::size_t n) {
            ++allocations;
            return ::operator new(n);
        }
        static void operator delete(void* p, std::size_t n) noexcept {
            (void)n;
            ::operator delete(p);
        }

        task get_return_object() {
            return task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() noexcept { return {}; }

        struct final_awaiter {
            bool await_ready() noexcept { return false; }
            std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> h) noexcept {
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

    explicit task(std::coroutine_handle<promise_type> handle) : h(handle) {}
    task(task&& other) noexcept : h(std::exchange(other.h, {})) {}
    task(const task&) = delete;
    ~task() { if (h) h.destroy(); }

    bool await_ready() const noexcept { return false; }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) noexcept {
        h.promise().continuation = caller;
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

struct ready_int {
    int value;
    bool await_ready() const noexcept { return true; }
    void await_suspend(std::coroutine_handle<>) const noexcept {}
    int await_resume() const noexcept { return value; }
};

task<int> empty_body() { co_return 0; }

task<int> five_ints() {
    int a = 1;
    int b = 2;
    int c = 3;
    int d = 4;
    int e = 5;
    co_return a + b + c + d + e;
}

task<int> with_await() {
    int x = co_await ready_int{10};
    int y = co_await ready_int{32};
    co_return x + y;
}

task<int> abi_boundary_shape() { co_return 42; }

int main() {
    auto before = task<int>::promise_type::allocations.load();
    coroutine_study::check(empty_body().sync_wait() == 0, "empty_body failed");
    coroutine_study::check(five_ints().sync_wait() == 15, "five_ints failed");
    coroutine_study::check(with_await().sync_wait() == 42, "with_await failed");
    coroutine_study::check(abi_boundary_shape().sync_wait() == 42, "abi_boundary_shape failed");
    auto after = task<int>::promise_type::allocations.load();

    std::cout << "sizeof(task<int>)=" << sizeof(task<int>) << '\n';
    std::cout << "sizeof(coroutine_handle<>)=" << sizeof(std::coroutine_handle<>) << '\n';
    std::cout << "coroutine_handle trivially_copyable="
              << std::boolalpha << std::is_trivially_copyable_v<std::coroutine_handle<>> << '\n';
    std::cout << "promise allocations observed=" << (after - before) << '\n';
    std::cout << "ABI rule: expose result APIs across module boundaries; keep coroutine_handle ownership inside one toolchain/runtime boundary.\n";
    std::cout << "J2 reference: deterministic ABI probes passed\n";
}
