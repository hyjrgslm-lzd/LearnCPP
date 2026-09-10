#include <iostream>
#include <type_traits>

#if defined(C04_FORCE_FOLD_CONSTRAINTS_PROBE) || (defined(C04_HAS_FOLD_EXPANDED_CONSTRAINTS) && C04_HAS_FOLD_EXPANDED_CONSTRAINTS)
#define C04_TRY_FOLD_CONSTRAINTS 1
#endif
#ifndef C04_TRY_FOLD_CONSTRAINTS
#define C04_TRY_FOLD_CONSTRAINTS 0
#endif

#if C04_TRY_FOLD_CONSTRAINTS
template<class T>
concept Basic = requires { typename T::value_type; };

template<class T>
concept Refined = Basic<T> && true;

template<class... Ts>
    requires (Basic<Ts> && ...)
constexpr int rank(Ts...) {
    return 1;
}

template<class... Ts>
    requires (Refined<Ts> && ...)
constexpr int rank(Ts...) {
    return 2;
}

struct WithValueType {
    using value_type = int;
};
#endif

int main() {
    std::cout << "probe=fold_constraints header=na macro="
#if defined(__cpp_fold_expressions)
              << __cpp_fold_expressions
#else
              << 0
#endif
              << " body=" << C04_TRY_FOLD_CONSTRAINTS << "\n";
#if !C04_TRY_FOLD_CONSTRAINTS
    std::cout << "SKIP fold expanded constraints: compile probe did not prove P2963R3 behavior\n";
    return 77;
#else
    static_assert(rank(WithValueType{}, WithValueType{}) == 2);
    std::cout << "PASS fold constraint behavior smoke check\n";
    return 0;
#endif
}
