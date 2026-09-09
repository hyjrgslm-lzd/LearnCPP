#ifndef CONCURRENCY_STUDY_QUEUE_BASELINE_HPP
#define CONCURRENCY_STUDY_QUEUE_BASELINE_HPP

#include <cstddef>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace cs::queue_lab {

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
        if (queue_.size() == capacity_) return false;
        queue_.push(value);
        return true;
    }

    bool try_pop(T& value) {
        std::lock_guard lock(mutex_);
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
