#include <any_shape.hpp>

#include <check.hpp>

#include <stdexcept>
#include <string>
#include <cstdint>
#include <utility>

namespace {

void check_empty_state()
{
    l11::AnyShape empty;
    check(!empty, "default AnyShape is empty");

    bool threw = false;
    try {
        static_cast<void>(empty.name());
    } catch (const std::logic_error&) {
        threw = true;
    }
    check(threw, "empty name throws");
}

void check_dispatch_and_const()
{
    const l11::AnyShape shape{l11::Rectangle{"rect", 3, 4}};
    check(static_cast<bool>(shape), "constructed AnyShape is not empty");
    check(shape.name() == "rect", "name dispatch reaches target");
    check(shape.dimensions() == l11::Dimensions{3, 4}, "dimensions dispatch reaches target");
}

void check_deep_copy()
{
    l11::AnyShape original{l11::Rectangle{"copy-me", 5, 6}};
    l11::AnyShape copy = original;

    check(copy.name() == original.name(), "copy keeps observable name");
    check(copy.dimensions() == original.dimensions(), "copy keeps observable dimensions");
    check(copy.target_address() != original.target_address(), "copy owns a separate target");
}

void check_move_and_self_assignment()
{
    l11::AnyShape shape{l11::Rectangle{"move-me", 7, 8}};
    const void* old_address = shape.target_address();

    l11::AnyShape moved = std::move(shape);
    check(!shape, "moved-from AnyShape is empty");
    check(moved.target_address() == old_address, "move transfers ownership");
    check(moved.name() == "move-me", "moved target remains usable");

    moved = moved;
    check(moved.name() == "move-me", "copy self-assignment keeps value");

    moved = std::move(moved);
    check(moved.name() == "move-me", "move self-assignment keeps value");
}

void check_copy_failure_keeps_target()
{
    l11::CountingShape::reset();
    {
        l11::AnyShape source{l11::CountingShape{"fragile", {9, 10}}};
        l11::AnyShape target{l11::Rectangle{"stable", 1, 2}};

        l11::CountingShape::reset_copy_plan();
        l11::CountingShape::throw_on_copy(1);

        bool threw = false;
        try {
            target = source;
        } catch (const std::runtime_error&) {
            threw = true;
        }

        check(threw, "copy assignment reports target copy failure");
        check(static_cast<bool>(target), "copy failure keeps target unchanged");
        check(target.name() == "stable", "copy failure keeps old target name");
        check(target.dimensions() == l11::Dimensions{1, 2}, "copy failure keeps old target dimensions");
    }
    check(l11::CountingShape::live_count() == 0, "copy failure releases all counting targets");
}

void check_destructor_counts()
{
    l11::CountingShape::reset();
    {
        l11::AnyShape tracked{l11::CountingShape{"tracked", {2, 3}}};
        check(l11::CountingShape::live_count() == 1, "erased object owns one target");
    }
    check(l11::CountingShape::live_count() == 0, "AnyShape destructor destroys erased target");
    check(l11::CountingShape::destroyed_count() >= 2, "temporary and erased targets were destroyed");
}

void check_over_aligned_target()
{
    l11::OverAlignedShape::reset();
    {
        l11::AnyShape shape{l11::OverAlignedShape{"wide", {11, 12}}};
        const auto address = reinterpret_cast<std::uintptr_t>(shape.target_address());
        check(address % alignof(l11::OverAlignedShape) == 0, "over-aligned target keeps required alignment");
        check(shape.name() == "wide", "over-aligned target dispatches name");
        check(shape.dimensions() == l11::Dimensions{11, 12}, "over-aligned target dispatches dimensions");

        l11::AnyShape copy = shape;
        const auto copy_address = reinterpret_cast<std::uintptr_t>(copy.target_address());
        check(copy_address % alignof(l11::OverAlignedShape) == 0, "over-aligned clone keeps required alignment");
        check(copy.target_address() != shape.target_address(), "over-aligned clone owns a separate target");
        check(copy.name() == "wide", "over-aligned clone dispatches name");
        check(l11::OverAlignedShape::live_count() == 2, "over-aligned original and clone are both live");
    }
    check(l11::OverAlignedShape::live_count() == 0, "over-aligned erased targets are destroyed");
}

} // namespace

int main()
{
    check_empty_state();
    check_dispatch_and_const();
    check_deep_copy();
    check_move_and_self_assignment();
    check_copy_failure_keeps_target();
    check_destructor_counts();
    check_over_aligned_target();
}
