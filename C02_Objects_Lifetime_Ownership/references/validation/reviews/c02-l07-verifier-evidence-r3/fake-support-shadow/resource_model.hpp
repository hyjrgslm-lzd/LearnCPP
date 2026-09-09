#pragma once

namespace l07_support {

struct ResourceCounters {
    int first_alive = 0;
    int second_alive = 0;
    int invalid_release = 0;
};

struct AcquisitionPlan {
    int throw_on_acquire = 0;
};

class ResourceHandle {
public:
    [[nodiscard]] bool owns() const noexcept { return true; }
    [[nodiscard]] int id() const noexcept { return 1; }
};

inline void reset_resource_model(AcquisitionPlan = {}) {}
[[nodiscard]] inline ResourceCounters resource_counters() noexcept { return {}; }

} // namespace l07_support
