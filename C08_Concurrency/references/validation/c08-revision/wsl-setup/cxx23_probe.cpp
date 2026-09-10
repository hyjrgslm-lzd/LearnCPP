#include <atomic>
#include <barrier>
#include <iostream>
#include <thread>
#include <vector>

int main() {
    std::atomic<int> sum{0};
    std::barrier sync(4);
    std::vector<std::jthread> threads;

    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&] {
            sync.arrive_and_wait();
            sum.fetch_add(1, std::memory_order_relaxed);
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    std::cout << "sum=" << sum.load() << '\n';
    return sum.load() == 4 ? 0 : 1;
}
