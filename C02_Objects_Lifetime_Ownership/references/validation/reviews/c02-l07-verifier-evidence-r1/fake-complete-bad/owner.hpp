#pragma once

#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace l07 {

struct ResourceCounters {
    int first_acquired = 0;
    int second_acquired = 0;
    int first_released = 0;
    int second_released = 0;
    int first_alive = 0;
    int second_alive = 0;
};

struct AcquisitionPlan {
    int throw_on_acquire = 0;
};

inline ResourceCounters fake_counters;
inline AcquisitionPlan fake_plan;
inline int fake_next_id = 1;
inline std::vector<std::string> fake_events;

inline void fake_acquire_first(int id) {
    ++fake_counters.first_acquired;
    ++fake_counters.first_alive;
    fake_events.push_back("acquire first " + std::to_string(id));
}

inline void fake_acquire_second(int id) {
    ++fake_counters.second_acquired;
    ++fake_counters.second_alive;
    fake_events.push_back("acquire second " + std::to_string(id));
}

inline void fake_release_first(int id) {
    --fake_counters.first_alive;
    ++fake_counters.first_released;
    fake_events.push_back("release first " + std::to_string(id));
}

inline void fake_release_second(int id) {
    --fake_counters.second_alive;
    ++fake_counters.second_released;
    fake_events.push_back("release second " + std::to_string(id));
}

inline void reset_resource_model(AcquisitionPlan next_plan = {}) {
    fake_counters = ResourceCounters{};
    fake_plan = next_plan;
    fake_next_id = 1;
    fake_events.clear();
}

[[nodiscard]] inline ResourceCounters resource_counters() {
    return fake_counters;
}

[[nodiscard]] inline std::vector<std::string> resource_events() {
    return fake_events;
}

inline void student_ready() {}

class two_resource_owner {
public:
    two_resource_owner() {
        first_id_ = fake_next_id++;
        fake_acquire_first(first_id_);
        if (fake_plan.throw_on_acquire == 2) {
            fake_events.push_back("throw second");
            fake_release_first(first_id_);
            first_id_ = empty;
            throw std::runtime_error("second acquire failed");
        }
        second_id_ = fake_next_id++;
        fake_acquire_second(second_id_);
    }

    explicit two_resource_owner(AcquisitionPlan next_plan) {
        reset_resource_model(next_plan);
        first_id_ = fake_next_id++;
        fake_acquire_first(first_id_);
        if (fake_plan.throw_on_acquire == 2) {
            fake_events.push_back("throw second");
            fake_release_first(first_id_);
            first_id_ = empty;
            throw std::runtime_error("second acquire failed");
        }
        second_id_ = fake_next_id++;
        fake_acquire_second(second_id_);
    }

    ~two_resource_owner() {
        reset();
    }

    two_resource_owner(const two_resource_owner&) = delete;
    two_resource_owner& operator=(const two_resource_owner&) = delete;

    two_resource_owner(two_resource_owner&& other) noexcept
        : first_id_(std::exchange(other.first_id_, empty)),
          second_id_(std::exchange(other.second_id_, empty)) {}

    two_resource_owner& operator=(two_resource_owner&& other) noexcept {
        if (this != &other) {
            reset();
            first_id_ = std::exchange(other.first_id_, empty);
            second_id_ = std::exchange(other.second_id_, empty);
        }
        return *this;
    }

    [[nodiscard]] bool owns_first() const noexcept { return first_id_ != empty; }
    [[nodiscard]] bool owns_second() const noexcept { return second_id_ != empty; }
    [[nodiscard]] int first_id() const noexcept { return first_id_; }
    [[nodiscard]] int second_id() const noexcept { return second_id_; }

    void reset() noexcept {
        if (second_id_ != empty) {
            fake_release_second(std::exchange(second_id_, empty));
        }
        if (first_id_ != empty) {
            fake_release_first(std::exchange(first_id_, empty));
        }
    }

private:
    static constexpr int empty = -1;
    int first_id_ = empty;
    int second_id_ = empty;
};

static_assert(!std::is_copy_constructible_v<two_resource_owner>);
static_assert(!std::is_copy_assignable_v<two_resource_owner>);
static_assert(std::is_nothrow_move_constructible_v<two_resource_owner>);
static_assert(std::is_nothrow_move_assignable_v<two_resource_owner>);

} // namespace l07
