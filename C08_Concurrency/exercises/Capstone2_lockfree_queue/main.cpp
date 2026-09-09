#include "concurrency_study/queue_checks.hpp"
#include <iostream>
int main() {
    cs::queue_lab::check_reservation_gap<cs::queue_lab::mpmc_ring<std::size_t>>();
    std::cout << "A later push completed, but pop temporarily failed behind an unpublished ticket.\n"
                 "This is a bounded reservation protocol; see solution.cpp for all Parts.\n";
}
