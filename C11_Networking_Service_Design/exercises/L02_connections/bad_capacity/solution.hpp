#pragma once
#include <deque>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
namespace exercise {
class queue {
    std::deque<std::string> messages;
    std::size_t held = 0, offset = 0;
    bool blocked = false;
public:
    // Deliberate bug: size accounting hides the moved string's spare capacity.
    bool enqueue(std::string s) {
        if (s.size() > 65536-held) return false;
        if (s.empty()) return true;
        const auto n = s.size(); messages.push_back(std::move(s)); held += n;
        if (held >= 49152) blocked = true;
        return true;
    }
    std::span<const char> front() const { return messages.empty() ? std::span<const char>{} : std::span<const char>{messages.front()}.subspan(offset); }
    void consume(std::size_t n) {
        if (!n || n > front().size()) throw std::invalid_argument("bad length");
        offset += n;
        if (offset == messages.front().size()) { held -= messages.front().size(); messages.pop_front(); offset = 0; }
        if (held <= 32768) blocked = false;
    }
    std::size_t bytes() const { return held; }
    bool empty() const { return messages.empty(); }
    bool paused() const { return blocked; }
};
}
