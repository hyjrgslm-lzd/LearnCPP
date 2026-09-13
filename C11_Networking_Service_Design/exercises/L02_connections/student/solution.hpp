#pragma once
#include <cstddef>
#include <span>
#include <stdexcept>
#include <string_view>
namespace exercise {
class queue {
public:
    bool enqueue(std::string_view) { throw std::logic_error("UNFINISHED: implement bounded write queue"); }
    std::span<const char> front() const { return {}; }
    void consume(std::size_t) { throw std::logic_error("UNFINISHED: consume actual bytes sent"); }
    std::size_t bytes() const { return 0; }
    bool paused() const { return false; }
    bool empty() const { return true; }
};
}
