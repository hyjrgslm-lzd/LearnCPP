#include "concurrency_study/exercise_check.hpp"
#include <atomic>
#include <future>
#include <iostream>

int main() {
    std::atomic<int> x{0}, y{0};
    auto worker = std::async(std::launch::async, [&] {
        x.store(1);
        return y.load();
    });
    y.store(1);
    const int main_read = x.load();
    const int worker_read = worker.get();
    cs::check(main_read != 0 || worker_read != 0, "SC excludes both-zero");
    // TODO Part 1: finite rounds, barriers OUTSIDE the store/load window; add relaxed and RA.
    // TODO Part 2: symmetric SC fences with relaxed atomic accesses.
    // TODO Part 3: implement the three release/acquire fence publication forms.
    std::cout << "Starter: SC observations=" << main_read << ',' << worker_read << '\n';
}
