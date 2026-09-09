#include <memory>

int main() {
    auto owner = std::make_unique<int[]>(4);
    owner[1] = 23;
    volatile int* borrowed = owner.get();
    owner.reset();
    return borrowed[1];
}
