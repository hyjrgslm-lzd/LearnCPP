#ifndef CS_HAS_STD_HAZARD_POINTER
#define CS_HAS_STD_HAZARD_POINTER 0
#endif

#include "concurrency_study/exercise_check.hpp"

#include <iostream>

#if CS_HAS_STD_HAZARD_POINTER
#include <atomic>
#include <hazard_pointer>
#endif

int main() try {
#if CS_HAS_STD_HAZARD_POINTER
    struct hp_node : std::hazard_pointer_obj_base<hp_node> {
        int value = 7;
    };
    std::atomic<hp_node*> source{new hp_node{}};
    auto hp = std::make_hazard_pointer();
    hp_node* protected_node = hp.protect(source);
    cs::check(protected_node && protected_node->value == 7, "native standard HP protect");
    source.store(nullptr);
    hp.reset_protection();
    protected_node->retire();
    std::cout << "F03 native std hazard_pointer OK\n";
    return 0;
#else
    std::cerr << "SKIP: CS_HAS_STD_HAZARD_POINTER=0; native standard hazard_pointer unavailable\n";
    return 77;
#endif
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}
