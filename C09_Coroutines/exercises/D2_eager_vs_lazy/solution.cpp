#include <coroutine>
#include <string>
#include <utility>
#include <vector>
#include "coroutine_study/exercise_check.hpp"

using coroutine_study::check;

std::vector<std::string> events;

template <class InitialSuspend>
struct task {
    struct promise_type {
        int value = 0;
        task get_return_object() noexcept { return task{std::coroutine_handle<promise_type>::from_promise(*this)}; }
        InitialSuspend initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_value(int v) noexcept { value = v; }
        void unhandled_exception() { throw; }
    };
    explicit task(std::coroutine_handle<promise_type> h) : h_(h) {}
    task(task&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    ~task() { if (h_) h_.destroy(); }
    int get() { if (!h_.done()) h_.resume(); return h_.promise().value; }
    std::coroutine_handle<promise_type> h_;
};

using lazy_task = task<std::suspend_always>;
using eager_task = task<std::suspend_never>;

lazy_task lazy() { events.push_back("lazy body"); co_return 1; }
eager_task eager() { events.push_back("eager body"); co_return 2; }

int main() {
    events.push_back("before lazy create");
    auto l = lazy();
    events.push_back("after lazy create");
    check(events == std::vector<std::string>({"before lazy create", "after lazy create"}), "lazy body must not run at creation");
    check(l.get() == 1, "lazy get starts body");
    events.clear();
    events.push_back("before eager create");
    auto e = eager();
    events.push_back("after eager create");
    check(events == std::vector<std::string>({"before eager create", "eager body", "after eager create"}), "eager body runs before creator regains control");
    check(e.get() == 2, "eager result survives final suspend");
}
