#include <check.hpp>
#include <quantity_templates.hpp>

#include <concepts>
#include <iostream>
#include <type_traits>

namespace l01_checks {

template<class A, class B>
concept addable = requires(A a, B b) {
    l01::add_same_unit(a, b);
};

template<class A, class B>
using add_result_t = decltype(l01::add_same_unit(std::declval<A>(), std::declval<B>()));

void run_all() {
    using meters_i = l01::quantity<int, l01::meters>;
    using meters_ll = l01::quantity<long long, l01::meters>;
    using km_i = l01::quantity<int, l01::kilometers>;

    check((std::same_as<typename meters_i::value_type, int>), "quantity exposes value_type");
    check((std::same_as<typename meters_i::unit_type, l01::meters>), "quantity exposes unit_type");
    check((std::same_as<l01::quantity_value_t<meters_ll>, long long>), "quantity_value_t reads dependent value_type");

    check(l01::unit_scale_v<l01::centimeters> == 1, "centimeters scale specialization is used");
    check(l01::unit_scale_v<l01::meters> == 100, "meters scale specialization is used");
    check(l01::unit_scale_v<l01::kilometers> == 100000, "kilometers scale specialization is used");

    check(l01::to_base(km_i{3}) == 300000, "to_base uses unit_scale_v specialization");
    check(l01::to_base(meters_i{7}) == 700, "to_base converts meters to centimeter base");

    check(!addable<meters_i, km_i>, "different units must not be addable");
    check(l01::same_unit_v<meters_i, meters_ll>, "same_unit_v accepts same unit with different value types");
    check(!l01::same_unit_v<meters_i, km_i>, "same_unit_v rejects different units");
    check(addable<meters_i, meters_ll>, "same units must be addable");

    auto sum = l01::add_same_unit(meters_i{2}, meters_ll{5});
    check(sum.value == 7, "add_same_unit adds values");
    check((std::same_as<decltype(sum), l01::quantity<long long, l01::meters>>), "add_same_unit preserves unit and result value type");
}

} // namespace l01_checks

int main() {
    l01_checks::run_all();
    std::cout << "L01_templates quantity checks OK\n";
}
