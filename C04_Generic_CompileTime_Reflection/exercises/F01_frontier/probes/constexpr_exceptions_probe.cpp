#include <iostream>
#include <optional>

#if (defined(__cpp_constexpr_exceptions) && __cpp_constexpr_exceptions >= 202411L) || defined(C04_FORCE_CONSTEXPR_EXCEPTIONS_PROBE)
#define C04_HAS_CONSTEXPR_EXCEPTIONS 1
#endif
#ifndef C04_HAS_CONSTEXPR_EXCEPTIONS
#define C04_HAS_CONSTEXPR_EXCEPTIONS 0
#endif

#if C04_HAS_CONSTEXPR_EXCEPTIONS
struct DivideByZero {};

constexpr int checked_divide(int value, int by) {
    if (by == 0) {
        throw DivideByZero{};
    }
    return value / by;
}

constexpr std::optional<int> try_divide(int value, int by) {
    try {
        return checked_divide(value, by);
    } catch (DivideByZero) {
        return std::nullopt;
    }
}
#endif

int main() {
    std::cout << "probe=constexpr_exceptions header=na macro=" << C04_HAS_CONSTEXPR_EXCEPTIONS
              << " body=" << C04_HAS_CONSTEXPR_EXCEPTIONS << "\n";
#if !C04_HAS_CONSTEXPR_EXCEPTIONS
    std::cout << "SKIP constexpr exceptions: __cpp_constexpr_exceptions < 202411L\n";
    return 77;
#else
    static_assert(try_divide(8, 2).value() == 4);
    static_assert(!try_divide(8, 0).has_value());
    std::cout << "PASS constexpr throw/catch\n";
    return 0;
#endif
}
