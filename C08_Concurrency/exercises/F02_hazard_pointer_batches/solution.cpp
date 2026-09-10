#ifndef CS_HAS_STD_HAZARD_POINTER_BATCH
#define CS_HAS_STD_HAZARD_POINTER_BATCH 0
#endif

#include "concurrency_study/exercise_check.hpp"

#include <iostream>

#if CS_HAS_STD_HAZARD_POINTER_BATCH
#include <array>
#include <atomic>
#include <hazard_pointer>
#include <span>
#endif

int main() try {
#if CS_HAS_STD_HAZARD_POINTER_BATCH
    std::array<std::hazard_pointer, 2> hps{};
    std::make_hazard_pointer_batch(std::span<std::hazard_pointer>{});
    std::make_hazard_pointer_batch(std::span{hps});
    cs::check(!hps[0].empty() && !hps[1].empty(), "nonempty batch owns all requested hazard pointers");

    struct node : std::hazard_pointer_obj_base<node> {
        int value = 42;
    };
    std::atomic<node*> first{new node{}};
    node* protected_node = hps[0].protect(first);
    cs::check(protected_node && protected_node->value == 42, "one handle in the batch protects a node");
    first.store(nullptr);
    cs::check(!hps[0].empty() && !hps[1].empty(), "mixed associated/unassociated handles remain owned");
    std::clear_hazard_pointer_batch(std::span{hps});
    cs::check(hps[0].empty() && hps[1].empty(), "clear destroys owned hazard pointers and leaves elements empty");
    std::clear_hazard_pointer_batch(std::span{hps});
    cs::check(hps[0].empty() && hps[1].empty(), "repeated clear keeps empty elements empty");
    protected_node->retire();
    std::cout << "F02 native C++29 hazard pointer batch OK: empty/nonempty/mixed/repeated-clear-empty\n";
    return 0;
#else
    std::cerr << "SKIP: CS_HAS_STD_HAZARD_POINTER_BATCH=0; C++29 hazard pointer batch unavailable\n";
    return 77;
#endif
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}
