#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace cs {

// Unbounded, mutex-based teaching pool. Never destroy it on one of its workers.
// shutdown closes admission and returns a drain future; join also joins threads.
class work_stealing_pool {
    using task = std::function<void()>;
    struct queue { std::mutex mutex; std::deque<task> tasks; };
    std::vector<std::unique_ptr<queue>> queues_;
    // ponytail: global accounting lock and O(P) victim scan; profile before sharding metadata.
    std::mutex state_, joining_;
    std::condition_variable changed_;
    bool closed_ = false, signalled_ = false;
    std::size_t queued_ = 0, active_ = 0, next_ = 0;
    std::promise<void> drained_;
    std::shared_future<void> drain_ = drained_.get_future().share();
    std::atomic<std::size_t> steals_{0};
    // Last member: all state remains alive until threads have been joined.
    std::vector<std::thread> workers_;
    static inline thread_local work_stealing_pool* current_ = nullptr;
    static inline thread_local std::size_t index_ = 0;

    void signal_locked() {
        if (closed_ && queued_ == 0 && active_ == 0 && !signalled_) {
            signalled_ = true;
            drained_.set_value();
        }
    }

    bool run_one(std::size_t self) {
        task work;
        for (std::size_t offset = 0; offset < queues_.size(); ++offset) {
            auto& q = *queues_[(self + offset) % queues_.size()];
            {
                std::lock_guard lock(q.mutex);
                if (q.tasks.empty()) continue;
                if (offset == 0) {
                    work = std::move(q.tasks.back());
                    q.tasks.pop_back();
                } else {
                    work = std::move(q.tasks.front());
                    q.tasks.pop_front();
                    steals_.fetch_add(1, std::memory_order_relaxed);
                }
            } // Never acquire state_ while holding a queue lock.
            {
                std::lock_guard lock(state_);
                --queued_;
                ++active_;
            }
            work(); // Only packaged_task wrappers: user exceptions enter futures.
            {
                std::lock_guard lock(state_);
                --active_;
                signal_locked();
            }
            changed_.notify_all(); // Also wakes cooperative waits for this result.
            return true;
        }
        return false;
    }

    void worker(std::size_t self) {
        current_ = this;
        index_ = self;
        for (;;) {
            if (run_one(self)) continue;
            std::unique_lock lock(state_);
            changed_.wait(lock, [&] { return queued_ != 0 || signalled_; });
            if (signalled_) break;
        }
        current_ = nullptr;
    }

public:
    explicit work_stealing_pool(std::size_t count) {
        if (count == 0) throw std::invalid_argument("worker count must be positive");
        queues_.reserve(count);
        for (std::size_t i = 0; i < count; ++i) queues_.push_back(std::make_unique<queue>());
        workers_.reserve(count);
        try {
            for (std::size_t i = 0; i < count; ++i)
                workers_.emplace_back([this, i] { worker(i); });
        } catch (...) {
            shutdown(); // Construction failure must wake and join partial workers.
            for (auto& t : workers_) t.join();
            throw;
        }
    }
    work_stealing_pool(const work_stealing_pool&) = delete;
    work_stealing_pool& operator=(const work_stealing_pool&) = delete;
    ~work_stealing_pool() { join(); }

    template<class F>
    auto submit(F&& f) -> std::future<std::invoke_result_t<std::decay_t<F>&>> {
        using result = std::invoke_result_t<std::decay_t<F>&>;
        auto packaged = std::make_shared<std::packaged_task<result()>>(std::forward<F>(f));
        auto future = packaged->get_future();
        task wrapped = [packaged] { (*packaged)(); };
        {
            std::lock_guard lock(state_);
            if (closed_) throw std::runtime_error("submit after shutdown");
            auto& q = *queues_[current_ == this ? index_ : next_++ % queues_.size()];
            std::lock_guard queue_lock(q.mutex);
            q.tasks.push_back(std::move(wrapped)); // Failure leaves counters unchanged.
            ++queued_;
        }
        changed_.notify_one();
        return future;
    }

    std::shared_future<void> shutdown() {
        {
            std::lock_guard lock(state_);
            closed_ = true;
            signal_locked();
        }
        changed_.notify_all();
        return drain_;
    }

    void join() {
        if (current_ == this) throw std::logic_error("worker cannot join its own pool");
        std::lock_guard lock(joining_);
        shutdown().get();
        for (auto& t : workers_) if (t.joinable()) t.join();
    }

    // Only same-pool, non-deferred futures in well-nested fork-join code.
    // External callers use get; workers execute pending work while waiting.
    template<class T> T wait(std::future<T>& future) {
        if (current_ != this) return future.get();
        if (!future.valid()) throw std::invalid_argument("invalid future");
        if (future.wait_for(std::chrono::seconds(0)) == std::future_status::deferred)
            throw std::invalid_argument("deferred future is not a pool task");
        auto ready = [&] { return future.wait_for(std::chrono::seconds(0)) == std::future_status::ready; };
        while (!ready()) {
            if (run_one(index_)) continue;
            std::unique_lock lock(state_);
            changed_.wait(lock, [&] { return queued_ != 0 || ready(); });
        }
        return future.get();
    }
    std::size_t steals() const { return steals_.load(std::memory_order_relaxed); }
};

namespace scheduling {
// Same deterministic, skewed work for every scheduler. Unsigned wrap is intended.
inline std::uint64_t work(std::size_t id, std::size_t size) {
    std::uint64_t value = id + 1;
    const std::size_t rounds = id < size / 4 ? 12000 : 120;
    for (std::size_t i = 0; i < rounds; ++i) value = value * 6364136223846793005ULL + 1;
    return value;
}

// One result slot per ID; all joins precede return, including exception unwinding.
inline std::vector<std::uint64_t> run(std::string_view variant, std::size_t size, std::size_t threads) {
    if (!threads) throw std::invalid_argument("threads must be positive");
    if (variant != "static" && variant != "dynamic" && variant != "stealing")
        throw std::invalid_argument("variant must be static, dynamic or stealing");
    std::vector<std::uint64_t> values(size);
    if (variant == "stealing") {
        work_stealing_pool pool(threads);
        std::vector<std::future<void>> results;
        results.reserve(size);
        for (std::size_t id = 0; id < size; ++id)
            results.push_back(pool.submit([&, id] { values[id] = work(id, size); }));
        for (auto& f : results) f.get();
        pool.join();
    } else {
        std::atomic<std::size_t> next{0};
        std::vector<std::future<void>> results;
        std::vector<std::jthread> workers;
        results.reserve(threads);
        workers.reserve(threads);
        for (std::size_t t = 0; t < threads; ++t) {
            std::packaged_task<void()> task([&, t] {
                if (variant == "static") {
                    const auto begin = (size / threads) * t + (t < size % threads ? t : size % threads);
                    const auto end = begin + size / threads + (t < size % threads);
                    for (auto id = begin; id < end; ++id) values[id] = work(id, size);
                } else {
                    for (;;) {
                        const auto id = next.fetch_add(1, std::memory_order_relaxed);
                        if (id >= size) break;
                        values[id] = work(id, size);
                    }
                }
            });
            results.push_back(task.get_future());
            workers.emplace_back(std::move(task));
        }
        for (auto& f : results) f.get();
    }
    return values;
}
} // namespace scheduling
} // namespace cs
