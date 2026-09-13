#pragma once
#include <cstddef>
#include <deque>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
namespace exercise {
class queue {
    std::deque<std::string> stack;
    std::size_t held = 0, offset = 0;
    bool blocked = false;
public:
    bool enqueue(std::string_view s) {
        if (s.size() > 65536-held) return false;
        if (s.empty()) return true;
        stack.emplace_back(s); held += s.size();
        if (held >= 49152) blocked = true;
        return true;
    }
    // Deliberate bug: consumes the newest queued message first.
    std::span<const char> front() const { return stack.empty() ? std::span<const char>{} : std::span<const char>{stack.back()}.subspan(offset); }
    void consume(std::size_t n) {
        if (!n || n > front().size()) throw std::invalid_argument("bad send length");
        offset += n;
        if (offset == stack.back().size()) { held -= stack.back().size(); stack.pop_back(); offset = 0; }
        if (held <= 32768) blocked = false;
    }
    std::size_t bytes() const { return held; }
    bool paused() const { return blocked; }
    bool empty() const { return stack.empty(); }
};
}
