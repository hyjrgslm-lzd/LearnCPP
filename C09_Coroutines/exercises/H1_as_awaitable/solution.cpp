#include <stdexec/execution.hpp>

#include <atomic>
#include <coroutine>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>

namespace ex = stdexec;

namespace h1 {

struct stopped_error : std::exception {
    const char* what() const noexcept override { return "sender stopped"; }
};

struct stopped_tag {};

struct bridge_state {
    static constexpr std::uintptr_t completed = 1;

    std::atomic<std::uintptr_t> continuation{0};
    std::variant<std::monostate, int, std::exception_ptr, stopped_tag> result;

    void complete(std::coroutine_handle<> h) noexcept
    {
        const auto old = continuation.exchange(completed, std::memory_order_acq_rel);
        if (old != 0) {
            h.resume();
        }
    }
};

template <typename Promise>
struct bridge_receiver {
    using receiver_concept = ex::receiver_t;

    std::coroutine_handle<Promise> coroutine;
    bridge_state* state;

    template <typename V>
    friend void tag_invoke(ex::set_value_t, bridge_receiver&& self, V&& value) noexcept
    {
        self.state->result.template emplace<int>(static_cast<int>(std::forward<V>(value)));
        self.state->complete(self.coroutine);
    }

    template <typename E>
    friend void tag_invoke(ex::set_error_t, bridge_receiver&& self, E&& error) noexcept
    {
        if constexpr (std::is_same_v<std::decay_t<E>, std::exception_ptr>) {
            self.state->result.template emplace<std::exception_ptr>(std::forward<E>(error));
        } else {
            try {
                throw std::forward<E>(error);
            } catch (...) {
                self.state->result.template emplace<std::exception_ptr>(std::current_exception());
            }
        }
        self.state->complete(self.coroutine);
    }

    friend void tag_invoke(ex::set_stopped_t, bridge_receiver&& self) noexcept
    {
        self.state->result.template emplace<stopped_tag>();
        self.state->complete(self.coroutine);
    }

    friend auto tag_invoke(ex::get_env_t, const bridge_receiver&) noexcept
    {
        return ex::empty_env{};
    }
};

template <typename Sender, typename Promise>
struct sender_awaitable {
    Sender sender;
    std::coroutine_handle<Promise> coroutine;
    bridge_state state;

    using receiver_t = bridge_receiver<Promise>;
    using op_state_t = ex::connect_result_t<Sender, receiver_t>;
    std::optional<op_state_t> op_state;

    bool await_ready() const noexcept { return false; }

    bool await_suspend(std::coroutine_handle<Promise> h)
    {
        coroutine = h;
        op_state.emplace(ex::connect(std::move(sender), receiver_t{h, &state}));
        ex::start(*op_state);

        const auto old = state.continuation.exchange(
            reinterpret_cast<std::uintptr_t>(h.address()),
            std::memory_order_acq_rel);
        return old != bridge_state::completed;
    }

    int await_resume()
    {
        if (auto* value = std::get_if<int>(&state.result)) {
            return *value;
        }
        if (auto* error = std::get_if<std::exception_ptr>(&state.result)) {
            std::rethrow_exception(*error);
        }
        if (std::holds_alternative<stopped_tag>(state.result)) {
            throw stopped_error{};
        }
        throw std::logic_error("sender resumed without a completion signal");
    }
};

template <typename Sender, typename Promise>
auto as_awaitable(Sender&& sender, std::coroutine_handle<Promise> h)
{
    return sender_awaitable<std::remove_cvref_t<Sender>, Promise>{
        std::forward<Sender>(sender), h};
}

struct task {
    struct promise_type {
        int value = 0;
        std::exception_ptr error;

        task get_return_object() noexcept
        {
            return task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_value(int v) noexcept { value = v; }
        void unhandled_exception() noexcept { error = std::current_exception(); }

        template <typename Sender>
            requires ex::sender<std::remove_cvref_t<Sender>>
        auto await_transform(Sender&& sender)
        {
            return as_awaitable(
                std::forward<Sender>(sender),
                std::coroutine_handle<promise_type>::from_promise(*this));
        }
    };

    explicit task(std::coroutine_handle<promise_type> h) noexcept : handle(h) {}
    task(task&& other) noexcept : handle(std::exchange(other.handle, {})) {}
    task(const task&) = delete;
    ~task()
    {
        if (handle) {
            handle.destroy();
        }
    }

    int run()
    {
        handle.resume();
        auto& promise = handle.promise();
        if (promise.error) {
            std::rethrow_exception(promise.error);
        }
        return promise.value;
    }

    std::coroutine_handle<promise_type> handle;
};

task value_demo()
{
    const int x = co_await ex::just(42);
    const int y = co_await ex::just(10);
    co_return x + y;
}

task error_demo()
{
    try {
        (void)co_await ex::just_error(
            std::make_exception_ptr(std::runtime_error("whoops")));
    } catch (const std::runtime_error&) {
        co_return -1;
    }
    co_return 0;
}

task stopped_demo()
{
    try {
        (void)co_await ex::just_stopped();
    } catch (const stopped_error&) {
        co_return -2;
    }
    co_return 0;
}

} // namespace h1

int main()
{
    const int value = h1::value_demo().run();
    const int error = h1::error_demo().run();
    const int stopped = h1::stopped_demo().run();

    std::printf("value=%d error=%d stopped=%d\n", value, error, stopped);
    return (value == 52 && error == -1 && stopped == -2) ? 0 : 1;
}
