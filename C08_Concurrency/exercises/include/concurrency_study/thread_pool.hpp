#pragma once

#include "bounded_channel.hpp"
#include <exception>
#include <functional>
#include <future>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace cs {

// Fixed workers, bounded waiting tasks, blocking submit, drain-only shutdown.
// The owner must outlive all API calls and must never be destroyed by its worker.
class thread_pool {
public:
    explicit thread_pool(std::size_t worker_count, std::size_t capacity)
        : tasks_(capacity), errors_(worker_count) {
        if (worker_count == 0) throw std::invalid_argument("worker count must be positive");
        workers_.reserve(worker_count);
        try {
            for (std::size_t i = 0; i < worker_count; ++i)
                workers_.emplace_back([this, i] {
                    current_pool_ = this;
                    try {
                        while (auto task = tasks_.pop()) (*task)();
                    } catch (...) {
                        // User exceptions go to packaged_task's future. This is
                        // an infrastructure failure; report after joining.
                        errors_[i] = std::current_exception();
                        tasks_.close();
                    }
                    current_pool_ = nullptr;
                });
        } catch (...) {
            // The destructor of an incompletely constructed pool will NOT run.
            tasks_.close();
            for (auto& worker : workers_) worker.join();
            throw;
        }
    }

    thread_pool(const thread_pool&) = delete;
    thread_pool& operator=(const thread_pool&) = delete;

    ~thread_pool() noexcept {
        // Self-destruction cannot be repaired by throwing or detaching: workers
        // still access members. This is an explicit owner-lifetime violation.
        if (current_pool_ == this) std::terminate();
        try { shutdown(); } catch (...) { std::terminate(); }
    }

    template<class F, class... A>
    auto submit(F&& f, A&&... args)
        -> std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<A>...>> {
        reject_worker_call();
        using R = std::invoke_result_t<std::decay_t<F>, std::decay_t<A>...>;
        // Captures and result deduction use the SAME decayed, rvalue invocation.
        // Use std::ref explicitly when a borrowed lvalue is intended.
        std::packaged_task<R()> task(
            [fn = std::forward<F>(f), ... values = std::forward<A>(args)]() mutable -> R {
                return std::invoke(std::move(fn), std::move(values)...);
            });
        auto result = task.get_future();
        if (!tasks_.push([task = std::move(task)]() mutable { task(); }))
            throw std::runtime_error("submit on closed thread_pool");
        return result;
    }

    // Concurrent shutdown callers are serialized for join. close and submit
    // linearize on the channel mutex. Accepted tasks drain before return.
    void shutdown() {
        reject_worker_call();
        tasks_.close();
        std::lock_guard lock(shutdown_mutex_);
        for (auto& worker : workers_)
            if (worker.joinable()) worker.join();
        for (const auto& error : errors_)
            if (error) std::rethrow_exception(error);
    }

private:
    void reject_worker_call() const {
        if (current_pool_ == this)
            throw std::logic_error("same-pool worker cannot submit or shutdown");
    }
    inline static thread_local const thread_pool* current_pool_ = nullptr;
    bounded_channel<std::move_only_function<void()>> tasks_;
    std::vector<std::exception_ptr> errors_; // distinct worker slots; read after join
    std::mutex shutdown_mutex_;
    std::vector<std::jthread> workers_; // destroyed before shared state
};
} // namespace cs
