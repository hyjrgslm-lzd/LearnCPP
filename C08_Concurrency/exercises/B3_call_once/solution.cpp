#include "concurrency_study/exercise_check.hpp"
#include <atomic>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <vector>

std::atomic<int> static_constructions{0};
const int& local_resource() {
    static const int value = [] { ++static_constructions; return 456; }();
    return value;
}

int main() {
    std::once_flag flag;
    std::unique_ptr<int> value;
    int attempts = 0; // accessed only by serialized active calls, then after get
    std::atomic<int> failures{0};
    std::vector<std::future<void>> readers;
    for (int i = 0; i < 8; ++i)
        readers.push_back(std::async(std::launch::async, [&] {
            for (;;) {
                try {
                    std::call_once(flag, [&](int initial) {
                        if (++attempts == 1) throw std::runtime_error("retry me");
                        value = std::make_unique<int>(initial);
                    }, 123);
                    break;
                } catch (const std::runtime_error&) { ++failures; }
            }
            cs::check(*value == 123, "passive caller sees initialized object");
            cs::check(local_resource() == 456, "local static publication");
        }));
    for (auto& reader : readers) reader.get();
    cs::check(attempts == 2 && failures == 1, "one failed attempt and one success");
    cs::check(static_constructions == 1, "local static constructed once");
    std::call_once(flag, [] { cs::check(false, "successful flag must not rerun"); });
    std::cout << "B3 OK: retry, once-only success, publication, local static\n";
}
