#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <initializer_list>
#include <stdexcept>

namespace l05_support {

struct Handle {
    int id = 0;
};

struct Slot {
    int id = 0;
    bool alive = false;
    std::array<int, 8> values{};
    std::size_t size = 0;
};

inline std::array<Slot, 8>& slots() {
    static std::array<Slot, 8> storage{};
    return storage;
}

inline int& invalid_releases() {
    static int count = 0;
    return count;
}

inline int& next_id() {
    static int id = 1;
    return id;
}

inline void reset_model() {
    for (auto& slot : slots()) {
        slot = Slot{};
    }
    invalid_releases() = 0;
    next_id() = 1;
}

inline Slot* find(Handle handle) {
    for (auto& slot : slots()) {
        if (slot.id == handle.id) {
            return &slot;
        }
    }
    return nullptr;
}

inline int live_count() {
    int count = 0;
    for (auto const& slot : slots()) {
        if (slot.alive) {
            ++count;
        }
    }
    return count;
}

inline Handle allocate(std::initializer_list<int> values) {
    if (values.size() > 8) {
        throw std::length_error("too many values");
    }
    for (auto& slot : slots()) {
        if (!slot.alive) {
            slot = Slot{};
            slot.id = next_id()++;
            slot.alive = true;
            slot.size = values.size();
            std::copy(values.begin(), values.end(), slot.values.begin());
            return Handle{slot.id};
        }
    }
    throw std::runtime_error("no slot");
}

inline Handle clone(Handle source) {
    auto* source_slot = find(source);
    if (source_slot == nullptr || !source_slot->alive) {
        throw std::logic_error("clone from dead storage");
    }
    for (auto& slot : slots()) {
        if (!slot.alive) {
            slot = Slot{};
            slot.id = next_id()++;
            slot.alive = true;
            slot.size = source_slot->size;
            std::copy_n(source_slot->values.begin(), source_slot->size, slot.values.begin());
            return Handle{slot.id};
        }
    }
    throw std::runtime_error("no slot");
}

inline void release(Handle handle) noexcept {
    if (handle.id == 0) {
        return;
    }
    auto* slot = find(handle);
    if (slot == nullptr || !slot->alive) {
        ++invalid_releases();
        return;
    }
    slot->alive = false;
}

inline std::size_t size(Handle handle) {
    auto* slot = find(handle);
    if (slot == nullptr || !slot->alive) {
        throw std::logic_error("dead storage");
    }
    return slot->size;
}

inline int get(Handle handle, std::size_t index) {
    auto* slot = find(handle);
    if (slot == nullptr || !slot->alive || index >= slot->size) {
        throw std::logic_error("bad access");
    }
    return slot->values[index];
}

inline void set(Handle handle, std::size_t index, int value) {
    auto* slot = find(handle);
    if (slot == nullptr || !slot->alive || index >= slot->size) {
        throw std::logic_error("bad access");
    }
    slot->values[index] = value;
}

} // namespace l05_support
