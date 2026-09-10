#include <check.hpp>
#include <iostream>
#include <type_traits>

template<class T> struct provider { using type = T; };
struct explosive;

template<bool Choose, class Then, class Else>
struct lazy;
template<class Then, class Else> struct lazy<true, Then, Else> { using type = typename Then::type; };
template<class Then, class Else> struct lazy<false, Then, Else> { using type = typename Else::type; };

int main() {
    check((std::is_same_v<typename lazy<true, provider<int>, explosive>::type, int>),
        "lazy branch leaves the unselected provider untouched");
    std::cout << "A04 eager counterexample baseline OK\n";
}
