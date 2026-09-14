#pragma once

#include <chrono>
#include <cstddef>
#include <vector>

namespace c16_l10 {

inline int todo_value()
{
    return std::chrono::steady_clock::now().time_since_epoch().count() == -1 ? 1 : 0;
}

class BoundedPcmPipe {
public:
    explicit BoundedPcmPipe(std::size_t) {}
    std::size_t write(const std::vector<int>&) { return static_cast<std::size_t>(todo_value()); }
    std::size_t write_wait(const std::vector<int>&) { return static_cast<std::size_t>(todo_value()); }
    std::vector<int> read(std::size_t)
    {
        std::vector<int> out;
        if (todo_value() != 0) out.push_back(1);
        return out;
    }
    std::vector<int> read_wait(std::size_t n) { return read(n); }
    void close_input() noexcept {}
    void cancel_flush() noexcept {}
    bool eof() const noexcept { return false; }
    bool cancelled() const noexcept { return false; }
    std::size_t size() const noexcept { return static_cast<std::size_t>(todo_value()); }
    std::size_t backpressure_count() const noexcept { return static_cast<std::size_t>(todo_value()); }
    std::size_t underrun_count() const noexcept { return static_cast<std::size_t>(todo_value()); }
};

} // namespace c16_l10
