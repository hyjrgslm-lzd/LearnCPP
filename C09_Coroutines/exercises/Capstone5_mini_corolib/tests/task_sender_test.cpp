#include "mini/task.hpp"
#include "coroutine_study/exercise_check.hpp"
#include <stdexec/execution.hpp>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace ex = stdexec;
struct completion { int calls = 0; int value = 0; std::exception_ptr error; bool stopped = false; };
struct receiver {
    using receiver_concept = ex::receiver_t;
    completion* result;
    friend void tag_invoke(ex::set_value_t, receiver&& self, int value) noexcept {
        ++self.result->calls; self.result->value = value;
    }
    friend void tag_invoke(ex::set_error_t, receiver&& self, std::exception_ptr error) noexcept {
        ++self.result->calls; self.result->error = std::move(error);
    }
    friend void tag_invoke(ex::set_stopped_t, receiver&& self) noexcept {
        ++self.result->calls; self.result->stopped = true;
    }
    friend auto tag_invoke(ex::get_env_t, const receiver&) noexcept { return ex::empty_env{}; }
};
mini::task<int> value() { co_return 42; }
mini::task<int> error() { throw std::runtime_error("task body error"); co_return 0; }
template<class Task>
void inspect(Task task, bool expect_error) {
    if constexpr (ex::sender<Task> && requires(Task t, receiver r) { std::move(t).connect(r); }) {
        completion result;
        // Direct member connect prevents an automatic awaitable fallback masking the exercise.
        auto operation = std::move(task).connect(receiver{&result});
        coroutine_study::check(result.calls == 0, "connect must not start the task");
        ex::start(operation);
        coroutine_study::check(result.calls == 1 && !result.stopped, "task must complete through one channel");
        if (expect_error) {
            coroutine_study::check(static_cast<bool>(result.error), "task error channel missing");
            try { std::rethrow_exception(result.error); }
            catch (const std::runtime_error& e) {
                coroutine_study::check(std::string_view(e.what()) == "task body error", "task error changed");
            }
        } else coroutine_study::check(!result.error && result.value == 42, "task sender value mismatch");
    } else throw std::logic_error("TODO: implement mini::task sender metadata and member connect");
}
int main() {
    try { inspect(value(), false); inspect(error(), true); }
    catch (const std::exception& e) { std::cerr << "starter check failed: " << e.what() << '\n'; return 1; }
    std::cout << "task sender: deferred start, value/error, one completion checked\n";
}
