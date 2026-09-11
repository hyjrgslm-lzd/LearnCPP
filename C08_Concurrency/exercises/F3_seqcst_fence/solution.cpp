#include "concurrency_study/exercise_check.hpp"

#include <array>
#include <atomic>
#include <barrier>
#include <future>
#include <iostream>

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

enum class mode { relaxed, acq_rel, seq_cst, sc_fence };

int store_load(std::atomic<int>& own, std::atomic<int>& other, mode variant) {
    const auto store_order = variant == mode::seq_cst ? std::memory_order_seq_cst :
                             variant == mode::acq_rel ? std::memory_order_release :
                             std::memory_order_relaxed;
    const auto load_order = variant == mode::seq_cst ? std::memory_order_seq_cst :
                            variant == mode::acq_rel ? std::memory_order_acquire :
                            std::memory_order_relaxed;
    own.store(1, store_order);
    if (variant == mode::sc_fence) std::atomic_thread_fence(std::memory_order_seq_cst);
    return other.load(load_order);
}

int store_buffering(mode variant) {
    constexpr int rounds = 4000;
    std::atomic<int> x{0}, y{0};
    std::barrier phase(2);
    std::array<int, rounds> worker_reads{}, main_reads{};
    auto worker = std::async(std::launch::async, [&] {
        for (int i = 0; i < rounds; ++i) {
            phase.arrive_and_wait();
            worker_reads[i] = store_load(x, y, variant);
            phase.arrive_and_wait();
        }
    });
    for (int i = 0; i < rounds; ++i) {
        x.store(0, std::memory_order_relaxed);
        y.store(0, std::memory_order_relaxed);
        phase.arrive_and_wait();
        main_reads[i] = store_load(y, x, variant);
        phase.arrive_and_wait();
    }
    worker.get(); // assertions/logging happen only after both participants finish
    int both_zero = 0;
    for (int i = 0; i < rounds; ++i) {
        cs::check((worker_reads[i] == 0 || worker_reads[i] == 1) &&
                  (main_reads[i] == 0 || main_reads[i] == 1), "litmus result domain");
        both_zero += worker_reads[i] == 0 && main_reads[i] == 0;
    }
    return both_zero;
}

// Three fence publication forms: fence -> atomic, atomic -> fence, fence -> fence.
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
    const int observed = data; // do not move after future.get(), which would hide missing edges
    producer.get();
    cs::check(observed == 42, "fence publication bridge");
}

int main() {
    const int relaxed = store_buffering(mode::relaxed);
    const int ra = store_buffering(mode::acq_rel);
    const int sc = store_buffering(mode::seq_cst);
    const int fence = store_buffering(mode::sc_fence);
    cs::check(sc == 0 && fence == 0, "SC operations and symmetric SC fences forbid 0,0");
#if CS_VERIFIED_CLANG18_GLIBCXX13_TSAN_LIMIT
    std::cout << "SKIP: verified Clang 18 + libstdc++ 13 + TSan does not support std::atomic_thread_fence "
                 "as a publication edge; store-buffering ran here and fence_publication is covered by non-TSan runs; see "
                 "C08_Concurrency/exercises/BUILD_GUIDE.md\n";
#else
    for (int form = 0; form < 3; ++form) fence_publication(form);
#endif
    std::cout << "F3_reference OK: both-zero relaxed=" << relaxed << ", RA=" << ra
              << ", SC=" << sc << ", SC-fence=" << fence;
#if CS_VERIFIED_CLANG18_GLIBCXX13_TSAN_LIMIT
    std::cout << "; fence publication skipped under this TSan build\n";
    return 77;
#else
    std::cout << "; three fence bridges\n";
#endif
}
