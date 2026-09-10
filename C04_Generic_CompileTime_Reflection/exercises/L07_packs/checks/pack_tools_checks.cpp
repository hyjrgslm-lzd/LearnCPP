#include <check.hpp>
#include <pack_tools.hpp>

#include <array>
#include <iostream>
#include <type_traits>

template<class T>
struct add_pointer_alias {
    using type = T*;
};

template<class T>
using add_pointer_alias_t = typename add_pointer_alias<T>::type;

int main() {
    check(c04::count_types<int, double, char> == 3, "count_types reports the size of a type pack");
    check(c04::count_values<1, 'x', true> == 3, "count_values reports the size of a value pack");
    check(c04::all_true<>, "empty all fold identity must be true");
    check(c04::all_true<true, true>, "all_true accepts all true values");
    check(!c04::all_true<true, false>, "all_true rejects one false value");
    check(!c04::any_true<>, "empty any fold identity must be false");
    check(c04::any_true<false, true>, "any_true accepts one true value");
    check(c04::sum_values<> == 0, "empty sum uses explicit zero identity");
    check(c04::sum_values<1, 2, 3> == 6, "sum_values folds values");
    check(c04::left_subtract<1, 2, 3> == -6, "left binary fold keeps left association");
    check(c04::right_subtract<1, 2, 3> == 2, "right binary fold keeps right association");

    std::array<int, 3> order{};
    int next = 0;
    c04::call_in_order([&] { order[0] = next++; }, [&] { order[1] = next++; }, [&] { order[2] = next++; });
    check((order == std::array{0, 1, 2}), "comma fold must call arguments left to right");

    check(c04::constant<42>::value == 42, "auto NTTP stores integer value");
    check(c04::constant<'x'>::value == 'x', "auto NTTP stores char value");
    using named = c04::named_value<"id", 7>;
    check(named::name.value[0] == 'i' && named::name.value[1] == 'd', "fixed_string preserves string literal spelling");
    check(named::value == 7, "named_value carries NTTP payload");
    check((std::is_same_v<c04::apply_unary_template<add_pointer_alias_t, int>, int*>),
          "template template parameter applies unary template");
    std::cout << "L07_packs checks OK\n";
}
