#include <array>
#include <iostream>

int main() {
#if defined(__cpp_expansion_statements) && __cpp_expansion_statements >= 202506L
#define C04_HAS_EXPANSION_STATEMENTS 1
#elif defined(C04_FORCE_EXPANSION_STATEMENTS_PROBE)
#define C04_HAS_EXPANSION_STATEMENTS 1
#else
#define C04_HAS_EXPANSION_STATEMENTS 0
#endif
    std::cout << "probe=expansion_statement header=na macro=" << C04_HAS_EXPANSION_STATEMENTS
              << " body=" << C04_HAS_EXPANSION_STATEMENTS << "\n";
#if !C04_HAS_EXPANSION_STATEMENTS
    std::cout << "SKIP expansion statements: __cpp_expansion_statements < 202506L\n";
    return 77;
#else
    constexpr std::array values{1, 2, 3};
    int sum = 0;
    template for (constexpr int value : values) {
        sum += value;
    }
    if (sum != 6) {
        std::cout << "FAIL expansion statement sum\n";
        return 1;
    }
    std::cout << "PASS expansion statements\n";
    return 0;
#endif
}
