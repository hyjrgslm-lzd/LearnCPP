#include "capability_support.hpp"
#include <check.hpp>
#include <memory>
#include <string>
#include <version>

int main() {
#if defined(__cpp_lib_indirect)
    print_macro("__cpp_lib_indirect", __cpp_lib_indirect);
#else
    return skip("__cpp_lib_indirect not defined");
#endif
#if defined(__cpp_lib_polymorphic)
    print_macro("__cpp_lib_polymorphic", __cpp_lib_polymorphic);
#else
    return skip("__cpp_lib_polymorphic not defined");
#endif

#if !defined(__cpp_lib_indirect) || __cpp_lib_indirect < 202502L || \
    !defined(__cpp_lib_polymorphic) || __cpp_lib_polymorphic < 202502L
    return skip("std::indirect/std::polymorphic unavailable");
#else
    std::indirect<std::string> text{"abc"};
    auto copy = text;
    *copy = "xyz";
    check(*text == "abc" && *copy == "xyz", "indirect copies owned value");

    struct Shape {
        virtual ~Shape() = default;
        virtual int area() const = 0;
    };
    struct Square : Shape {
        int side;
        explicit Square(int s) : side{s} {}
        int area() const override { return side * side; }
    };
    std::polymorphic<Shape> shape{std::in_place_type<Square>, 6};
    auto shape_copy = shape;
    check(shape.area() == 36 && shape_copy.area() == 36, "polymorphic copies dynamic object");
    return 0;
#endif
}

