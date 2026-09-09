#pragma once
#include "mini/sync_wait.hpp"
#include <atomic>
namespace mini {
class async_scope {
    std::atomic<int> in_flight_{0};
public:
    template <typename Sender>
    void spawn(Sender&& sender) {
        ++in_flight_;
        (void)sync_wait(std::forward<Sender>(sender));
        --in_flight_;
    }
    void wait_empty() {}
    int in_flight() const noexcept { return in_flight_.load(); }
};
} // namespace mini
