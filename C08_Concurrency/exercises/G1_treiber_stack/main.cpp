#include "concurrency_study/queue_linked.hpp"
#include "concurrency_study/exercise_check.hpp"
#include <iostream>
int main() {
    cs::queue_lab::treiber_stack<int> stack;
    stack.try_push(10); stack.try_push(20);
    int value = 0;
    cs::check(stack.try_pop(value) && value == 20, "predict LIFO");
    std::cout << "first pop=" << value << "; see solution.cpp for concurrent checks and HP cleanup\n";
}
