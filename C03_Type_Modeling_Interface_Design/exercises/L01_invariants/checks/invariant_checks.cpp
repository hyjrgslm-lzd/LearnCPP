#include <bounded_int.hpp>

#include <check.hpp>

#include <stdexcept>

int main()
{
    l01::BoundedInt value{5, 0, 10};
    check(value.value() == 5, "constructor keeps valid value");
    check(value.min() == 0 && value.max() == 10, "bounds are observable");

    value.set(10);
    check(value.value() == 10, "upper bound is included");
    value.set(0);
    check(value.value() == 0, "lower bound is included");

    bool ctor_threw = false;
    try {
        (void)l01::BoundedInt{11, 0, 10};
    } catch (const std::exception&) {
        ctor_threw = true;
    }
    check(ctor_threw, "constructor rejects values outside range");

    bool bad_bounds_threw = false;
    try {
        (void)l01::BoundedInt{0, 10, 0};
    } catch (const std::exception&) {
        bad_bounds_threw = true;
    }
    check(bad_bounds_threw, "constructor rejects inverted bounds");

    bool set_threw = false;
    try {
        value.set(-1);
    } catch (const std::exception&) {
        set_threw = true;
    }
    check(set_threw, "set rejects values outside range");
    check(value.value() == 0, "failed set keeps old value");
}
