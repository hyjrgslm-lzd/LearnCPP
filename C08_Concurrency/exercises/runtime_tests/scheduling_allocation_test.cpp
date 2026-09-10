#include "concurrency_study/exercise_check.hpp"
#include "concurrency_study/work_stealing_pool.hpp"
#include <atomic>
#include <cstdlib>
#include <exception>
#include <future>
#include <iostream>
#include <latch>
#include <new>
#include <string_view>
#include <system_error>
#include <utility>
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

struct unwind_audit {
    bool data_survived_drain = false;
    std::exception_ptr producer_error;
};

#if !CS_VERIFIED_CLANG18_GLIBCXX13_TSAN_LIMIT
// Runtime-only allocation injection. The verified Clang 18 + libstdc++ 13 + TSan
// combination owns global new/delete interceptors.
static thread_local bool fail_next_allocation = false;

void* operator new(std::size_t bytes) {
    if (std::exchange(fail_next_allocation, false)) throw std::bad_alloc();
    if (void* result = std::malloc(bytes ? bytes : 1)) return result;
    throw std::bad_alloc();
}

void* operator new[](std::size_t bytes) { return ::operator new(bytes); }
void operator delete(void* address) noexcept { std::free(address); }
void operator delete[](void* address) noexcept { std::free(address); }
void operator delete(void* address, std::size_t) noexcept { std::free(address); }
void operator delete[](void* address, std::size_t) noexcept { std::free(address); }

static void inject_creation_and_submit_failure(unwind_audit& audit) {
    std::vector<std::atomic<int>> seen(1);
    std::vector<std::exception_ptr> errors(1);
    std::latch submitted(1), fail_submit(1), release_task(1);
    struct data_observer {
        decltype(seen)& values;
        decltype(errors)& failures;
        unwind_audit& audit;
        ~data_observer() {
            audit.data_survived_drain = values[0] == 1;
            audit.producer_error = failures[0];
        }
    } observe{seen, errors, audit}; // Examines live data AFTER the pool destructor.
    cs::work_stealing_pool pool(1);
    std::vector<std::jthread> producers; // Destroy producers -> pool -> data.
    try {
        producers.emplace_back([&] {
            bool accepted = false;
            try {
                pool.submit([&] { release_task.wait(); ++seen[0]; });
                accepted = true;
            } catch (...) { errors[0] = std::current_exception(); }
            submitted.count_down();
            fail_submit.wait();
            if (!accepted) return;
            try {
                fail_next_allocation = true;
                pool.submit([] {}); // The real submit's make_shared allocation fails.
                fail_next_allocation = false;
                errors[0] = std::make_exception_ptr(std::runtime_error("injection did not fire"));
            } catch (...) {
                fail_next_allocation = false;
                errors[0] = std::current_exception();
            }
        });
        submitted.wait(); // At least one accepted task exists before launch failure.
        // Deterministic failure at the SECOND producer launch site, not an OS
        // resource-exhaustion claim. The first producer is still alive here.
        throw std::system_error(std::make_error_code(std::errc::resource_unavailable_try_again),
                                "injected producer creation failure");
    } catch (...) {
        fail_submit.count_down();
        release_task.count_down();
        throw; // Preserve the creation exception while RAII performs all joins.
    }
}
#endif

int main() try {
#if CS_VERIFIED_CLANG18_GLIBCXX13_TSAN_LIMIT
    std::cout << "SKIP: verified Clang 18 + libstdc++ 13 + TSan provides global new/delete interceptors; "
                 "allocation injection is covered by non-TSan runs; see "
                 "references/validation/c08-revision/tsan-diagnosis/diagnosis-20260910.md\n";
    return 77;
#else
    unwind_audit audit;
    bool original_exception = false;
    try { inject_creation_and_submit_failure(audit); }
    catch (const std::system_error& error) {
        original_exception = error.code() == std::errc::resource_unavailable_try_again &&
            std::string_view(error.what()).find("injected producer creation failure") != std::string_view::npos;
    }
    cs::check(original_exception, "original thread-creation failure survives producer/pool cleanup");
    cs::check(audit.data_survived_drain, "accepted work drains before seen/errors are destroyed");
    bool allocation_exception = false;
    if (audit.producer_error) try { std::rethrow_exception(audit.producer_error); }
        catch (const std::bad_alloc&) { allocation_exception = true; }
    cs::check(allocation_exception, "live producer's actual submit allocation failure is preserved");
    std::cout << "scheduling allocation injection checks OK\n";
#endif
} catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
