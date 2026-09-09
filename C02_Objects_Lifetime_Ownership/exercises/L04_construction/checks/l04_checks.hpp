#pragma once

#include <array>
#include <cstddef>
#include <exception>
#include <stdexcept>
#include <string>
#include <vector>

namespace l04_checks {

enum class ResourceName {
    first,
    second,
};

enum class Event {
    acquire_first,
    acquire_second,
    throw_first,
    throw_second,
    body_observed,
    release_first,
    release_second,
    release_null,
    double_release,
};

struct ResourceHandle {
    int id = 0;
};

struct AcquireError : std::exception {
    char const* what() const noexcept override {
        return "acquire failed";
    }
};

struct Recorder {
    static constexpr std::size_t max_events = 16;
    static constexpr std::size_t max_slots = 4;

    std::array<Event, max_events> events{};
    std::size_t event_count = 0;
    std::array<int, max_slots> slot_ids{};
    std::array<bool, max_slots> slot_live{};
    int next_id = 1;
    int live = 0;
    int acquire_attempts = 0;
    int fail_acquire_at = -1;

    ResourceHandle acquire(ResourceName name) {
        log(name == ResourceName::first ? Event::acquire_first : Event::acquire_second);
        ++acquire_attempts;
        if (acquire_attempts == fail_acquire_at) {
            log(name == ResourceName::first ? Event::throw_first : Event::throw_second);
            throw AcquireError{};
        }

        auto id = next_id++;
        for (std::size_t index = 0; index < slot_ids.size(); ++index) {
            if (slot_ids[index] == 0) {
                slot_ids[index] = id;
                slot_live[index] = true;
                break;
            }
        }
        ++live;
        return ResourceHandle{id};
    }

    void release(ResourceName name, ResourceHandle handle) noexcept {
        if (handle.id == 0) {
            log(Event::release_null);
            return;
        }

        auto* slot = find_slot(handle.id);
        if (slot == nullptr || !*slot) {
            log(Event::double_release);
            return;
        }

        *slot = false;
        --live;
        log(name == ResourceName::first ? Event::release_first : Event::release_second);
    }

    void observe_body() noexcept {
        log(Event::body_observed);
    }

private:
    void log(Event event) noexcept {
        if (event_count < events.size()) {
            events[event_count] = event;
        }
        ++event_count;
    }

    bool* find_slot(int id) noexcept {
        for (std::size_t index = 0; index < slot_ids.size(); ++index) {
            if (slot_ids[index] == id) {
                return &slot_live[index];
            }
        }
        return nullptr;
    }
};

} // namespace l04_checks
