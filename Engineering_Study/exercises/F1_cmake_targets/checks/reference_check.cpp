#include <check.hpp>
#include <f1_math/math.hpp>

int main() {
    check(f1_math::add_scaled(20, 1) == 42, "PUBLIC include and PRIVATE implementation should compose");
    check(f1_math::add_scaled(0, 0) == 0, "zero inputs must keep the contract");
    check(f1_math::add_scaled(-2, 3) == 2, "both inputs participate in the result");
#ifdef F1_REQUIRE_EXPLICIT_RESULT
    check(true, "INTERFACE definition reached the consumer");
#else
    check(false, "missing transitive INTERFACE definition");
#endif
}
