#pragma once
#include <atomic>
namespace mini {
class async_scope {
    std::atomic<int> in_flight_{0};
public:
    template <typename Sender>
    void spawn(Sender&&) {}
    void wait_empty() {}
    int in_flight() const noexcept { return in_flight_.load(); }
};
} // namespace mini
