#include <check.hpp>

#include <iostream>
#include <type_traits>

template<class... Ts>
struct list {};

template<class A, class B>
struct concat;

template<class... A, class... B>
struct concat<list<A...>, list<B...>> {
    using type = list<A..., B...>;
};

template<class T>
struct provider {
    using type = T;
};

struct explosive;

template<bool ChooseThen, class Then, class Else>
struct lazy_type;

template<class Then, class Else>
struct lazy_type<true, Then, Else> {
    using type = typename Then::type;
};

template<class Then, class Else>
struct lazy_type<false, Then, Else> {
    using type = typename Else::type;
};

int main() {
    using joined = concat<list<int>, list<double, char>>::type;
    check((std::is_same_v<joined, list<int, double, char>>), "type_list algorithms are type computations");
    using selected = lazy_type<true, provider<int>, explosive>::type;
    check((std::is_same_v<selected, int>), "unselected lazy branch is not instantiated");
    std::cout << "L08 type-list observation OK\n";
}
