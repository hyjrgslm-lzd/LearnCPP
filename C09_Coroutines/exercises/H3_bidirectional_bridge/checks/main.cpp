#include "fixture.hpp"
#include "student.hpp"

#include <stdexec/execution.hpp>

#include <cstdio>
#include <exception>
#include <optional>
#include <stdexcept>
#include <utility>

namespace ex = stdexec;

namespace {
struct checker_trace {
    int sender_task_body = 0;
    int inner_body = 0;
    int outer_after_await = 0;
    int bridge_after_await = 0;
};

checker_trace* current_trace = nullptr;
h3_bridge_fixture* current_bridge_fixture = nullptr;
} // namespace

my_task<int> sender_task(int value)
{
    ++current_trace->sender_task_body;
    co_return value;
}

my_task<int> inner_task(int value)
{
    ++current_trace->inner_body;
    co_return value;
}

my_task<int> outer_task(int value)
{
    int v = co_await inner_task(value);
    ++current_trace->outer_after_await;
    co_return v * 2;
}

my_task<int> bridge_task(int value)
{
    int v = co_await current_bridge_fixture->sender(ex::just(value));
    ++current_trace->bridge_after_await;
    co_return v + 1;
}

class int_receiver {
    enum class channel { none, value, error, stopped };
    struct state {
        std::optional<int> value;
        std::exception_ptr error;
        int completions = 0;
        channel completed = channel::none;
    };
    state* result_;
    explicit int_receiver(state& result) noexcept : result_(&result) {}
    template<class Sender> friend int run_inline_sender(Sender&& sender);
public:
    using receiver_concept = ex::receiver_t;
    int_receiver(const int_receiver&) = default;
    int_receiver(int_receiver&&) = default;
    friend void tag_invoke(ex::set_value_t, int_receiver&& self, int v) noexcept {
        ++self.result_->completions;
        self.result_->completed = channel::value;
        self.result_->value.emplace(v);
    }
    friend void tag_invoke(ex::set_error_t, int_receiver&& self, std::exception_ptr error) noexcept {
        ++self.result_->completions;
        self.result_->completed = channel::error;
        self.result_->error = std::move(error);
    }
    friend void tag_invoke(ex::set_stopped_t, int_receiver&& self) noexcept {
        ++self.result_->completions;
        self.result_->completed = channel::stopped;
    }
    friend auto tag_invoke(ex::get_env_t, const int_receiver&) noexcept { return ex::empty_env{}; }
};

template <class Sender>
int run_inline_sender(Sender&& sender)
{
    int_receiver::state result;
    auto op = ex::connect(static_cast<Sender&&>(sender), int_receiver{result});
    if (result.completions != 0) throw std::runtime_error("connect must not complete a task");
    ex::start(op);
    if (result.completions != 1) throw std::runtime_error("sender must complete exactly once");
    if (result.completed == int_receiver::channel::error) {
        if (!result.error || result.value) throw std::runtime_error("invalid error completion");
        std::rethrow_exception(result.error);
    }
    if (result.completed != int_receiver::channel::value || !result.value || result.error)
        throw std::runtime_error("expected one value completion");
    return *result.value;
}

int main() try
{
    checker_trace trace;
    h3_bridge_fixture bridge_fixture;
    current_trace = &trace;
    current_bridge_fixture = &bridge_fixture;

    const int sender = run_inline_sender(sender_task(31));
    const int awaited = run_inline_sender(outer_task(11));
    const int bridged = run_inline_sender(bridge_task(40));
    const bool ok = sender == 31
        && awaited == 22
        && bridged == 41
        && trace.sender_task_body == 1
        && trace.inner_body == 1
        && trace.outer_after_await == 1
        && bridge_fixture.saw_bridge_once()
        && trace.bridge_after_await == 1;
    if (!ok) {
        std::printf(
            "student check failed: values=%d/%d/%d trace=%d/%d/%d/%d/%d/%d, expected 31/22/41 and 1/1/1/1/1/1\n",
            sender,
            awaited,
            bridged,
            trace.sender_task_body,
            trace.inner_body,
            trace.outer_after_await,
            bridge_fixture.bridge_sender_started(),
            bridge_fixture.bridge_sender_completed(),
            trace.bridge_after_await);
        return 1;
    }
    std::puts("H3 student check passed.");
}

catch (const std::exception& error) {
    std::printf("student check failed: %s\n", error.what());
    return 1;
}
