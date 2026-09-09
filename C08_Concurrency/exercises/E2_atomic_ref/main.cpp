#include "concurrency_study/exercise_check.hpp"
#include <atomic>
#include <iostream>

struct alignas(std::atomic_ref<int>::required_alignment) cell { int value = 0; };
int main() {
    cell item;
    {
        std::atomic_ref<int> ref(item.value);
        ref.store(123);
        // TODO Part 1: apply this access phase to array elements and parallel increments.
        cs::check(ref.load() == 123, "access through ref while alive");
    }
    // TODO Part 2: explain why ordinary access becomes legal only after ALL refs die.
    cs::check(item.value == 123, "ordinary phase");
    std::cout << "Starter: aligned reference and ordinary access phases checked\n";
}
