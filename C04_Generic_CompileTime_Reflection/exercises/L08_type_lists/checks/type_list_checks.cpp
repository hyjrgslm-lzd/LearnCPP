#include <check.hpp>
#include <type_list_tools.hpp>

#include <exception>
#include <iostream>
#include <string>
#include <type_traits>

template<class T>
using add_const_ref = const T&;

template<class T>
struct is_integral_trait : std::is_integral<T> {};

struct ToText {
    std::string operator()(int) const;
    void operator()(double) const;
};

struct IntOnly {
    int operator()(int) const;
};

template<class T>
struct type_provider {
    using type = T;
};

struct ExplosiveProvider;

template<class F, class List>
concept transformable = requires {
    typename c04::transform_completion_signatures_t<F, List>;
};

int main() {
    using input = c04::type_list<int, double, int, char>;
    check((std::is_same_v<c04::map_t<add_const_ref, c04::type_list<int, double>>, c04::type_list<const int&, const double&>>),
          "map_t applies a unary template to each element");
    check((std::is_same_v<c04::filter_t<is_integral_trait, input>, c04::type_list<int, int, char>>),
          "filter_t keeps only matching element types");
    check((std::is_same_v<c04::concat_t<c04::type_list<int>, c04::type_list<double, char>>, c04::type_list<int, double, char>>),
          "concat_t appends two type lists");
    check((std::is_same_v<c04::unique_t<input>, c04::type_list<int, double, char>>),
          "unique_t keeps the first copy of repeated types");
    check((std::is_same_v<c04::lazy_type_t<true, type_provider<int>, ExplosiveProvider>, int>),
          "unselected lazy branch must not be instantiated");

    using sigs = c04::type_list<
        c04::value_sig<int>,
        c04::value_sig<double>,
        c04::value_sig<int>,
        c04::error_sig<std::exception_ptr>,
        c04::stopped_sig>;
    using transformed = c04::transform_completion_signatures_t<ToText, sigs>;
    using expected = c04::type_list<c04::value_sig<std::string>, c04::value_sig<>, c04::error_sig<std::exception_ptr>, c04::stopped_sig>;
    check((std::is_same_v<transformed, expected>),
          "error and stopped signatures must survive value transform");
    check((std::is_same_v<c04::transform_completion_signatures_t<ToText, c04::type_list<>>, c04::type_list<>>),
          "empty signature list remains empty");
    check(!transformable<IntOnly, c04::type_list<c04::value_sig<std::string>>>,
          "uninvocable value signature must be rejected by constraints");
    std::cout << "L08_type_lists checks OK\n";
}
