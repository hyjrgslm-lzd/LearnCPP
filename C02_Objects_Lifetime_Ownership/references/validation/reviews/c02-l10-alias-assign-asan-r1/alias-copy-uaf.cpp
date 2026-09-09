#include <rc.hpp>
#include <cstdlib>
#include <iostream>

struct Node {
    explicit Node(int v) : value(v) {}
    int value;
    l10::rc_ptr<Node> child;
};

int main() {
    l10_support::reset_model();
    auto p = l10::make_rc<Node>(1);
    p->child = l10::make_rc<Node>(2);
    p = p->child;
    if (!p || p->value != 2 || p.use_count() != 1) {
        std::cerr << "copy alias assignment produced wrong owner\n";
        return 2;
    }
    p.reset();
    const auto c = l10_support::counters();
    if (c.objects_alive != 0 || c.control_blocks_alive != 0) {
        std::cerr << "copy alias assignment leaked: objects=" << c.objects_alive << " blocks=" << c.control_blocks_alive << "\n";
        return 3;
    }
    std::cout << "copy alias assignment OK\n";
    return 0;
}
