#include <check.hpp>

#include <concepts>
#include <iostream>
#include <string_view>
#include <type_traits>

namespace c04_l06_observation {

struct Named {
    std::string_view name;
    int value;
};

template<class T>
concept simple_name = requires(T field) {
    field.name;
};

template<class T>
concept typed_name = requires {
    typename T::name_type;
};

template<class T>
concept compound_name = requires(T field) {
    { field.name } -> std::convertible_to<std::string_view>;
};

template<class T>
concept nested_small = requires {
    requires sizeof(T) <= 32;
};

template<class T>
concept atom_a = sizeof(T) > 1;

template<class T>
concept atom_b = sizeof(T) > 1;

template<class T>
concept larger_than_two = sizeof(T) > 2;

template<class T>
    requires atom_a<T>
int rank(T) {
    return 1;
}

template<class T>
    requires (atom_a<T> && larger_than_two<T>)
int rank(T) {
    return 2;
}

} // namespace c04_l06_observation

int main() {
    using c04_l06_observation::Named;

    check(c04_l06_observation::simple_name<Named>, "simple requirement checks expression formation");
    check(!c04_l06_observation::typed_name<Named>, "type requirement checks nested type names");
    check(c04_l06_observation::compound_name<Named>, "compound requirement checks conversion constraint");
    check(c04_l06_observation::nested_small<Named>, "nested requirement checks an extra boolean condition");

    check(c04_l06_observation::atom_a<int> == c04_l06_observation::atom_b<int>,
        "two concepts can be logically equal for a type");

    check(c04_l06_observation::rank(0) == 2,
        "same-parameter overloads can be ordered by constraint subsumption");

    std::cout << "L06_constraints observation OK\n";
}
