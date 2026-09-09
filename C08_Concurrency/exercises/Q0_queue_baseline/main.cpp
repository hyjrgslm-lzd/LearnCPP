#include "concurrency_study/queue_baseline.hpp"

#include <iostream>

int main() {
    cs::queue_lab::mutex_queue<int> queue(2);
    int value = -1;
    std::cout << std::boolalpha;
    std::cout << "empty pop: " << queue.try_pop(value) << '\n';
    std::cout << "push 10: " << queue.try_push(10) << '\n';
    std::cout << "push 20: " << queue.try_push(20) << '\n';
    std::cout << "push 30 when full: " << queue.try_push(30) << '\n';
    // TODO Part 1: predict the two returned values and the final empty result.
    while (queue.try_pop(value)) std::cout << "pop: " << value << '\n';
    // TODO Part 2: change capacity to 1; explain which operations fail and why.
    // TODO Part 3: implement the same interface locally, then use the reference
    // scenarios to check FIFO, unchanged output on empty, and concurrent IDs.
}
