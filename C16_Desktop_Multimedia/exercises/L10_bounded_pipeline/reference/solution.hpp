#pragma once

#include <algorithm>
#include <cstddef>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <stdexcept>
#include <vector>

namespace c16_l10 {

class BoundedPcmPipe {
public:
    explicit BoundedPcmPipe(std::size_t capacity) : capacity_(capacity)
    {
        if (capacity == 0) {
            throw std::invalid_argument("capacity must be positive");
        }
    }

    std::size_t write(const std::vector<int>& samples)
    {
        std::lock_guard lock(mutex_);
        if (closed_ || cancelled_) {
            return 0;
        }
        const auto free = capacity_ - queue_.size();
        const auto accepted = std::min(free, samples.size());
        for (std::size_t i = 0; i < accepted; ++i) {
            queue_.push_back(samples[i]);
        }
        if (accepted < samples.size()) {
            ++backpressure_;
        }
        not_empty_.notify_all();
        return accepted;
    }

    std::size_t write_wait(const std::vector<int>& samples)
    {
        std::unique_lock lock(mutex_);
        std::size_t accepted = 0;
        while (accepted < samples.size() && !closed_ && !cancelled_) {
            not_full_.wait(lock, [&] { return queue_.size() < capacity_ || closed_ || cancelled_; });
            while (accepted < samples.size() && queue_.size() < capacity_ && !closed_ && !cancelled_) {
                queue_.push_back(samples[accepted++]);
                not_empty_.notify_all();
            }
        }
        return accepted;
    }

    std::vector<int> read(std::size_t max_count)
    {
        std::lock_guard lock(mutex_);
        const auto count = std::min(max_count, queue_.size());
        if (count == 0 && !closed_ && !cancelled_) {
            ++underrun_;
        }
        std::vector<int> out;
        out.reserve(count);
        for (std::size_t i = 0; i < count; ++i) {
            out.push_back(queue_.front());
            queue_.pop_front();
        }
        not_full_.notify_all();
        return out;
    }

    std::vector<int> read_wait(std::size_t max_count)
    {
        if (max_count == 0) {
            return {};
        }
        std::unique_lock lock(mutex_);
        not_empty_.wait(lock, [&] { return !queue_.empty() || closed_ || cancelled_; });
        const auto count = std::min(max_count, queue_.size());
        std::vector<int> out;
        out.reserve(count);
        for (std::size_t i = 0; i < count; ++i) {
            out.push_back(queue_.front());
            queue_.pop_front();
        }
        not_full_.notify_all();
        return out;
    }

    void close_input() noexcept
    {
        std::lock_guard lock(mutex_);
        closed_ = true;
        not_empty_.notify_all();
        not_full_.notify_all();
    }

    void cancel_flush() noexcept
    {
        std::lock_guard lock(mutex_);
        cancelled_ = true;
        closed_ = true;
        queue_.clear();
        not_empty_.notify_all();
        not_full_.notify_all();
    }

    bool eof() const noexcept
    {
        std::lock_guard lock(mutex_);
        return (closed_ || cancelled_) && queue_.empty();
    }

    bool cancelled() const noexcept
    {
        std::lock_guard lock(mutex_);
        return cancelled_;
    }

    std::size_t size() const noexcept
    {
        std::lock_guard lock(mutex_);
        return queue_.size();
    }

    std::size_t backpressure_count() const noexcept
    {
        std::lock_guard lock(mutex_);
        return backpressure_;
    }

    std::size_t underrun_count() const noexcept
    {
        std::lock_guard lock(mutex_);
        return underrun_;
    }

private:
    const std::size_t capacity_;
    mutable std::mutex mutex_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
    std::deque<int> queue_;
    bool closed_ = false;
    bool cancelled_ = false;
    std::size_t backpressure_ = 0;
    std::size_t underrun_ = 0;
};

} // namespace c16_l10
