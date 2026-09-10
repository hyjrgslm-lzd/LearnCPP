#include <atomic>
#include <thread>

int main() {
    std::atomic<int> value{0};
    std::thread worker([&] { value.store(1); });
    worker.join();
    return value.load() == 1 ? 0 : 1;
}
