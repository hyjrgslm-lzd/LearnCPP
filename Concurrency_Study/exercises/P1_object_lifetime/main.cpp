#include "concurrency_study/exercise_check.hpp"
#include <functional>
#include <iostream>
#include <memory>
#include <tuple>
#include <utility>

int main() {
    int value = 10;
    auto snapshot = [value] { return value; };
    auto borrowed = [&value] { return value; };
    value = 20;
    std::cout << "snapshot=" << snapshot() << ", borrowed=" << borrowed() << '\n';
    cs::check(snapshot() == 10 && borrowed() == 20, "capture baseline");
    // TODO Part 1: add constructor/destructor records; verify reverse destruction.
    // TODO Part 2: move a unique_ptr into a closure; check both owners.
    // TODO Part 3: call a member through std::invoke and std::ref.
    // TODO Part 4: throw inside a resource-owning scope; check cleanup afterward.
    auto arguments = std::make_tuple(value, 2);
    value = 30;
    cs::check(std::apply([](int n, int factor) { return n * factor; }, arguments) == 40,
              "saved value arguments are a snapshot");
    std::cout << "saved arguments=(20,2), later apply=40, original=30\n";
    // TODO Part 5: compare make_tuple, tie, and make_tuple(std::ref(value)).
    // TODO Part 6: save callable/tuple in a closure; apply move-only arguments.
}
