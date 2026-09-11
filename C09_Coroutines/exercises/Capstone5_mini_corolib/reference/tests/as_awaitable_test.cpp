#include "mini_ref/stdexec_awaitable.hpp"

#include <atomic>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <thread>
#include <vector>

struct worker_group {
    template <class Fn>
    void submit(Fn&& fn) {
        std::lock_guard lock{mutex};
        workers.emplace_back(std::forward<Fn>(fn));
    }

    void join() {
        for (;;) {
            std::vector<std::jthread> current;
            {
                std::lock_guard lock{mutex};
                if (workers.empty()) return;
                current.swap(workers);
            }
            for (auto& worker : current) {
                if (worker.joinable()) worker.join();
            }
        }
    }

    ~worker_group() { join(); }

    std::mutex mutex;
    std::vector<std::jthread> workers;
};

struct async_control {
    std::atomic_bool worker_started{false};
    std::atomic_bool release_completion{false};
    std::atomic_bool completed{false};
    std::atomic_int body_resumes{0};
};

struct async_int_sender {
    using sender_concept = stdexec::sender_t;
    std::shared_ptr<async_control> control;
    worker_group* workers{};
    int value{};

    template <typename Receiver>
    struct op {
        std::shared_ptr<async_control> control;
        worker_group* workers{};
        int value{};
        Receiver receiver;

        op(std::shared_ptr<async_control> c, worker_group* w, int v, Receiver r)
            : control(std::move(c)), workers(w), value(v), receiver(std::move(r)) {}
        op(op&&) noexcept = default;
        op(const op&) = delete;
        op& operator=(const op&) = delete;
        op& operator=(op&&) = delete;

        void start() noexcept {
            workers->submit([control = control, value = value, receiver = std::move(receiver)]() mutable {
                control->worker_started.store(true, std::memory_order_release);
                while (!control->release_completion.load(std::memory_order_acquire)) {
                    std::this_thread::yield();
                }
                stdexec::set_value(std::move(receiver), value);
                control->completed.store(true, std::memory_order_release);
            });
        }
    };

    template <typename Receiver>
    friend op<Receiver> tag_invoke(stdexec::connect_t, async_int_sender self, Receiver receiver) {
        return op<Receiver>{std::move(self.control), self.workers, self.value, std::move(receiver)};
    }
};

static mini_ref::task<int> compute() {
    int x = co_await mini_ref::as_stdexec_awaitable(stdexec::just(21));
    co_return x * 2;
}

static mini_ref::task<int> receive_async(std::shared_ptr<async_control> control,
                                        worker_group& workers, int value) {
    int x = co_await mini_ref::as_stdexec_awaitable(async_int_sender{control, &workers, value});
    ++control->body_resumes;
    co_return x + 1;
}

int main() {
    auto result = mini_ref::sync_wait(compute());
    if (!result || std::get<0>(*result) != 42) std::abort();

    auto control = std::make_shared<async_control>();
    worker_group workers;
    std::optional<std::tuple<int>> async_result;
    std::thread waiter{[&] {
        async_result = mini_ref::sync_wait(receive_async(control, workers, 7)).value();
    }};
    while (!control->worker_started.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    control->release_completion.store(true, std::memory_order_release);
    waiter.join();
    workers.join();
    if (!async_result || std::get<0>(*async_result) != 8) std::abort();
    if (!control->completed.load(std::memory_order_acquire)) std::abort();
    if (control->body_resumes != 1) std::abort();

    auto abandoned_control = std::make_shared<async_control>();
    {
        auto abandoned = receive_async(abandoned_control, workers, 9);
        abandoned.start();
        while (!abandoned_control->worker_started.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
    }
    abandoned_control->release_completion.store(true, std::memory_order_release);
    workers.join();
    if (!abandoned_control->completed.load(std::memory_order_acquire)) std::abort();
    if (abandoned_control->body_resumes != 0) std::abort();

    bool error_seen = false;
    try {
        auto failing = []() -> mini_ref::task<int> {
            co_return co_await mini_ref::as_stdexec_awaitable(
                stdexec::just_error(std::make_exception_ptr(std::runtime_error{"boom"})));
        };
        (void)mini_ref::sync_wait(failing());
    } catch (const std::runtime_error&) {
        error_seen = true;
    }
    if (!error_seen) std::abort();

    bool stopped_seen = false;
    try {
        auto stopped = []() -> mini_ref::task<int> {
            co_return co_await mini_ref::as_stdexec_awaitable(stdexec::just_stopped());
        };
        (void)mini_ref::sync_wait(stopped());
    } catch (const std::runtime_error&) {
        stopped_seen = true;
    }
    if (!stopped_seen) std::abort();
}
