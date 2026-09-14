#pragma once

#include <cstddef>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <stdexcept>
#include <vector>

namespace c16_l10 {

class BoundedPcmPipe {
public:
    explicit BoundedPcmPipe(std::size_t limit) : limit_(limit)
    {
        if (!limit) {
            throw std::invalid_argument("empty capacity");
        }
    }

    std::size_t write(const std::vector<int>& input)
    {
        std::scoped_lock guard(lock_);
        if (done_ || aborted_) {
            return 0;
        }
        std::size_t accepted = 0;
        while (accepted < input.size() && buffer_.size() < limit_) {
            buffer_.push_back(input[accepted++]);
        }
        pressure_ += accepted != input.size() ? 1 : 0;
        readable_.notify_all();
        return accepted;
    }

    std::size_t write_wait(const std::vector<int>& input)
    {
        std::unique_lock guard(lock_);
        std::size_t accepted = 0;
        while (accepted < input.size() && !done_ && !aborted_) {
            writable_.wait(guard, [&] { return buffer_.size() < limit_ || done_ || aborted_; });
            while (accepted < input.size() && buffer_.size() < limit_ && !done_ && !aborted_) {
                buffer_.push_back(input[accepted++]);
                readable_.notify_all();
            }
        }
        return accepted;
    }

    std::vector<int> read(std::size_t wanted)
    {
        std::scoped_lock guard(lock_);
        std::vector<int> out;
        while (wanted-- && !buffer_.empty()) {
            out.push_back(buffer_.front());
            buffer_.pop_front();
        }
        if (out.empty() && !done_ && !aborted_) {
            ++underruns_;
        }
        writable_.notify_all();
        return out;
    }

    std::vector<int> read_wait(std::size_t wanted)
    {
        if (wanted == 0) {
            return {};
        }
        std::unique_lock guard(lock_);
        readable_.wait(guard, [&] { return !buffer_.empty() || done_ || aborted_; });
        std::vector<int> out;
        while (wanted-- && !buffer_.empty()) {
            out.push_back(buffer_.front());
            buffer_.pop_front();
        }
        writable_.notify_all();
        return out;
    }

    void close_input() noexcept
    {
        std::scoped_lock guard(lock_);
        done_ = true;
        readable_.notify_all();
        writable_.notify_all();
    }

    void cancel_flush() noexcept
    {
        std::scoped_lock guard(lock_);
        aborted_ = true;
        done_ = true;
        buffer_.clear();
        readable_.notify_all();
        writable_.notify_all();
    }

    bool eof() const noexcept
    {
        std::scoped_lock guard(lock_);
        return (done_ || aborted_) && buffer_.empty();
    }

    bool cancelled() const noexcept
    {
        std::scoped_lock guard(lock_);
        return aborted_;
    }

    std::size_t size() const noexcept
    {
        std::scoped_lock guard(lock_);
        return buffer_.size();
    }

    std::size_t backpressure_count() const noexcept
    {
        std::scoped_lock guard(lock_);
        return pressure_;
    }

    std::size_t underrun_count() const noexcept
    {
        std::scoped_lock guard(lock_);
        return underruns_;
    }

private:
    std::size_t limit_;
    mutable std::mutex lock_;
    std::condition_variable readable_;
    std::condition_variable writable_;
    std::deque<int> buffer_;
    bool done_ = false;
    bool aborted_ = false;
    std::size_t pressure_ = 0;
    std::size_t underruns_ = 0;
};

} // namespace c16_l10
