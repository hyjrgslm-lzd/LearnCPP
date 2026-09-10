#include <thread>

int main() {
    int value = 0;
    std::thread worker([&] { value = 1; });
    value = 2;
    worker.join();
    return value;
}
