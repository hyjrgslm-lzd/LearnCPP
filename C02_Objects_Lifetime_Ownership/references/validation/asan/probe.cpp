#include <memory>
#include "check.hpp"

int main() {
    auto owner = std::make_unique<int[]>(4);
    owner[1] = 23;
#ifdef C02_INTENTIONAL_FAULT
    // Deliberate UB, enabled only in the isolated diagnostic process.
    volatile int* borrowed = owner.get();
    owner.reset();
    const int observed = borrowed[1];
    return observed == 23 ? 0 : 1;
#else
    check(owner[1] == 23, "C02_ASAN_SAFE_VALUE");
    owner.reset();
    check(!owner, "C02_ASAN_SAFE_RELEASE");
    std::cout << "C02_ASAN_SAFE_OK\n";
#endif
}
