#include "concurrency_study/queue_checks.hpp"
#include "concurrency_study/queue_linked.hpp"
#include <iostream>

struct tracked_value {
    inline static std::atomic<int> live{0};
    std::size_t id = 0;
    tracked_value() { ++live; }
    tracked_value(const tracked_value& other) : id(other.id) { ++live; }
    tracked_value& operator=(const tracked_value& other) noexcept { id = other.id; return *this; }
    ~tracked_value() { --live; }
};

int main() {
    using namespace cs::queue_lab;
    check_const_copy([] { return treiber_stack<copy_overload_probe>{}; });
    treiber_stack<std::size_t> stack;
    std::size_t value = 99;
    cs::check(!stack.try_pop(value) && value == 99, "empty preserves output");
    cs::check(stack.try_push(10) && stack.try_push(20), "push two nodes");
    cs::check(stack.try_pop(value) && value == 20, "LIFO first");
    cs::check(stack.try_pop(value) && value == 10, "LIFO second");
    transfer(stack, 12007, 3, 4); // overlapping push/pop, every ID checked
    cs::check(!stack.try_pop(value), "stack drained");
    cs::hazard_pointer_cleanup();
    {
        treiber_stack<tracked_value> tracked;
        tracked_value in, out;
        for (std::size_t i = 0; i < 1000; ++i) {
            in.id = i;
            cs::check(tracked.try_push(in) && tracked.try_pop(out) && out.id == i, "tracked LIFO");
        }
        cs::hazard_pointer_cleanup();
        cs::check(tracked_value::live.load() == 2, "retired stack nodes released during run");
        tracked.try_push(in); // destructor also handles nonempty stack
    }
    cs::check(tracked_value::live.load() == 0, "all stack payloads destroyed");
    std::cout << "G1 OK; pointer atomics lock-free=" << stack.atomics_lock_free()
              << "; complete operation uses allocating/locking HP domain\n";
}
