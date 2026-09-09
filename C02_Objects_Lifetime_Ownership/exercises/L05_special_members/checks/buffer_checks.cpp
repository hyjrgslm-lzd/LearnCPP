#include <check.hpp>
#include <type_traits>
#include <utility>

#include "memory_model.hpp"
#include L05_IMPLEMENTATION_HEADER

namespace {

void check_type_contract() {
    static_assert(std::is_copy_constructible_v<l05::IntBuffer>);
    static_assert(std::is_copy_assignable_v<l05::IntBuffer>);
    static_assert(std::is_nothrow_move_constructible_v<l05::IntBuffer>);
    static_assert(std::is_nothrow_move_assignable_v<l05::IntBuffer>);
}

void check_deep_copy() {
    l05_support::reset_model();
    {
        l05::IntBuffer first{1, 2, 3};
        l05::IntBuffer copy = first;
        check(l05_support::live_count() == 2, "copy must have independent storage");
        first.set(0, 9);
        check(copy.get(0) == 1, "copy keeps old value after source mutation");
        copy.set(1, 8);
        check(first.get(1) == 2, "source keeps old value after copy mutation");
    }
    check(l05_support::live_count() == 0, "copy scenario releases all storage");
    check(l05_support::invalid_releases() == 0, "copy scenario has no invalid release");
}

void check_copy_assignment_releases_target() {
    l05_support::reset_model();
    {
        l05::IntBuffer source{4, 5};
        l05::IntBuffer target{7};
        target = source;
        check(l05_support::live_count() == 2, "copy assignment should release old target storage");
        source.set(0, 6);
        check(target.get(0) == 4, "copy assignment makes an independent copy");
    }
    check(l05_support::live_count() == 0, "copy assignment releases all storage");
    check(l05_support::invalid_releases() == 0, "copy assignment has no invalid release");
}

void check_move_transfers() {
    l05_support::reset_model();
    {
        l05::IntBuffer source{10, 11};
        l05::IntBuffer target = std::move(source);
        check(source.empty(), "move construction leaves source empty by this exercise contract");
        check(target.size() == 2 && target.get(1) == 11, "move construction transfers storage");
    }
    check(l05_support::live_count() == 0, "move construction releases transferred storage");
}

void check_move_assignment_releases_target() {
    l05_support::reset_model();
    {
        l05::IntBuffer source{1, 2};
        l05::IntBuffer target{3, 4};
        target = std::move(source);
        check(l05_support::live_count() == 1, "move assignment should release old target storage");
        check(source.empty(), "move assignment leaves source empty by this exercise contract");
        check(target.get(0) == 1 && target.get(1) == 2, "move assignment transfers values");
        target = std::move(target);
        check(l05_support::live_count() == 1, "self move should not release live storage");
    }
    check(l05_support::live_count() == 0, "move assignment releases all storage");
    check(l05_support::invalid_releases() == 0, "move assignment has no invalid release");
}

} // namespace

int main() {
    check_type_contract();
    check_deep_copy();
    check_copy_assignment_releases_target();
    check_move_transfers();
    check_move_assignment_releases_target();
}
