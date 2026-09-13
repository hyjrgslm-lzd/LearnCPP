#pragma once
#include <cstddef>
#include <deque>
#include <algorithm>
#include <memory>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace c11 {
class write_queue {
    struct message { std::unique_ptr<char[]> data; std::size_t size; };
    std::deque<message> messages_;
    std::size_t held_ = 0, offset_ = 0;
    bool paused_ = false;
public:
    static constexpr std::size_t limit = 64 * 1024, high = 48 * 1024, low = 32 * 1024;
    bool enqueue(std::string_view input) {
        if (input.size() > limit - held_) return false;
        if (input.empty()) return true;
        const auto size = input.size();
        auto data = std::make_unique<char[]>(size);
        std::copy(input.begin(), input.end(), data.get());
        // Retain exactly the requested payload allocation, not an input string's
        // potentially huge spare capacity. Container/allocator metadata is separate.
        messages_.push_back(message{std::move(data), size});
        held_ += size;
        if (held_ >= high) paused_ = true;
        return true;
    }
    std::span<const char> front() const noexcept {
        if (messages_.empty()) return {};
        return std::span<const char>{messages_.front().data.get(), messages_.front().size}.subspan(offset_);
    }
    void consume(std::size_t count) {
        if (count == 0 || count > front().size()) throw std::invalid_argument("invalid completed send length");
        offset_ += count;
        if (offset_ == messages_.front().size) {
            held_ -= messages_.front().size; messages_.pop_front(); offset_ = 0;
        }
        if (held_ <= low) paused_ = false;
    }
    std::size_t bytes() const noexcept { return held_; }
    bool paused() const noexcept { return paused_; }
    bool empty() const noexcept { return messages_.empty(); }
};
} // namespace c11
