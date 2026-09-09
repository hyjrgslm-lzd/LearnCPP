#pragma once

#include <array>
#include <stdexcept>

namespace l07_support {

enum class ResourceKind { first, second };
enum class EventKind { acquire_first, acquire_second, release_first, release_second, throw_first, throw_second, invalid_release };

struct Event {
    EventKind kind{};
    int id = -1;
};

struct ResourceCounters {
    int first_acquired = 0;
    int second_acquired = 0;
    int first_released = 0;
    int second_released = 0;
    int first_alive = 0;
    int second_alive = 0;
    int invalid_release = 0;
};

struct AcquisitionPlan {
    int throw_on_acquire = 0;
};

class ResourceHandle {
public:
    ResourceHandle() = default;
    [[nodiscard]] static ResourceHandle first(int id) noexcept { return ResourceHandle(ResourceKind::first, id); }
    [[nodiscard]] static ResourceHandle second(int id) noexcept { return ResourceHandle(ResourceKind::second, id); }
    [[nodiscard]] bool owns() const noexcept { return id_ != -1; }
    [[nodiscard]] bool is_first() const noexcept { return kind_ == ResourceKind::first && owns(); }
    [[nodiscard]] bool is_second() const noexcept { return kind_ == ResourceKind::second && owns(); }
    [[nodiscard]] int id() const noexcept { return id_; }

private:
    ResourceHandle(ResourceKind kind, int id) noexcept : kind_(kind), id_(id) {}
    ResourceKind kind_ = ResourceKind::first;
    int id_ = -1;
};

inline ResourceCounters counters;
inline AcquisitionPlan plan;
inline int next_id = 1;
inline int event_count = 0;
inline std::array<Event, 64> events{};

inline void record(EventKind kind, int id = -1) noexcept {
    events[static_cast<std::size_t>(event_count++)] = Event{kind, id};
}

inline void reset_resource_model(AcquisitionPlan next_plan = {}) {
    if (counters.first_alive != 0 || counters.second_alive != 0) {
        throw std::logic_error("cannot reset resource model while resources are alive");
    }
    counters = ResourceCounters{};
    plan = next_plan;
    next_id = 1;
    event_count = 0;
    events = {};
}

[[nodiscard]] inline ResourceCounters resource_counters() noexcept { return counters; }
[[nodiscard]] inline int resource_event_count() noexcept { return event_count; }
[[nodiscard]] inline Event resource_event(int index) noexcept { return events[static_cast<std::size_t>(index)]; }

inline ResourceHandle acquire_first() {
    if (plan.throw_on_acquire == 1) {
        record(EventKind::throw_first);
        throw std::runtime_error("first acquire failed");
    }
    const int id = next_id++;
    ++counters.first_acquired;
    ++counters.first_alive;
    record(EventKind::acquire_first, id);
    return ResourceHandle::first(id);
}

inline ResourceHandle acquire_second() {
    if (plan.throw_on_acquire == 2) {
        record(EventKind::throw_second);
        if (counters.first_alive != 0) {
            --counters.first_alive;
            ++counters.first_released;
            record(EventKind::release_first, 1);
        }
        throw std::runtime_error("second acquire failed");
    }
    const int id = next_id++;
    ++counters.second_acquired;
    ++counters.second_alive;
    record(EventKind::acquire_second, id);
    return ResourceHandle::second(id);
}

inline void release_first(ResourceHandle handle) noexcept {
    --counters.first_alive;
    ++counters.first_released;
    record(EventKind::release_first, handle.id());
}

inline void release_second(ResourceHandle handle) noexcept {
    --counters.second_alive;
    ++counters.second_released;
    record(EventKind::release_second, handle.id());
}

} // namespace l07_support
