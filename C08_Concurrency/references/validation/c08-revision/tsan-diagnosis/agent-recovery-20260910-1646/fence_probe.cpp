#include <atomic>
#include <cassert>
#include <future>
#include <iostream>

void fence_publication(int form) {
    int data = 0;
    std::atomic<bool> ready{false};
    auto producer = std::async(std::launch::async, [&] {
        data = 42;
        if (form != 1) std::atomic_thread_fence(std::memory_order_release);
        ready.store(true, form == 1 ? std::memory_order_release : std::memory_order_relaxed);
        ready.notify_one();
    });
    ready.wait(false, form == 0 ? std::memory_order_acquire : std::memory_order_relaxed);
    if (form != 0) std::atomic_thread_fence(std::memory_order_acquire);
    int observed = data;
    producer.get();
    assert(observed == 42);
}

void atomic_control() {
    int data = 0;
    std::atomic<bool> ready{false};
    auto producer = std::async(std::launch::async, [&] {
        data = 42;
        ready.store(true, std::memory_order_release);
        ready.notify_one();
    });
    ready.wait(false, std::memory_order_acquire);
    int observed = data;
    producer.get();
    assert(observed == 42);
}

int main(int argc, char** argv) {
    std::string mode = argc > 1 ? argv[1] : "fence0";
    if (mode == "atomic") atomic_control();
    else if (mode == "fence0") fence_publication(0);
    else if (mode == "fence1") fence_publication(1);
    else if (mode == "fence2") fence_publication(2);
    else assert(false);
    std::cout << "ok " << mode << "\n";
}
