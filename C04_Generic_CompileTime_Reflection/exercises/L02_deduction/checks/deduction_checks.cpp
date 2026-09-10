#include <check.hpp>
#include <deduction_probe.hpp>

#include <concepts>
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

namespace l02_checks {

template<class Token>
using token_type_t = typename Token::type;

void sample_function(int) {}

void run_all() {
    int value = 1;
    const int const_value = 2;
    int data[4]{1, 2, 3, 4};

    using by_value_int = token_type_t<decltype(l02::by_value_token(value))>;
    using by_value_const = token_type_t<decltype(l02::by_value_token(const_value))>;
    using by_value_array = token_type_t<decltype(l02::by_value_token(data))>;
    using by_value_function = token_type_t<decltype(l02::by_value_token(sample_function))>;

    check((std::same_as<by_value_int, int>), "by-value keeps plain int");
    check((std::same_as<by_value_const, int>), "by-value drops top-level const");
    check((std::same_as<by_value_array, int*>), "by-value decays array to pointer");
    check((std::same_as<by_value_function, void (*)(int)>), "by-value decays function to pointer");

    using by_ref_int = token_type_t<decltype(l02::by_ref_token(value))>;
    using by_ref_const = token_type_t<decltype(l02::by_ref_token(const_value))>;
    check((std::same_as<by_ref_int, int>), "T& sees non-const lvalue element type");
    check((std::same_as<by_ref_const, const int>), "T& preserves const in T");

    using array_result = decltype(l02::array_ref_token(data));
    check((std::same_as<typename array_result::element_type, int>), "array reference keeps element type");
    check(array_result::extent == 4, "array reference must preserve extent");

    int& kept_lvalue = l02::identity_decltype_auto(value);
    check(&kept_lvalue == &value, "decltype(auto) identity keeps lvalue reference");
    kept_lvalue = 9;
    check(value == 9, "identity write goes through original object");

    using rvalue_result = decltype(l02::identity_decltype_auto(std::string{"x"}));
    check((std::same_as<rvalue_result, std::string&&>), "decltype(auto) identity keeps rvalue reference type");

    using frozen = token_type_t<decltype(l02::freeze_as<long long>(3))>;
    check((std::same_as<frozen, long long>), "type_identity freezes T and accepts explicit target");
}

} // namespace l02_checks

int main() {
    l02_checks::run_all();
    std::cout << "L02_deduction checks OK\n";
}
