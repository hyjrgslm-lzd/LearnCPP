#include <array>
#include <atomic>
#include <coroutine>
#include <exception>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <tuple>

void check(bool ok, const char* message) {
    if (!ok) {
        throw std::runtime_error(message);
    }
}

struct manual_task {
    struct promise_type {
        std::exception_ptr error;
        manual_task get_return_object() { return manual_task{std::coroutine_handle<promise_type>::from_promise(*this)}; }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() noexcept { error = std::current_exception(); }
    };

    std::coroutine_handle<promise_type> h{};

    explicit manual_task(std::coroutine_handle<promise_type> handle) : h(handle) {}
    manual_task(manual_task&& other) noexcept : h(std::exchange(other.h, {})) {}
    manual_task& operator=(manual_task&& other) noexcept {
        if (this != &other) {
            if (h) h.destroy();
            h = std::exchange(other.h, {});
        }
        return *this;
    }
    manual_task(const manual_task&) = delete;
    manual_task& operator=(const manual_task&) = delete;
    ~manual_task() { if (h) h.destroy(); }

    void start() {
        check(h && !h.done(), "task must be startable");
        h.resume();
        if (h.promise().error) std::rethrow_exception(h.promise().error);
    }
};

struct start_gate {
    int expected{};
    std::atomic<int> started{0};
    std::atomic<int> completed{0};
    std::atomic<bool> released{false};
    std::array<std::coroutine_handle<>, 8> waiters{};
};

struct gated_awaitable {
    start_gate& gate;
    int slot{};

    bool await_ready() const noexcept { return false; }

    bool await_suspend(std::coroutine_handle<> h) {
        check(slot >= 0 && slot < static_cast<int>(gate.waiters.size()), "slot must be in range");
        gate.waiters[static_cast<std::size_t>(slot)] = h;
        int now = gate.started.fetch_add(1) + 1;
        check(now <= gate.expected, "too many children started");
        if (now == gate.expected) {
            gate.released = true;
            for (int i = 0; i < gate.expected; ++i) {
                auto waiter = gate.waiters[static_cast<std::size_t>(i)];
                if (waiter && waiter != h) {
                    waiter.resume();
                }
            }
            return false;
        }
        return true;
    }

    void await_resume() {
        check(gate.released.load(), "child resumed before all children started");
        gate.completed.fetch_add(1);
    }
};

manual_task child(start_gate& gate, int slot) {
    co_await gated_awaitable{gate, slot};
}

void good_same_thread_fanout() {
    start_gate gate;
    gate.expected = 3;
    auto a = child(gate, 0);
    auto b = child(gate, 1);
    auto c = child(gate, 2);

    a.start();
    check(gate.started == 1 && gate.completed == 0 && !gate.released, "first child should suspend on same thread");
    b.start();
    check(gate.started == 2 && gate.completed == 0 && !gate.released, "second child should suspend on same thread");
    c.start();
    check(gate.started == 3 && gate.completed == 3 && gate.released, "third child should release all same-thread waiters");
}

void bad_missing_slot_is_finite() {
    start_gate gate;
    gate.expected = 3;
    auto a = child(gate, 0);
    auto b = child(gate, 1);

    a.start();
    b.start();

    check(gate.started == 2, "bad case should start exactly two children");
    check(gate.completed == 0, "bad case must not complete before all expected slots arrive");
    check(!gate.released, "bad missing slot must remain unreleased and observable without a timeout");
}

void bad_wrong_result_is_finite() {
    auto observed = std::make_tuple(100, 999, 300);
    auto expected = std::make_tuple(100, 200, 300);
    check(observed != expected, "wrong result slot should be detected by tuple comparison");
}

int main() try {
    good_same_thread_fanout();
    std::cout << "good_same_thread=true\n";
    bad_missing_slot_is_finite();
    std::cout << "bad_missing_slot_detected=true\n";
    bad_wrong_result_is_finite();
    std::cout << "bad_wrong_result_detected=true\n";
    return 0;
} catch (const std::exception& ex) {
    std::cerr << "r3 gate validation failed: " << ex.what() << "\n";
    return 1;
}
