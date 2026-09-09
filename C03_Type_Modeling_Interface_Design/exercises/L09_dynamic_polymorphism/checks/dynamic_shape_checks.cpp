#include <shape.hpp>

#include <check.hpp>

#include <memory>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

void check_invalid_dimensions()
{
    bool rejected = false;
    try {
        l09::Rectangle bad{0, 4};
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected, "rectangle rejects non-positive dimensions");

    rejected = false;
    try {
        l09::Circle too_large{std::numeric_limits<int>::max() / 2 + 1};
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected, "circle rejects dimensions that would overflow");
}

void check_base_dispatch()
{
    std::unique_ptr<l09::Shape> shape = std::make_unique<l09::Rectangle>(3, 4);
    check(shape->name() == "rectangle", "virtual name dispatch uses dynamic type");
    check(shape->dimensions() == l09::Dimensions{3, 4}, "rectangle dimensions keep constructor values");

    shape = std::make_unique<l09::Circle>(5);
    check(shape->name() == "circle", "circle name dispatch uses dynamic type");
    check(shape->dimensions() == l09::Dimensions{10, 10}, "circle dimensions are diameter bounds");
}

void check_clone_dynamic_type()
{
    l09::Rectangle rectangle{3, 4};
    std::unique_ptr<l09::Shape> rectangle_copy = rectangle.clone();
    check(rectangle_copy != nullptr, "rectangle clone returns an owning pointer");
    check(rectangle_copy.get() != &rectangle, "clone owns an independent object");
    check(dynamic_cast<l09::Rectangle*>(rectangle_copy.get()) != nullptr, "rectangle clone preserves dynamic type");
    check(rectangle_copy->name() == rectangle.name(), "rectangle clone keeps observable name");
    check(rectangle_copy->dimensions() == rectangle.dimensions(), "rectangle clone keeps observable dimensions");

    l09::Circle circle{6};
    std::unique_ptr<l09::Shape> copy = circle.clone();

    check(copy != nullptr, "clone returns an owning pointer");
    check(copy.get() != &circle, "clone owns an independent object");
    check(dynamic_cast<l09::Circle*>(copy.get()) != nullptr, "clone preserves dynamic type");
    check(copy->name() == circle.name(), "clone keeps observable name");
    check(copy->dimensions() == circle.dimensions(), "clone keeps observable dimensions");
}

void check_destructor_through_base()
{
    check(l09::Rectangle::alive_count() == 0, "rectangle live counter starts clean");
    {
        std::unique_ptr<l09::Shape> shape = std::make_unique<l09::Rectangle>(2, 3);
        check(l09::Rectangle::alive_count() == 1, "rectangle live counter observes construction");
    }
    check(l09::Rectangle::alive_count() == 0, "base deletion runs derived destructor");
}

} // namespace

int main()
{
    check_invalid_dimensions();
    check_base_dispatch();
    check_clone_dynamic_type();
    check_destructor_through_base();
}
