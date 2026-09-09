#pragma once

#include <array>
#include <cstddef>
#include <stdexcept>

namespace l03_support {

struct Slot {
    int value = 0;
    int generation = 0;
    bool alive = false;
};

inline std::array<Slot, 4>& slots() {
    static std::array<Slot, 4> storage{};
    return storage;
}

class Owner {
public:
    explicit Owner(int value) : index_(allocate(value)), generation_(slots()[index_].generation) {}

    ~Owner() {
        if (index_ >= 0) {
            slots()[static_cast<std::size_t>(index_)].alive = false;
        }
    }

    Owner(Owner const&) = delete;
    Owner& operator=(Owner const&) = delete;

    int index() const noexcept {
        return index_;
    }

    int generation() const noexcept {
        return generation_;
    }

    void set(int value) {
        slots()[static_cast<std::size_t>(index_)].value = value;
    }

    void expire_for_test() noexcept {
        slots()[static_cast<std::size_t>(index_)].alive = false;
    }

private:
    static int allocate(int value) {
        auto& storage = slots();
        for (std::size_t index = 0; index < storage.size(); ++index) {
            if (!storage[index].alive) {
                ++storage[index].generation;
                storage[index].value = value;
                storage[index].alive = true;
                return static_cast<int>(index);
            }
        }
        throw std::runtime_error("no lifetime model slot");
    }

    int index_ = -1;
    int generation_ = 0;
};

inline bool alive(int index, int generation) noexcept {
    if (index < 0 || static_cast<std::size_t>(index) >= slots().size()) {
        return false;
    }
    auto const& slot = slots()[static_cast<std::size_t>(index)];
    return slot.alive && slot.generation == generation;
}

inline int read(int index, int generation) {
    if (!alive(index, generation)) {
        throw std::logic_error("borrowed object is not alive");
    }
    return slots()[static_cast<std::size_t>(index)].value;
}

inline int unsafe_peek_for_bad_variant(int index) {
    return slots()[static_cast<std::size_t>(index)].value;
}

} // namespace l03_support
