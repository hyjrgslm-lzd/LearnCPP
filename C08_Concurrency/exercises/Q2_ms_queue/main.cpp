#include "concurrency_study/queue_linked.hpp"
#include "concurrency_study/exercise_check.hpp"
#include <iostream>
int main() {
    cs::queue_lab::ms_queue<int> queue;
    queue.try_push(10); queue.try_push(20);
    int value = 0;
    cs::check(queue.try_pop(value) && value == 10, "MS FIFO");
    std::cout << "first pop=" << value << "; destructor releases remaining live and retired nodes\n";
}
