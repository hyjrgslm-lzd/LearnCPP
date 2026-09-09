#include "concurrency_study/exercise_check.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <iostream>

// Exhaust all four-operation SC schedules respecting each thread's program order.
// This is an SC interleaving model, not an ARM emulator or a full C++ model checker.
int main() {
    std::array<int, 4> schedule{0, 1, 2, 3};
    unsigned outcomes = 0;
    int legal = 0;
    bool lost_update = false, retained_update = false;
    do {
        const auto pos = [&](int op) {
            return std::find(schedule.begin(), schedule.end(), op) - schedule.begin();
        };
        if (pos(0) > pos(1) || pos(2) > pos(3)) continue;
        ++legal;
        int x = 0, y = 0, r0 = -1, r1 = -1;
        int split = 0, a = -1, b = -1;
        for (int op : schedule) {
            switch (op) {
            case 0: x = 1; a = split; break;
            case 1: r0 = y; split = a + 1; break;
            case 2: y = 1; b = split; break;
            case 3: r1 = x; split = b + 1; break;
            }
        }
        outcomes |= 1u << (r0 * 2 + r1);
        lost_update |= split == 1;
        retained_update |= split == 2;
    } while (std::next_permutation(schedule.begin(), schedule.end()));
    cs::check(legal == 6 && outcomes == 0b1110, "SC SB outcomes exactly 01,10,11");
    cs::check(lost_update && retained_update, "SC split increment can still lose updates");

    std::atomic<int> value{8};
    int expected = 7;
    cs::check(!value.compare_exchange_strong(expected, 9) && expected == 8,
              "failed CAS refills expected");
    cs::check(value.compare_exchange_strong(expected, 8) && expected == 8,
              "unchanged numeric value can be a successful RMW");
    std::cout << "atomic_protocol_test OK: six schedules and CAS return contracts\n";
}
