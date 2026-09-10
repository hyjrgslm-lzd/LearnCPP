#ifndef CONCURRENCY_STUDY_QUEUE_BASELINE_HPP
#define CONCURRENCY_STUDY_QUEUE_BASELINE_HPP

#include <atomic>
#include <cstddef>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace cs::queue_lab {

#if defined(CS_QUEUE_DIAGNOSTICS) && !defined(CS_QUEUE_DIAGNOSTIC_SUPPORT_HPP)
#define CS_QUEUE_DIAGNOSTIC_SUPPORT_HPP
struct queue_diagnostic_counters {
    std::atomic<std::size_t> mutex_acquisitions{0};
    std::atomic<std::size_t> spsc_remote_loads{0};
};

struct queue_diagnostic_snapshot {
    std::size_t mutex_acquisitions = 0;
    std::size_t spsc_remote_loads = 0;
};

inline queue_diagnostic_counters queue_diagnostics;

inline void reset_queue_diagnostics() noexcept {
    queue_diagnostics.mutex_acquisitions.store(0, std::memory_order_relaxed);
    queue_diagnostics.spsc_remote_loads.store(0, std::memory_order_relaxed);
}

inline queue_diagnostic_snapshot read_queue_diagnostics() noexcept {
    return {
        queue_diagnostics.mutex_acquisitions.load(std::memory_order_relaxed),
        queue_diagnostics.spsc_remote_loads.load(std::memory_order_relaxed),
    };
}

inline void note_mutex_acquired() noexcept {
    queue_diagnostics.mutex_acquisitions.fetch_add(1, std::memory_order_relaxed);
}

inline void note_spsc_remote_load() noexcept {
    queue_diagnostics.spsc_remote_loads.fetch_add(1, std::memory_order_relaxed);
}
#elif !defined(CS_QUEUE_DIAGNOSTIC_SUPPORT_HPP)
#define CS_QUEUE_DIAGNOSTIC_SUPPORT_HPP
inline void note_mutex_acquired() noexcept {}
inline void note_spsc_remote_load() noexcept {}
#endif

// Bounded FIFO baseline. try_* never waits for space/data, but mutex acquisition
// may block. Destruction requires all participating threads to have stopped.
template<class T>
class mutex_queue {
    static_assert(std::is_nothrow_move_assignable_v<T>,
                  "the teaching pop operation requires nonthrowing assignment");
    static_assert(std::is_nothrow_destructible_v<T>);
public:
    explicit mutex_queue(std::size_t capacity) : capacity_(capacity) {
        if (capacity == 0) throw std::invalid_argument("queue capacity must be positive");
    }

    mutex_queue(const mutex_queue&) = delete;
    mutex_queue& operator=(const mutex_queue&) = delete;

    bool try_push(const T& value) {
        std::lock_guard lock(mutex_);
        note_mutex_acquired();
        if (queue_.size() == capacity_) return false;
        queue_.push(value);
        return true;
    }

    bool try_pop(T& value) {
        std::lock_guard lock(mutex_);
        note_mutex_acquired();
        if (queue_.empty()) return false;
        value = std::move(queue_.front());
        queue_.pop();
        return true;
    }

private:
    const std::size_t capacity_;
    std::mutex mutex_;
    std::queue<T> queue_;
};

} // namespace cs::queue_lab
#endif
