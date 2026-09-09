#include <storage_slot.hpp>

#include <check.hpp>

#include <iostream>
#include <stdexcept>

#if defined(_MSC_VER)
#pragma warning(disable : 4702)
#endif

namespace {

struct Tracker {
    static inline int alive = 0;
    static inline int constructed = 0;
    static inline int destroyed = 0;
    static inline int throw_on_value = -1;

    int value;

    explicit Tracker(int next) : value(next)
    {
        if (next == throw_on_value) {
            throw std::runtime_error("planned construction failure");
        }
        ++alive;
        ++constructed;
    }

    Tracker(const Tracker&) = delete;
    Tracker& operator=(const Tracker&) = delete;

    ~Tracker() noexcept
    {
        --alive;
        ++destroyed;
    }

    static void reset()
    {
        alive = 0;
        constructed = 0;
        destroyed = 0;
        throw_on_value = -1;
    }
};

void check_empty_destroy()
{
    Tracker::reset();
    l12::storage_slot<Tracker> slot;
    check(!slot.engaged(), "new slot starts empty");
    slot.destroy();
    check(!slot.engaged(), "destroy on empty keeps slot empty");
    check(Tracker::alive == 0, "empty destroy must not touch objects");
}

void check_construct_and_destroy()
{
    Tracker::reset();
    {
        l12::storage_slot<Tracker> slot;
        Tracker& item = slot.construct(7);
        check(slot.engaged(), "slot must report engaged after successful construct");
        check(item.value == 7, "construct returns the live object");
        check(&slot.get() == &item, "get returns the same object");
        check(Tracker::alive == 1, "one object alive after construct");
        slot.destroy();
        check(!slot.engaged(), "destroy clears engaged");
        check(Tracker::alive == 0, "destroy ends object lifetime");
    }
    check(Tracker::destroyed == 1, "object destroyed exactly once");
}

void check_repeated_construct_rejected()
{
    Tracker::reset();
    l12::storage_slot<Tracker> slot;
    slot.construct(1);
    bool rejected = false;
    try {
        slot.construct(2);
    } catch (const std::logic_error&) {
        rejected = true;
    }
    check(rejected, "construct on occupied slot must throw logic_error");
    check(slot.get().value == 1, "failed repeated construct keeps old object");
    slot.destroy();
    check(Tracker::alive == 0, "repeated construct path leaves no leak");
}

void check_construct_failure_does_not_commit()
{
    Tracker::reset();
    Tracker::throw_on_value = 9;
    l12::storage_slot<Tracker> slot;
    bool threw = false;
    try {
        slot.construct(9);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    check(threw, "planned constructor failure must be visible");
    check(!slot.engaged(), "failed construct keeps slot empty");
    check(Tracker::alive == 0, "failed construct must not create a live object");
    slot.destroy();
    check(Tracker::destroyed == 0, "failed construct must not destroy an unconstructed object");
}

void check_destructor_cleans_live_object()
{
    Tracker::reset();
    {
        l12::storage_slot<Tracker> slot;
        slot.construct(3);
        check(Tracker::alive == 1, "live before slot destructor");
    }
    check(Tracker::alive == 0, "slot destructor destroys live object");
}

} // namespace

int main()
{
    check_empty_destroy();
    check_construct_and_destroy();
    check_repeated_construct_rejected();
    check_construct_failure_does_not_commit();
    check_destructor_cleans_live_object();
    std::cout << "L12_storage_contract OK\n";
}
