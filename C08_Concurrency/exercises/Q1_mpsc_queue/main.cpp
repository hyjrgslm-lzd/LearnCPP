#include "concurrency_study/queue_checks.hpp"
#include <iostream>
int main() {
    cs::queue_lab::check_reservation_gap<cs::queue_lab::mpsc_ring<std::size_t>>();
    std::cout << "MPSC publication gap reproduced without sleep; predict the two final values.\n";
}
