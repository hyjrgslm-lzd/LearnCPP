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

    [[nodiscard]] bool owns() const noexcept { return id_ != empty; }
    [[nodiscard]] bool is_first() const noexcept { return kind_ == ResourceKind::first && owns(); }
    [[nodiscard]] bool is_second() const noexcept { return kind_ == ResourceKind::second && owns(); }
    [[nodiscard]] int id() const noexcept { return id_; }

private:
    static constexpr int empty = -1;

    ResourceHandle(ResourceKind kind, int id) noexcept
        : kind_(kind),
          id_(id) {}

    ResourceKind kind_ = ResourceKind::first;
    int id_ = empty;
};

namespace detail {

inline constexpr int max_events = 64;
inline constexpr int max_ids = 64;
inline ResourceCounters counters;
inline AcquisitionPlan plan;
inline int next_id = 1;
inline std::array<bool, max_ids> live_first_ids{};
inline std::array<bool, max_ids> live_second_ids{};
inline int event_count = 0;
inline std::array<Event, max_events> events{};

inline void record(EventKind kind, int id = -1) noexcept {
    if (event_count < max_events) {
        events[static_cast<std::size_t>(event_count)] = Event{kind, id};
        ++event_count;
    }
}

} // namespace detail

inline void reset_resource_model(AcquisitionPlan next_plan = {}) {
    if (detail::counters.first_alive != 0 || detail::counters.second_alive != 0) {
        throw std::logic_error("cannot reset resource model while resources are alive");
    }
    detail::counters = ResourceCounters{};
    detail::plan = next_plan;
    detail::next_id = 1;
    detail::live_first_ids = {};
    detail::live_second_ids = {};
    detail::event_count = 0;
    detail::events = {};
}

[[nodiscard]] inline ResourceCounters resource_counters() noexcept {
    return detail::counters;
}

[[nodiscard]] inline int resource_event_count() noexcept {
    return detail::event_count;
}

[[nodiscard]] inline Event resource_event(int index) noexcept {
    if (index < 0 || index >= detail::event_count) {
        return Event{EventKind::invalid_release, -1};
    }
    return detail::events[static_cast<std::size_t>(index)];
}

inline ResourceHandle acquire_first() {
    if (detail::plan.throw_on_acquire == 1) {
        detail::record(EventKind::throw_first);
        throw std::runtime_error("first acquire failed");
    }
    const int id = detail::next_id++;
    ++detail::counters.first_acquired;
    ++detail::counters.first_alive;
    if (id < detail::max_ids) {
        detail::live_first_ids[static_cast<std::size_t>(id)] = true;
    }
    detail::record(EventKind::acquire_first, id);
    return ResourceHandle::first(id);
}

inline ResourceHandle acquire_second() {
    if (detail::plan.throw_on_acquire == 2) {
        detail::record(EventKind::throw_second);
        throw std::runtime_error("second acquire failed");
    }
    const int id = detail::next_id++;
    ++detail::counters.second_acquired;
    ++detail::counters.second_alive;
    if (id < detail::max_ids) {
        detail::live_second_ids[static_cast<std::size_t>(id)] = true;
    }
    detail::record(EventKind::acquire_second, id);
    return ResourceHandle::second(id);
}

inline void release_first(ResourceHandle handle) noexcept {
    const bool valid_id = handle.id() > 0 && handle.id() < detail::max_ids;
    if (!handle.is_first() || !valid_id || !detail::live_first_ids[static_cast<std::size_t>(handle.id())]) {
        ++detail::counters.invalid_release;
        detail::record(EventKind::invalid_release, handle.id());
        return;
    }
    --detail::counters.first_alive;
    ++detail::counters.first_released;
    detail::live_first_ids[static_cast<std::size_t>(handle.id())] = false;
    detail::record(EventKind::release_first, handle.id());
}

inline void release_second(ResourceHandle handle) noexcept {
    const bool valid_id = handle.id() > 0 && handle.id() < detail::max_ids;
    if (!handle.is_second() || !valid_id || !detail::live_second_ids[static_cast<std::size_t>(handle.id())]) {
        ++detail::counters.invalid_release;
        detail::record(EventKind::invalid_release, handle.id());
        return;
    }
    --detail::counters.second_alive;
    ++detail::counters.second_released;
    detail::live_second_ids[static_cast<std::size_t>(handle.id())] = false;
    detail::record(EventKind::release_second, handle.id());
}

} // namespace l07_support
