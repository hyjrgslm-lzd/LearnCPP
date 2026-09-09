#include "concurrency_study/queue_checks.hpp"
#include <iostream>

int main() {
    using namespace cs::queue_lab;
    check_const_copy([] { return mpsc_ring<copy_overload_probe>{2}; });
    mpsc_ring<std::size_t> q(8);
    check_capacity(q, 8);
    check_producer_order(transfer(q, 12007, 4, 1), 12007, 4);
    check_reservation_gap<mpsc_ring<std::size_t>>();
    std::cout << "Q1 OK: producer order, exact IDs, deterministic publication gap; atomics lock-free="
              << q.atomics_lock_free() << '\n';
}
