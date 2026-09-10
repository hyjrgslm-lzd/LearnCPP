#include <check.hpp>

#include <iostream>
#include <type_traits>
#include <utility>

namespace c04_l04_observation {

template<class T>
struct Base {
    int value() const noexcept { return 7; }

    template<class U>
    int convert() const noexcept {
        return static_cast<int>(sizeof(U));
    }

    using result_type = int;
};

template<class T>
struct Derived : Base<T> {
    int use_this() const noexcept {
        return this->value();
    }

    int use_template_keyword() const noexcept {
        return this->template convert<long>();
    }

    typename Base<T>::result_type use_typename() const noexcept {
        return this->value();
    }
};

namespace unsafe_cpo {

struct NoRoute {};

inline int recursion_depth = 0;

struct inspect_fn;
extern const inspect_fn inspect;

struct inspect_fn {
    template<class T>
    int operator()(T&& object) const {
        ++recursion_depth;
        if (recursion_depth > 1) {
            return 99;
        }
        return inspect(std::forward<T>(object));
    }
};

inline const inspect_fn inspect{};

template<class T>
concept accepted = requires(T&& object) {
    inspect(std::forward<T>(object));
};

} // namespace unsafe_cpo

} // namespace c04_l04_observation

int main() {
    c04_l04_observation::Derived<int> derived{};
    check(derived.use_this() == 7, "this-> makes the dependent base member lookup dependent");
    check(derived.use_template_keyword() == static_cast<int>(sizeof(long)),
        "template keyword disambiguates dependent member template calls");
    check((std::is_same_v<decltype(derived.use_typename()), int>),
        "typename disambiguates dependent nested type names");

    using c04_l04_observation::unsafe_cpo::NoRoute;
    check(c04_l04_observation::unsafe_cpo::accepted<NoRoute&>,
        "unisolated CPO accepts a no-route object through its own name");
    NoRoute object{};
    check(c04_l04_observation::unsafe_cpo::inspect(object) == 99,
        "recursion guard proves the call returns to the CPO object");

    std::cout << "L04_lookup observation OK\n";
}
