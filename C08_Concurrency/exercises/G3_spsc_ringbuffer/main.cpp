#include "concurrency_study/queue_versions.hpp"
#include "concurrency_study/exercise_check.hpp"
#include <iostream>
int main() {
    cs::queue_lab::spsc_ring<int> queue(2);
    cs::check(queue.try_push(10) && queue.try_push(20), "usable capacity is two");
    cs::check(!queue.try_push(30), "full");
    int value = 0;
    cs::check(queue.try_pop(value) && value == 10, "FIFO");
    std::cout << "first pop=" << value << "; compare cached variant in solution.cpp\n";
}
