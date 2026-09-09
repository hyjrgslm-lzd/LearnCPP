#include <owner.hpp>
#include <check.hpp>

int main() {
    l07::reset_resource_model({});
    l07::two_resource_owner first;
    check(l07::resource_counters().first_alive == 1, "first owner live before second owner");
    l07::two_resource_owner second(l07::AcquisitionPlan{});
    check(l07::resource_counters().first_alive == 2, "second constructor must not reset first live owner");
    check(l07::resource_counters().second_alive == 2, "second constructor must not reset second live owner");
}
