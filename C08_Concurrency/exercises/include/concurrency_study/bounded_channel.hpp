#pragma once

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace cs {

// MPMC, bounded FIFO. close rejects push but preserves accepted values for pop.
// Destroy only after every caller has returned. Moving/destructing T must not
// throw or reenter this channel. Storage is allocated only during construction.
template<class T>
class bounded_channel {
    static_assert(std::is_nothrow_move_constructible_v<T>);
    static_assert(std::is_nothrow_destructible_v<T>);
public:
    explicit bounded_channel(std::size_t capacity) : slots_(validate(capacity)) {}
    bounded_channel(const bounded_channel&) = delete;
    bounded_channel& operator=(const bounded_channel&) = delete;

    // By-value transfer: even a rejected push may consume the caller's rvalue.
    bool push(T value) {
        std::unique_lock lock(mutex_);
        not_full_.wait(lock, [&] { return closed_ || size_ < slots_.size(); });
        if (closed_) return false;
        slots_[tail_].emplace(std::move(value));
        tail_ = (tail_ + 1) % slots_.size();
        ++size_;
        lock.unlock();
        not_empty_.notify_one();
        return true;
    }

    // nullopt means closed AND drained; an open empty channel waits.
    std::optional<T> pop() {
        std::unique_lock lock(mutex_);
        not_empty_.wait(lock, [&] { return closed_ || size_ != 0; });
        if (size_ == 0) return std::nullopt;
        std::optional<T> result(std::move(*slots_[head_]));
        slots_[head_].reset();
        head_ = (head_ + 1) % slots_.size();
        --size_;
        lock.unlock();
        not_full_.notify_one();
        return result;
    }

    void close() {
        {
            std::lock_guard lock(mutex_);
            closed_ = true;
        }
        not_full_.notify_all();
        not_empty_.notify_all();
    }

private:
    static std::size_t validate(std::size_t capacity) {
        if (capacity == 0) throw std::invalid_argument("channel capacity must be positive");
        return capacity;
    }
    std::vector<std::optional<T>> slots_;
    std::mutex mutex_;
    std::condition_variable not_full_, not_empty_;
    std::size_t head_ = 0, tail_ = 0, size_ = 0;
    bool closed_ = false;
};
} // namespace cs
