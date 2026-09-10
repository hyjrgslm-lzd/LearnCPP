#include "concurrency_study/exercise_check.hpp"
#include <atomic>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <vector>

#ifndef CS_VERIFIED_CLANG18_GLIBCXX13_TSAN_LIMIT
#if defined(__clang__) && defined(__clang_major__) && defined(__has_feature) && defined(_GLIBCXX_RELEASE)
#if __clang_major__ == 18 && _GLIBCXX_RELEASE == 13 && __has_feature(thread_sanitizer)
#define CS_VERIFIED_CLANG18_GLIBCXX13_TSAN_LIMIT 1
#else
#define CS_VERIFIED_CLANG18_GLIBCXX13_TSAN_LIMIT 0
#endif
#else
#define CS_VERIFIED_CLANG18_GLIBCXX13_TSAN_LIMIT 0
#endif
#endif

std::atomic<int> static_constructions{0};
const int& local_resource() {
    static const int value = [] { ++static_constructions; return 456; }();
    return value;
}

void successful_call_once_publication() {
    std::once_flag flag;
    std::unique_ptr<int> value;
    int attempts = 0; // accessed only by serialized active calls, then after get
    std::vector<std::future<void>> readers;
    for (int i = 0; i < 8; ++i)
        readers.push_back(std::async(std::launch::async, [&] {
            std::call_once(flag, [&](int initial) {
                ++attempts;
                value = std::make_unique<int>(initial);
            }, 123);
            cs::check(*value == 123, "passive caller sees initialized object");
            cs::check(local_resource() == 456, "local static publication");
        }));
    for (auto& reader : readers) reader.get();
    cs::check(attempts == 1, "one successful initialization");
    cs::check(static_constructions == 1, "local static constructed once");
    std::call_once(flag, [] { cs::check(false, "successful flag must not rerun"); });
}

void exception_retry_publication() {
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
}

int main() {
#if CS_VERIFIED_CLANG18_GLIBCXX13_TSAN_LIMIT
    successful_call_once_publication();
    std::cout << "B3 OK: once-only success, publication, local static\n";
    std::cout << "SKIP: verified Clang 18 + libstdc++ 13 + TSan does not reliably verify C++ exception paths; "
                 "call_once exception retry is covered by non-TSan runs, while success publication and local static ran here; see "
                 "references/validation/c08-revision/tsan-diagnosis/diagnosis-20260910.md\n";
    return 77;
#else
    exception_retry_publication();
    std::cout << "B3 OK: retry, once-only success, publication, local static\n";
#endif
}
