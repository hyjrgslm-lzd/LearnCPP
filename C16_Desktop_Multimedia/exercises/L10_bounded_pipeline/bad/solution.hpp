#pragma once

#include <cstddef>
#include <deque>
#include <vector>

namespace c16_l10 {

class BoundedPcmPipe {
public:
    explicit BoundedPcmPipe(std::size_t capacity) : capacity_(capacity) {}
    std::size_t write(const std::vector<int>& samples)
    {
        for (int sample : samples) {
            queue_.push_back(sample);
        }
        return samples.size();
    }
    std::size_t write_wait(const std::vector<int>& samples) { return write(samples); }
    std::vector<int> read(std::size_t max_count)
    {
        std::vector<int> out;
        while (max_count-- && !queue_.empty()) {
            out.push_back(queue_.front());
            queue_.pop_front();
        }
        return out;
    }
    std::vector<int> read_wait(std::size_t max_count) { return read(max_count); }
    void close_input() noexcept { closed_ = true; }
    void cancel_flush() noexcept { closed_ = true; }
    bool eof() const noexcept { return closed_; }
    bool cancelled() const noexcept { return false; }
    std::size_t size() const noexcept { return queue_.size(); }
    std::size_t backpressure_count() const noexcept { return 0; }
    std::size_t underrun_count() const noexcept { return 0; }

private:
    std::size_t capacity_;
    std::deque<int> queue_;
    bool closed_ = false;
};

} // namespace c16_l10
