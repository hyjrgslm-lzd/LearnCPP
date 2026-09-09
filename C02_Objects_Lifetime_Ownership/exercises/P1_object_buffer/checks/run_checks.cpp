#include <object_buffer.hpp>

#include <check.hpp>

#include <array>
#include <iostream>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <type_traits>

#if defined(_MSC_VER)
#pragma warning(disable : 4324)
#endif

namespace {

struct CopyFallback {
    static inline int alive = 0;
    static inline int copies = 0;
    static inline int moves = 0;
    static inline int destroys = 0;
    static inline int throw_on_copy = -1;

    int value = 0;

    explicit CopyFallback(int next = 0) : value(next) { ++alive; }

    CopyFallback(const CopyFallback& other) : value(other.value)
    {
        ++copies;
        if (throw_on_copy == copies) {
            throw std::runtime_error("planned copy failure");
        }
        ++alive;
    }

    CopyFallback(CopyFallback&& other) noexcept(false) : value(other.value)
    {
        other.value = -1000;
        ++moves;
        ++alive;
    }

    CopyFallback& operator=(const CopyFallback&) = delete;
    CopyFallback& operator=(CopyFallback&&) = delete;

    ~CopyFallback() noexcept
    {
        --alive;
        ++destroys;
    }

    static void reset_counts()
    {
        alive = 0;
        copies = 0;
        moves = 0;
        destroys = 0;
        throw_on_copy = -1;
    }

    static void reset_copy_plan()
    {
        copies = 0;
        moves = 0;
        throw_on_copy = -1;
    }
};

struct MoveOnly {
    static inline int alive = 0;
    static inline int moves = 0;
    int value = 0;

    explicit MoveOnly(int next = 0) : value(next) { ++alive; }
    MoveOnly(const MoveOnly&) = delete;
    MoveOnly& operator=(const MoveOnly&) = delete;
    MoveOnly(MoveOnly&& other) noexcept : value(other.value)
    {
        other.value = -1;
        ++moves;
        ++alive;
    }
    MoveOnly& operator=(MoveOnly&&) = delete;
    ~MoveOnly() noexcept { --alive; }
    static void reset() { alive = 0; moves = 0; }
};

struct alignas(64) OverAligned {
    int value;
};

void check_basic_ints()
{
    p1::object_buffer<int> buffer;
    check(buffer.size() == 0, "new buffer size");
    check(buffer.capacity() == 0, "new buffer capacity");
    buffer.reserve(2);
    check(buffer.size() == 0, "reserve keeps size");
    check(buffer.capacity() >= 2, "reserve grows capacity");
    const auto capacity = buffer.capacity();
    buffer.reserve(1);
    check(buffer.capacity() == capacity, "reserve no growth keeps capacity");
    buffer.push_back(1);
    check(buffer.size() == 1, "size after first push");
    buffer.push_back(2);
    auto view = buffer.view();
    check(view.size() == 2, "view sees two elements");
    check(view[0] == 1 && view[1] == 2, "view preserves values");
    buffer.pop_back();
    check(buffer.size() == 1 && buffer.view()[0] == 1, "pop removes last element");
    buffer.clear();
    check(buffer.size() == 0, "clear removes all elements");
    check(buffer.capacity() == capacity, "clear keeps capacity");
    bool threw = false;
    try {
        buffer.pop_back();
    } catch (const std::out_of_range&) {
        threw = true;
    }
    check(threw, "pop empty throws out_of_range");
}

void check_self_view_append()
{
    p1::object_buffer<int> buffer;
    buffer.push_back(10);
    auto old = buffer.view();
    buffer.push_back(old[0]);
    check(buffer.view().size() == 2, "self const view append size");
    check(buffer.view()[0] == 10 && buffer.view()[1] == 10, "self const view append value");
}

void check_new_tail_failure_before_old_move()
{
    CopyFallback::reset_counts();
    p1::object_buffer<CopyFallback> buffer;
    buffer.push_back(CopyFallback{1});
    const auto old_capacity = buffer.capacity();
    const auto old_size = buffer.size();
    const int old_value = buffer.view()[0].value;

    CopyFallback value{2};
    CopyFallback::reset_copy_plan();
    CopyFallback::throw_on_copy = 1;
    bool threw = false;
    try {
        buffer.push_back(std::move(value));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    check(threw, "new tail copy failure must throw");
    check(buffer.capacity() == old_capacity, "new tail failure keeps capacity");
    check(buffer.size() == old_size, "new tail failure keeps size");
    check(buffer.view()[0].value == old_value, "new tail failure keeps old value");
    check(CopyFallback::moves == 1, "new tail failure does not move old elements first");
}

void check_reserve_copy_failure_keeps_old_storage()
{
    CopyFallback::reset_counts();
    p1::object_buffer<CopyFallback> buffer;
    buffer.reserve(2);
    buffer.push_back(CopyFallback{1});
    buffer.push_back(CopyFallback{2});
    auto old = buffer.view();
    const auto old_capacity = buffer.capacity();

    CopyFallback::reset_copy_plan();
    CopyFallback::throw_on_copy = 2;
    bool threw = false;
    try {
        buffer.reserve(8);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    check(threw, "reserve copy failure must throw");
    check(buffer.capacity() == old_capacity, "reserve copy failure keeps capacity");
    check(buffer.size() == 2, "reserve copy failure keeps size");
    check(buffer.view().data() == old.data(), "reserve copy failure keeps old borrowing");
    check(buffer.view()[0].value == 1 && buffer.view()[1].value == 2, "reserve copy failure keeps values");
}

void check_growth_old_copy_failure_cleans_new_objects()
{
    CopyFallback::reset_counts();
    p1::object_buffer<CopyFallback> buffer;
    buffer.reserve(1);
    buffer.push_back(CopyFallback{1});
    const int alive_before = CopyFallback::alive;
    const auto old_capacity = buffer.capacity();

    CopyFallback value{2};
    CopyFallback::reset_copy_plan();
    CopyFallback::throw_on_copy = 2;
    bool threw = false;
    try {
        buffer.push_back(std::move(value));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    check(threw, "old copy failure during growth must throw");
    check(buffer.capacity() == old_capacity, "old copy failure keeps capacity");
    check(buffer.size() == 1, "old copy failure keeps size");
    check(buffer.view()[0].value == 1, "old copy failure keeps old value");
    check(CopyFallback::alive == alive_before + 1, "old copy failure cleans new prefix and new tail");
}

void check_move_only_and_borrowing()
{
    MoveOnly::reset();
    p1::object_buffer<MoveOnly> source;
    source.push_back(MoveOnly{1});
    source.push_back(MoveOnly{2});
    auto borrowed = source.view();
    p1::object_buffer<MoveOnly> target(std::move(source));
    check(source.size() == 0 && source.capacity() == 0, "move source becomes empty");
    check(target.size() == 2, "move target receives size");
    check(borrowed.data() == target.view().data(), "move transfers allocation and old borrowing follows target");

    auto& same_target = target;
    target = std::move(same_target);
    check(target.size() == 2, "self move keeps size");
    check(target.view()[0].value == 1 && target.view()[1].value == 2, "self move keeps values");

    p1::object_buffer<MoveOnly> other;
    other.push_back(MoveOnly{9});
    auto old_target = other.view();
    other = std::move(target);
    check(other.size() == 2, "move assignment receives source");
    check(target.size() == 0, "move assignment source empty");
    check(old_target.data() != other.view().data(), "move assignment invalidates target old borrowing");
}

void check_overaligned()
{
    p1::object_buffer<OverAligned> buffer;
    buffer.push_back(OverAligned{7});
    const auto address = reinterpret_cast<std::uintptr_t>(buffer.view().data());
    check(address % alignof(OverAligned) == 0, "allocator provides over-aligned storage");
    check(buffer.view()[0].value == 7, "over-aligned value stored");
}

void check_type_contract()
{
    static_assert(!std::is_copy_constructible_v<p1::object_buffer<int>>);
    static_assert(!std::is_copy_assignable_v<p1::object_buffer<int>>);
    static_assert(std::is_nothrow_move_constructible_v<p1::object_buffer<int>>);
    static_assert(std::is_nothrow_move_assignable_v<p1::object_buffer<int>>);
}

} // namespace

int main()
{
    check_type_contract();
    check_basic_ints();
    check_self_view_append();
    check_new_tail_failure_before_old_move();
    check_reserve_copy_failure_keeps_old_storage();
    check_growth_old_copy_failure_cleans_new_objects();
    check_move_only_and_borrowing();
    check_overaligned();
    check(CopyFallback::alive == 0, "copy fallback tests leave no live objects");
    check(MoveOnly::alive == 0, "move-only tests leave no live objects");
    std::cout << "P1_object_buffer_contract OK\n";
}
