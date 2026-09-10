#include <check.hpp>

#include <concepts>
#include <iostream>
#include <type_traits>

namespace c04_l05_observation {

struct exact {};
struct converted {};
struct pointer_template {};
struct const_pointer_template {};

converted choose(long) {
    return {};
}

template<class T>
exact choose(T) {
    return {};
}

template<class T>
pointer_template select_pointer(T*) {
    return {};
}

template<class T>
const_pointer_template select_pointer(const T*) {
    return {};
}

template<class T>
struct classifier {
    static constexpr int value = 0;
};

template<class T>
struct classifier<T*> {
    static constexpr int value = 1;
};

template<class T>
concept has_size_immediate = requires(T& object) {
    object.size();
};

struct NoSize {};

} // namespace c04_l05_observation

int main() {
    auto selected = c04_l05_observation::choose(1);
    check((std::same_as<decltype(selected), c04_l05_observation::exact>),
        "function template wins when it has the better conversion");

    const int const_value = 0;
    auto pointer_selected = c04_l05_observation::select_pointer(&const_value);
    check((std::same_as<decltype(pointer_selected), c04_l05_observation::const_pointer_template>),
        "function template partial ordering selects the const pointer template");

    check(c04_l05_observation::classifier<int*>::value == 1,
        "class template partial specialization handles pointer cases");

    check(!c04_l05_observation::has_size_immediate<c04_l05_observation::NoSize>,
        "requires expression keeps the missing size expression in the immediate context");

    std::cout << "L05_overload observation OK\n";
}
