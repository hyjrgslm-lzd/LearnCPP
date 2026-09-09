#include <check.hpp>

#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

namespace {

template<class... Fs>
struct Overload : Fs... {
    using Fs::operator()...;
};

template<class... Fs>
Overload(Fs...) -> Overload<Fs...>;

struct Width {
    int value = 0;
};

struct Height {
    int value = 0;
};

struct Rectangle {
    int width = 0;
    int height = 0;
};

struct Circle {
    int radius = 0;
};

struct ThrowingAlternative {
    ThrowingAlternative() { throw std::runtime_error("planned variant construction failure"); }
};

struct NoDefault {
    explicit NoDefault(int value) : value(value) {}
    int value = 0;
};

using Shape = std::variant<Rectangle, Circle>;
using Dimension = std::variant<Width, Height>;

int area_like(const Shape& shape)
{
    return std::visit(Overload{
        [](const Rectangle& r) { return r.width * r.height; },
        [](const Circle& c) { return c.radius * c.radius; },
    }, shape);
}

std::string relation(const Dimension& lhs, const Dimension& rhs)
{
    return std::visit(Overload{
        [](Width, Width) { return std::string("width-width"); },
        [](Width, Height) { return std::string("width-height"); },
        [](Height, Width) { return std::string("height-width"); },
        [](Height, Height) { return std::string("height-height"); },
    }, lhs, rhs);
}

void check_monostate_and_access()
{
    std::variant<std::monostate, Rectangle, Circle> state;
    check(std::holds_alternative<std::monostate>(state), "monostate makes default state explicit");

    state.emplace<Rectangle>(3, 4);
    check(std::get<Rectangle>(state).width == 3, "get reads known current alternative");
    check(std::get_if<Circle>(&state) == nullptr, "get_if returns null for inactive alternative");

    bool threw = false;
    try {
        (void)std::get<Circle>(state);
    } catch (const std::bad_variant_access&) {
        threw = true;
    }
    check(threw, "get throws for inactive alternative");
}

void check_duplicate_type_and_strong_type()
{
    std::variant<int, int> repeated{std::in_place_index<1>, 7};
    check(repeated.index() == 1, "duplicate type variant is selected by index");
    check(std::get<1>(repeated) == 7, "duplicate type variant is read by index");

    Dimension dimension = Width{8};
    check(std::holds_alternative<Width>(dimension), "strong types make business meaning unique");
}

void check_visit_dispatch()
{
    Shape rectangle = Rectangle{3, 4};
    Shape circle = Circle{5};
    check(area_like(rectangle) == 12, "visit dispatches rectangle branch");
    check(area_like(circle) == 25, "visit dispatches circle branch");

    Dimension width = Width{1};
    Dimension height = Height{2};
    check(relation(width, height) == "width-height", "multi variant visit dispatches active combination");
}

void check_valueless_and_move()
{
    std::variant<int, ThrowingAlternative> value{42};
    bool threw = false;
    try {
        value.emplace<ThrowingAlternative>();
    } catch (const std::runtime_error&) {
        threw = true;
    }

    check(threw, "variant emplace reports construction failure");
    check(value.valueless_by_exception(), "failed emplace can leave variant valueless by exception");

    std::variant<std::string, int> source{std::string("abc")};
    std::variant<std::string, int> target{std::move(source)};
    check(std::holds_alternative<std::string>(source), "moved-from variant still has an active alternative");
    check(std::get<std::string>(target) == "abc", "move constructs target from active alternative");
}

} // namespace

int main()
{
    static_assert(!std::is_default_constructible_v<std::variant<NoDefault, Circle>>,
        "first alternative controls default construction");

    check_monostate_and_access();
    check_duplicate_type_and_strong_type();
    check_visit_dispatch();
    check_valueless_and_move();
}
