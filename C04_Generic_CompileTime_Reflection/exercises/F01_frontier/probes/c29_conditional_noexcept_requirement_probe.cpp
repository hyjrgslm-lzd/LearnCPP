#include <concepts>
#include <iostream>

#if defined(C04_HAS_CONDITIONAL_NOEXCEPT_REQUIREMENT) && C04_HAS_CONDITIONAL_NOEXCEPT_REQUIREMENT
#define C04_TRY_CONDITIONAL_NOEXCEPT_REQUIREMENT 1
#elif defined(C04_FORCE_CONDITIONAL_NOEXCEPT_REQUIREMENT_PROBE)
#define C04_TRY_CONDITIONAL_NOEXCEPT_REQUIREMENT 1
#elif defined(__cpp_concepts) && __cpp_concepts >= 202606L
#define C04_TRY_CONDITIONAL_NOEXCEPT_REQUIREMENT 1
#endif
#ifndef C04_TRY_CONDITIONAL_NOEXCEPT_REQUIREMENT
#define C04_TRY_CONDITIONAL_NOEXCEPT_REQUIREMENT 0
#endif

#if C04_TRY_CONDITIONAL_NOEXCEPT_REQUIREMENT
struct Throws {
    int operator()() { return 1; }
};

struct NoThrow {
    int operator()() noexcept { return 2; }
};

template<class F, bool RequiredNoexcept>
concept ConditionallyCallable = requires(F f) {
    { f() } noexcept(RequiredNoexcept) -> std::same_as<int>;
};

#if defined(C04_P3822_NEGATIVE_NON_BOOL_CONDITION)
template<class F>
concept BadNoexceptCondition = requires(F f) {
    { f() } noexcept(F{}) -> std::same_as<int>;
};
static_assert(BadNoexceptCondition<NoThrow>);
#endif
#endif

int main() {
    std::cout << "probe=c29_conditional_noexcept_requirement header=na macro="
#if defined(__cpp_concepts)
              << __cpp_concepts
#else
              << 0
#endif
              << " body=" << C04_TRY_CONDITIONAL_NOEXCEPT_REQUIREMENT << "\n";
#if !C04_TRY_CONDITIONAL_NOEXCEPT_REQUIREMENT
    std::cout << "SKIP conditional noexcept compound requirement: compile probe did not prove P3822R2 behavior\n";
    return 77;
#else
    static_assert(ConditionallyCallable<Throws, false>);
    static_assert(!ConditionallyCallable<Throws, true>);
    static_assert(ConditionallyCallable<NoThrow, false>);
    static_assert(ConditionallyCallable<NoThrow, true>);
    std::cout << "PASS conditional noexcept compound requirement\n";
    return 0;
#endif
}
