#include <atomic>
#include <cassert>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

void retry_body() {
    std::once_flag flag;
    int attempts = 0;
    std::unique_ptr<int> value;
    for (;;) {
        try {
            std::call_once(flag, [&] {
                if (++attempts == 1) throw std::runtime_error("retry");
                value = std::make_unique<int>(123);
            });
            break;
        } catch (const std::runtime_error&) {}
    }
    assert(attempts == 2);
    assert(*value == 123);
}

void no_throw_body() {
    std::once_flag flag;
    int attempts = 0;
    int value = 0;
    std::call_once(flag, [&] { ++attempts; value = 123; });
    std::call_once(flag, [&] { assert(false); });
    assert(attempts == 1);
    assert(value == 123);
}

int main(int argc, char** argv) {
    std::string mode = argc > 1 ? argv[1] : "same-thread-retry";
    if (mode == "same-thread-retry") retry_body();
    else if (mode == "thread-retry") { std::thread t(retry_body); t.join(); }
    else if (mode == "async-retry") { auto f = std::async(std::launch::async, retry_body); f.get(); }
    else if (mode == "async-8-retry") {
        std::once_flag flag;
        std::unique_ptr<int> value;
        int attempts = 0;
        std::atomic<int> failures{0};
        std::vector<std::future<void>> fs;
        for (int i = 0; i < 8; ++i) fs.push_back(std::async(std::launch::async, [&] {
            for (;;) {
                try {
                    std::call_once(flag, [&] { if (++attempts == 1) throw std::runtime_error("retry"); value = std::make_unique<int>(123); });
                    break;
                } catch (const std::runtime_error&) { ++failures; }
            }
            assert(*value == 123);
        }));
        for (auto& f : fs) f.get();
        assert(attempts == 2);
        assert(failures == 1);
    }
    else if (mode == "async-8-control") {
        std::once_flag flag;
        std::unique_ptr<int> value;
        int attempts = 0;
        std::vector<std::future<void>> fs;
        for (int i = 0; i < 8; ++i) fs.push_back(std::async(std::launch::async, [&] {
            std::call_once(flag, [&] { ++attempts; value = std::make_unique<int>(123); });
            assert(*value == 123);
        }));
        for (auto& f : fs) f.get();
        assert(attempts == 1);
    }
    else no_throw_body();
    std::cout << "ok " << mode << "\n";
}
