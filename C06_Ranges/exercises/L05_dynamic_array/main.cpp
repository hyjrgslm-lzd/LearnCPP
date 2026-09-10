#include <check.hpp>
#include <dynamic_array.hpp>

#include <cstdint>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace {

struct CopyFallback {
    static inline int alive = 0;
    static inline int copies = 0;
    static inline int moves = 0;
    static inline int throw_on_copy = -1;

    int value = 0;

    explicit CopyFallback(int next = 0) : value(next) { ++alive; }

    CopyFallback(const CopyFallback& other) : value(other.value) {
        ++copies;
        if (throw_on_copy == copies) {
            throw std::runtime_error("planned copy failure");
        }
        ++alive;
    }

    CopyFallback(CopyFallback&& other) noexcept(false) : value(other.value) {
        other.value = -1000;
        ++moves;
        ++alive;
    }

    CopyFallback& operator=(const CopyFallback&) = delete;
    CopyFallback& operator=(CopyFallback&&) = delete;

    ~CopyFallback() noexcept { --alive; }

    static void reset() {
        alive = 0;
        copies = 0;
        moves = 0;
        throw_on_copy = -1;
    }

    static void reset_copy_plan() {
        copies = 0;
        moves = 0;
        throw_on_copy = -1;
    }
};

struct MoveOnly {
    static inline int alive = 0;
    int value = 0;

    explicit MoveOnly(int next = 0) : value(next) { ++alive; }
    MoveOnly(const MoveOnly&) = delete;
    MoveOnly& operator=(const MoveOnly&) = delete;
    MoveOnly(MoveOnly&& other) noexcept : value(other.value) {
        other.value = -1;
        ++alive;
    }
    MoveOnly& operator=(MoveOnly&&) = delete;
    ~MoveOnly() noexcept { --alive; }
    static void reset() { alive = 0; }
};

struct alignas(64) OverAligned {
    int value{};
};

void check_type_contract() {
    using c06_l05::dynamic_array;
    static_assert(!std::is_copy_constructible_v<dynamic_array<int>>);
    static_assert(!std::is_copy_assignable_v<dynamic_array<int>>);
    static_assert(std::is_nothrow_move_constructible_v<dynamic_array<int>>);
    static_assert(std::is_nothrow_move_assignable_v<dynamic_array<int>>);
}

void check_basic_ints() {
    c06_l05::dynamic_array<int> values;
    check(values.empty(), "new dynamic_array is empty");
    check(values.size() == 0, "new dynamic_array size");
    check(values.capacity() == 0, "new dynamic_array capacity");

    values.reserve(2);
    check(values.empty(), "reserve keeps size");
    check(values.capacity() >= 2, "reserve grows capacity");
    const auto capacity = values.capacity();
    values.reserve(1);
    check(values.capacity() == capacity, "reserve never shrinks");

    values.push_back(1);
    values.push_back(2);
    check(values.size() == 2, "push_back increases size");
    check(values[0] == 1 && values[1] == 2, "operator[] reads stored values");
    values[1] = 20;
    check(values.view()[1] == 20, "non-const operator[] writes stored values");

    values.pop_back();
    check(values.size() == 1 && values[0] == 1, "pop_back destroys only the tail");
    values.clear();
    check(values.empty() && values.capacity() == capacity, "clear destroys elements and keeps capacity");

    bool threw = false;
    try {
        values.pop_back();
    } catch (const std::out_of_range&) {
        threw = true;
    }
    check(threw, "pop_back on empty throws out_of_range");
}

void check_borrowing_invalidation_without_ub() {
    c06_l05::dynamic_array<int> values;
    values.reserve(1);
    values.push_back(7);
    std::span<int> borrowed = values.view();
    int* old_data = borrowed.data();
    const auto old_capacity = values.capacity();
    values.push_back(8);
    check(values.capacity() > old_capacity, "second push grows capacity from one");
    check(values.view().data() != old_data, "growth invalidates old borrowed address");
    check(values[0] == 7 && values[1] == 8, "growth preserves values");
}

void check_move_only() {
    MoveOnly::reset();
    c06_l05::dynamic_array<MoveOnly> source;
    source.push_back(MoveOnly{1});
    source.push_back(MoveOnly{2});
    auto* old_data = source.view().data();

    c06_l05::dynamic_array<MoveOnly> target(std::move(source));
    check(source.empty() && source.capacity() == 0, "move construction leaves source empty");
    check(target.size() == 2, "move construction transfers size");
    check(target.view().data() == old_data, "move construction transfers storage");

    c06_l05::dynamic_array<MoveOnly> other;
    other.push_back(MoveOnly{9});
    other = std::move(target);
    check(target.empty(), "move assignment leaves source empty");
    check(other.size() == 2 && other[0].value == 1 && other[1].value == 2, "move assignment transfers values");
    check(MoveOnly::alive == 2, "move-only array owns exactly two live elements");
}

void check_new_tail_failure_keeps_old_state() {
    CopyFallback::reset();
    c06_l05::dynamic_array<CopyFallback> values;
    values.push_back(CopyFallback{1});
    const auto old_capacity = values.capacity();
    const auto old_size = values.size();
    const int old_value = values[0].value;

    CopyFallback value{2};
    CopyFallback::reset_copy_plan();
    CopyFallback::throw_on_copy = 1;
    bool threw = false;
    try {
        values.push_back(std::move(value));
    } catch (const std::runtime_error&) {
        threw = true;
    }

    check(threw, "push new tail failure throws");
    check(values.capacity() == old_capacity, "push growth failure keeps old capacity");
    check(values.size() == old_size, "push growth failure keeps old size");
    check(values[0].value == old_value, "push growth failure keeps old value");
}

void check_reserve_copy_failure_keeps_borrowing() {
    CopyFallback::reset();
    c06_l05::dynamic_array<CopyFallback> values;
    values.reserve(2);
    values.push_back(CopyFallback{1});
    values.push_back(CopyFallback{2});
    auto old = values.view();
    const auto old_capacity = values.capacity();

    CopyFallback::reset_copy_plan();
    CopyFallback::throw_on_copy = 2;
    bool threw = false;
    try {
        values.reserve(8);
    } catch (const std::runtime_error&) {
        threw = true;
    }

    check(threw, "reserve copy failure throws");
    check(values.capacity() == old_capacity, "reserve copy failure keeps old capacity");
    check(values.size() == 2, "reserve copy failure keeps old size");
    check(values.view().data() == old.data(), "reserve copy failure keeps old borrowing valid");
    check(values[0].value == 1 && values[1].value == 2, "reserve copy failure keeps old values");
}

void check_limits_and_alignment() {
    c06_l05::dynamic_array<int> values;
    bool length_error = false;
    try {
        values.reserve(std::numeric_limits<std::size_t>::max());
    } catch (const std::length_error&) {
        length_error = true;
    }
    check(length_error, "oversized reserve is rejected before usable state changes");
    check(values.empty() && values.capacity() == 0, "oversized reserve leaves empty array unchanged");

    c06_l05::dynamic_array<OverAligned> aligned;
    aligned.push_back(OverAligned{5});
    const auto address = reinterpret_cast<std::uintptr_t>(aligned.view().data());
    check(address % alignof(OverAligned) == 0, "allocator returns over-aligned storage");
    check(aligned[0].value == 5, "over-aligned value is stored");
}

} // namespace

int main() {
    check_type_contract();
    check_basic_ints();
    check_borrowing_invalidation_without_ub();
    check_move_only();
    check_new_tail_failure_keeps_old_state();
    check_reserve_copy_failure_keeps_borrowing();
    check_limits_and_alignment();
    check(CopyFallback::alive == 0, "copy fallback tests leave no live objects");
    check(MoveOnly::alive == 0, "move-only tests leave no live objects");
    std::cout << "L05_dynamic_array checks OK\n";
}
