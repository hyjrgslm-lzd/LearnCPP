#include "concurrency_study/queue_checks.hpp"
#include <iostream>

int main() {
    using namespace cs::queue_lab;
    check_const_copy([] { return spsc_ring<copy_overload_probe>{2}; });
    check_const_copy([] { return spsc_ring<copy_overload_probe, true>{2}; });
    for (std::size_t capacity : {1, 2, 7, 64}) {
        check_spsc_bool<false>(capacity);
        check_spsc_bool<true>(capacity);
        spsc_ring<std::size_t> plain(capacity);
        spsc_ring<std::size_t, true> cached(capacity);
        check_capacity(plain, capacity);
        check_capacity(cached, capacity);
        check_producer_order(transfer(plain, 20003, 1, 1), 20003, 1);
        check_producer_order(transfer(cached, 20003, 1, 1), 20003, 1);
        std::cout << "capacity=" << capacity << " atomics lock-free="
                  << plain.atomics_lock_free() << ',' << cached.atomics_lock_free() << '\n';
    }
    bool rejected = false;
    try { spsc_ring<std::size_t> invalid(0); }
    catch (const std::invalid_argument&) { rejected = true; }
    cs::check(rejected, "zero rejected");
    std::cout << "G3 OK: usable capacity, reuse, ordinary/cached FIFO\n";
}
