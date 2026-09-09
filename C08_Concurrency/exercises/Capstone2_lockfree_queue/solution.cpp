#include "concurrency_study/queue_checks.hpp"
#include "concurrency_study/queue_baseline.hpp"
#include <array>
#include <iostream>

int main() {
    using namespace cs::queue_lab;
    check_const_copy([] { return mutex_queue<copy_overload_probe>{2}; });
    check_const_copy([] { return mutex_ring<copy_overload_probe>{2}; });
    check_const_copy([] { return mpmc_ring<copy_overload_probe>{2}; });
    {
        mutex_ring<bool> bits(2);
        const std::array<bool, 3> in{true, false, true};
        std::array<bool, 3> out{false, true, true};
        cs::check(bits.push_batch(in) == 2 && bits.pop_batch(out) == 2,
                  "locked bool batch compiles and accepts prefix");
        cs::check(out[0] && !out[1] && out[2], "const vector<bool> access preserves batch values");
    }
    spsc_ring<std::size_t> spsc(8);
    check_capacity(spsc, 8);
    check_producer_order(transfer(spsc, 10003, 1, 1), 10003, 1);
    for (std::size_t capacity : {0, 1, 3, 6}) {
        bool rejected = false;
        try { mpmc_ring<std::size_t> invalid(capacity); }
        catch (const std::invalid_argument&) { rejected = true; }
        cs::check(rejected, "reject 0/1/non-power-of-two");
    }
    mutex_queue<std::size_t> baseline(8);
    mutex_ring<std::size_t> ring(8);
    check_capacity(baseline, 8);
    check_capacity(ring, 8);
    std::array<std::size_t, 10> input{0,1,2,3,4,5,6,7,8,9}, output{};
    cs::check(ring.push_batch(input) == 8, "batch accepts prefix fitting capacity");
    cs::check(ring.pop_batch(output) == 8, "batch returns actual elements");
    for (std::size_t i = 0; i < 8; ++i) cs::check(output[i] == i, "batch FIFO prefix");
    transfer(baseline, 10003, 3, 4);
    transfer(ring, 10003, 3, 4, true, 3);
    for (std::size_t capacity : {2, 4, 64}) {
        mpmc_ring<std::size_t> q(capacity);
        check_capacity(q, capacity);
        check_producer_order(transfer(q, 10003, 3, 1), 10003, 3);
        transfer(q, 10003, 1, 4); // SPMC topology, no dedicated SPMC claim
        transfer(q, 10003, 3, 4);
        std::cout << "capacity=" << capacity << " atomics lock-free=" << q.atomics_lock_free() << '\n';
    }
    check_reservation_gap<mpmc_ring<std::size_t>>();
    check_narrow_wrap();
    std::cout << "Capstone2 OK: capacity, batch, MPSC/SPMC/MPMC IDs, reservation gap, wrap model\n";
}
