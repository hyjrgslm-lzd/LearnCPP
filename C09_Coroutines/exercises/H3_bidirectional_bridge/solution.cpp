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

namespace h3 {

template <typename T>
struct my_task;

template <typename T>
struct task_awaiter;

struct task_promise_marker {};

template <typename T>
struct is_my_task : std::false_type {};

template <typename T>
struct is_my_task<my_task<T>> : std::true_type {};

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

template <typename T>
struct erased_receiver {
    void* object = nullptr;
    void (*value)(void*, T) noexcept = nullptr;
    void (*error)(void*, std::exception_ptr) noexcept = nullptr;
    void (*stopped)(void*) noexcept = nullptr;

    explicit operator bool() const noexcept { return object != nullptr; }
};

template <typename T, typename Receiver>
erased_receiver<T> erase_receiver(Receiver* receiver) noexcept
{
    return {
        receiver,
        [](void* p, T value) noexcept {
            ex::set_value(std::move(*static_cast<Receiver*>(p)), std::move(value));
        },
        [](void* p, std::exception_ptr error) noexcept {
            ex::set_error(std::move(*static_cast<Receiver*>(p)), std::move(error));
        },
        [](void* p) noexcept {
            ex::set_stopped(std::move(*static_cast<Receiver*>(p)));
        }};
}

template <typename T>
struct my_task {
    using sender_concept = ex::sender_tag;
    using completion_signatures = ex::completion_signatures<
        ex::set_value_t(T),
        ex::set_error_t(std::exception_ptr),
        ex::set_stopped_t()>;

    struct promise_type : task_promise_marker {
        std::optional<T> result;
        std::exception_ptr failure;
        std::coroutine_handle<> continuation = std::noop_coroutine();
        erased_receiver<T> external_receiver;

        my_task get_return_object() noexcept
        {
            return my_task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_always initial_suspend() noexcept { return {}; }

        auto final_suspend() noexcept
        {
            struct awaiter {
                bool await_ready() noexcept { return false; }

                std::coroutine_handle<> await_suspend(
                    std::coroutine_handle<promise_type> h) noexcept
                {
                    auto& promise = h.promise();
                    if (promise.external_receiver) {
                        if (promise.failure) {
                            promise.external_receiver.error(
                                promise.external_receiver.object,
                                std::move(promise.failure));
                        } else if (promise.result) {
                            promise.external_receiver.value(
                                promise.external_receiver.object,
                                std::move(*promise.result));
                        } else {
                            promise.external_receiver.stopped(
                                promise.external_receiver.object);
                        }
                    }
                    return promise.continuation;
                }

                void await_resume() noexcept {}
            };
            return awaiter{};
        }

        void return_value(T value) noexcept(std::is_nothrow_move_constructible_v<T>)
        {
            result.emplace(std::move(value));
        }

        void unhandled_exception() noexcept { failure = std::current_exception(); }

        template <typename Sender>
            requires (!is_my_task<std::remove_cvref_t<Sender>>::value
                      && ex::sender<std::remove_cvref_t<Sender>>)
        auto await_transform(Sender&& sender)
        {
            return sender_awaitable<std::remove_cvref_t<Sender>, promise_type>{
                std::forward<Sender>(sender),
                std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        template <typename U>
        task_awaiter<U> await_transform(my_task<U>&& task) noexcept;

        template <typename U>
        my_task<U>& await_transform(my_task<U>&) = delete;
    };

    explicit my_task(std::coroutine_handle<promise_type> h) noexcept : handle(h) {}
    my_task(my_task&& other) noexcept : handle(std::exchange(other.handle, {})) {}
    my_task(const my_task&) = delete;
    my_task& operator=(const my_task&) = delete;
    ~my_task()
    {
        if (handle) {
            handle.destroy();
        }
    }

    template <typename Receiver>
    struct op_state {
        using operation_state_concept = ex::operation_state_t;

        my_task task;
        Receiver receiver;

        friend void tag_invoke(ex::start_t, op_state& self) noexcept
        {
            auto& promise = self.task.handle.promise();
            promise.external_receiver = erase_receiver<T>(&self.receiver);
            promise.continuation = std::noop_coroutine();
            self.task.handle.resume();
        }
    };

    template <typename Receiver>
    auto connect(Receiver receiver)
    {
        return op_state<std::remove_cvref_t<Receiver>>{
            std::move(*this), std::move(receiver)};
    }

    friend auto tag_invoke(ex::get_env_t, const my_task&) noexcept
    {
        return ex::empty_env{};
    }

    std::coroutine_handle<promise_type> handle;
};

template <typename T>
struct task_awaiter {
    my_task<T> task;

    bool await_ready() const noexcept { return task.handle.done(); }

    template <typename Promise>
        requires std::is_base_of_v<task_promise_marker, Promise>
    std::coroutine_handle<> await_suspend(std::coroutine_handle<Promise> caller) noexcept
    {
        task.handle.promise().continuation = caller;
        return task.handle;
    }

    T await_resume()
    {
        auto& promise = task.handle.promise();
        if (promise.failure) {
            std::rethrow_exception(promise.failure);
        }
        return std::move(*promise.result);
    }
};

template <typename T>
template <typename U>
task_awaiter<U> my_task<T>::promise_type::await_transform(my_task<U>&& task) noexcept
{
    return task_awaiter<U>{std::move(task)};
}

my_task<int> sender_side()
{
    co_return 42;
}

my_task<int> inner()
{
    co_return 10;
}

my_task<int> outer()
{
    const int v = co_await inner();
    co_return v * 2;
}

my_task<int> bridge_side()
{
    const int v = co_await ex::just(42);
    co_return v + 1;
}

} // namespace h3

struct int_receiver {
    using receiver_concept = ex::receiver_t;

    std::optional<int>* value;
    std::exception_ptr* error;
    bool* stopped;

    friend void tag_invoke(ex::set_value_t, int_receiver&& self, int v) noexcept
    {
        self.value->emplace(v);
    }

    friend void tag_invoke(ex::set_error_t, int_receiver&& self, std::exception_ptr e) noexcept
    {
        *self.error = std::move(e);
    }

    friend void tag_invoke(ex::set_stopped_t, int_receiver&& self) noexcept
    {
        *self.stopped = true;
    }

    friend auto tag_invoke(ex::get_env_t, const int_receiver&) noexcept
    {
        return ex::empty_env{};
    }
};

template <typename Sender>
int run_inline_sender(Sender&& sender)
{
    std::optional<int> value;
    std::exception_ptr error;
    bool stopped = false;
    auto op = ex::connect(static_cast<Sender&&>(sender), int_receiver{&value, &error, &stopped});
    ex::start(op);
    if (error) {
        std::rethrow_exception(error);
    }
    if (stopped || !value) {
        throw std::runtime_error("sender stopped or produced no value");
    }
    return *value;
}

int main()
{
    const int av = run_inline_sender(h3::sender_side());
    const int bv = run_inline_sender(h3::outer());
    const int cv = run_inline_sender(h3::bridge_side());

    std::printf("sender=%d awaitable=%d bridge=%d\n", av, bv, cv);
    return (av == 42 && bv == 20 && cv == 43) ? 0 : 1;
}
