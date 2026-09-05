#include <array>
#include <coroutine>
#include <coroutine_study/exercise_check.hpp>
#include <cstddef>
#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>

struct task {
    struct promise_type {
        std::exception_ptr error;
        static inline int allocations = 0;
        static inline int deallocations = 0;

        static void* operator new(std::size_t size) {
            ++allocations;
            return ::operator new(size);
        }

        static void operator delete(void* ptr, std::size_t size) noexcept {
            (void)size;
            ++deallocations;
            ::operator delete(ptr);
        }

        task get_return_object() {
            return task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() noexcept { error = std::current_exception(); }
    };

    std::coroutine_handle<promise_type> h{};

    explicit task(std::coroutine_handle<promise_type> handle) : h(handle) {}
    task(task&& other) noexcept : h(std::exchange(other.h, {})) {}
    task(const task&) = delete;
    ~task() { if (h) h.destroy(); }

    void run() {
        while (!h.done()) h.resume();
        if (h.promise().error) std::rethrow_exception(h.promise().error);
    }
};

struct ready {
    bool await_ready() const noexcept { return true; }
    void await_suspend(std::coroutine_handle<>) const noexcept {}
    void await_resume() const noexcept {}
};

struct ptr_awaitable {
    const char* p;
    bool await_ready() const noexcept { return true; }
    void await_suspend(std::coroutine_handle<>) const noexcept {}
    std::string_view await_resume() const noexcept { return p; }
};

enum class Category {
    ill_formed,
    undefined_behavior,
    non_recoverable_terminate,
    implementation_defined,
    unspecified,
    engineering_risk,
};

constexpr std::string_view name(Category c) {
    switch (c) {
    case Category::ill_formed: return "ill-formed";
    case Category::undefined_behavior: return "undefined behavior";
    case Category::non_recoverable_terminate: return "std::terminate";
    case Category::implementation_defined: return "implementation-defined";
    case Category::unspecified: return "unspecified";
    case Category::engineering_risk: return "engineering risk";
    }
    return "unknown";
}

struct Trap {
    int id;
    std::string_view title;
    Category category;
    std::string_view safe_rule;
};

constexpr std::array traps{
    Trap{1, "lambda reference crosses suspension", Category::undefined_behavior,
         "capture by value or move ownership into the coroutine frame"},
    Trap{2, "temporary-derived pointer stored by awaiter", Category::undefined_behavior,
         "name the object before co_await so its lifetime covers resume"},
    Trap{3, "thread-affine lock held across co_await", Category::undefined_behavior,
         "release std::mutex/std::lock_guard before any suspension point"},
    Trap{4, "initial_suspend throws", Category::engineering_risk,
         "make initial_suspend noexcept; do not depend on compiler cleanup details"},
    Trap{5, "detached coroutine outlives creator", Category::engineering_risk,
         "use sync_wait or async_scope; never let borrowed state escape"},
    Trap{6, "consumer persists a pointer past the yield window", Category::undefined_behavior,
         "copy the value or invalidate borrowed views before resuming the generator"},
    Trap{7, "promise destructor throws", Category::non_recoverable_terminate,
         "promise destructors must be noexcept and only record cleanup errors"},
    Trap{8, "custom generator persists string_view after co_yield completes", Category::undefined_behavior,
         "store strings by value or bound the view to the active yield window"},
};

task good_lambda_capture() {
    auto value = std::make_shared<int>(42);
    auto read = [value] { return *value; };
    co_await ready{};
    coroutine_study::check(read() == 42, "lambda capture fix failed");
    co_return;
}

task good_temporary_lifetime() {
    std::string s = "hello";
    auto view = co_await ptr_awaitable{s.c_str()};
    coroutine_study::check(view == "hello", "temporary lifetime fix failed");
    co_return;
}

task good_lock_boundary() {
    static std::mutex m;
    int snapshot = 0;
    {
        std::lock_guard lock(m);
        snapshot = 7;
    }
    co_await ready{};
    coroutine_study::check(snapshot == 7, "lock boundary fix failed");
    co_return;
}

task good_initial_suspend_policy() {
    co_await ready{};
    co_return;
}

task structured_child(int value) {
    co_await ready{};
    coroutine_study::check(value == 9, "structured lifetime fix failed");
    co_return;
}

task good_structured_lifetime() {
    auto child = structured_child(9);
    child.run();
    co_return;
}

task good_generator_reference_policy() {
    int frame_owned = 2;
    co_await ready{};
    coroutine_study::check(frame_owned == 2, "generator reference policy failed");
    co_return;
}

struct noexcept_cleanup {
    ~noexcept_cleanup() noexcept { cleaned = true; }
    static inline bool cleaned = false;
};

task good_promise_cleanup_policy() {
    noexcept_cleanup guard;
    co_await ready{};
    (void)guard;
    co_return;
}

task good_string_yield_policy() {
    std::string value = std::string("v") + std::to_string(1);
    co_await ready{};
    coroutine_study::check(value == "v1", "string yield policy failed");
    co_return;
}

int main() {
    for (auto trap : traps) {
        std::cout << "J1-" << trap.id << ": " << trap.title
                  << " | " << name(trap.category)
                  << " | rule: " << trap.safe_rule << '\n';
    }

    good_lambda_capture().run();
    good_temporary_lifetime().run();
    good_lock_boundary().run();
    good_initial_suspend_policy().run();
    good_structured_lifetime().run();
    good_generator_reference_policy().run();
    good_promise_cleanup_policy().run();
    good_string_yield_policy().run();
    coroutine_study::check(noexcept_cleanup::cleaned, "noexcept cleanup did not run");
    coroutine_study::check(
        task::promise_type::allocations == task::promise_type::deallocations,
        "task coroutine frames were not released");

    std::cout << "J1 reference: all safe invariants passed\n";
}
