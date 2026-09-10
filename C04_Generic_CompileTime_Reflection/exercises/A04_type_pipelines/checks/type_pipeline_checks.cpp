#include <check.hpp>
#include <type_pipelines.hpp>

#include <exception>
#include <iostream>
#include <string>
#include <type_traits>
#include <variant>

struct ThrowingText { std::string operator()(int) const; };
struct NoThrowVoid { void operator()(double) const noexcept; };
struct PointerOnly { int operator()(char*) const; };
template<class T> struct provider { using type = T; };
struct Explosive;

int main() {
    using namespace c04_pipeline;
    check((std::is_same_v<zip_t<type_list<int, double>, type_list<char, bool>>,
        type_list<type_list<int, char>, type_list<double, bool>>>), "zip preserves pair order");
    check(!zipable<type_list<int>, type_list<char, bool>>, "mismatched zip is a false boundary");
    check((std::is_same_v<flatten_t<type_list<int, type_list<double, char>, bool>>,
        type_list<int, double, char, bool>>), "flatten recursively expands nested type_list elements");
    check(!flattenable<int>, "flatten requires a type_list root");
    check((std::is_same_v<cartesian_product_t<>, type_list<type_list<>>>), "cartesian product has an empty-input unit");
    check((std::is_same_v<cartesian_product_t<type_list<int>, type_list<>>, type_list<>>), "empty input list makes product empty");
    check((std::is_same_v<cartesian_product_t<type_list<int, double>, type_list<char, bool>>,
        type_list<type_list<int, char>, type_list<int, bool>, type_list<double, char>, type_list<double, bool>>>),
        "cartesian product uses left-major order");
    check((std::is_same_v<variant_product_t<std::variant<int, double>, std::variant<char, bool>>,
        type_list<type_list<int, char>, type_list<int, bool>, type_list<double, char>, type_list<double, bool>>>),
        "variant products reuse the same type-list contract");
    check((std::is_same_v<lazy_type_t<true, provider<int>, Explosive>, int>), "lazy branch does not instantiate the unselected provider");

    using sigs = type_list<value_sig<int>, value_sig<double>, error_sig<std::logic_error>, stopped_sig>;
    using transformed = transform_completion_signatures_t<ThrowingText, type_list<value_sig<int>, error_sig<std::logic_error>, stopped_sig>>;
    check((std::is_same_v<transformed,
        type_list<value_sig<std::string>, error_sig<std::exception_ptr>, error_sig<std::logic_error>, stopped_sig>>),
        "error/stopped and throwing value channels survive");
    check((std::is_same_v<transform_completion_signatures_t<NoThrowVoid, type_list<value_sig<double>>>, type_list<value_sig<>>>),
        "nothrow void value channel becomes empty value without exception channel");
    check(!transformable<PointerOnly, sigs>, "uninvocable value signature is a false boundary");
    std::cout << "A04 type pipeline checks OK\n";
}
