#include "concurrency_study/exercise_check.hpp"
#include "concurrency_study/work_stealing_pool.hpp"
#include <iostream>
#include <latch>
#include <cstdlib>
#include <new>
#include <system_error>

// Runtime-only allocation injection. No production pool hooks or global timing.
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

struct unwind_audit {
    bool data_survived_drain = false;
    std::exception_ptr producer_error;
};

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

int main() try {
    for (int round = 0; round < 20; ++round) {
        std::vector<std::atomic<int>> seen(400);
        std::vector<std::exception_ptr> errors(4);
        cs::work_stealing_pool pool(4);
        std::vector<std::jthread> producers;
        for (int p = 0; p < 4; ++p) producers.emplace_back([&, p] {
            try {
                std::vector<std::future<void>> futures;
                for (int i = p; i < 400; i += 4)
                    futures.push_back(pool.submit([&, i] { seen[i].fetch_add(1, std::memory_order_relaxed); }));
                for (auto& f : futures) f.get();
            } catch (...) { errors[p] = std::current_exception(); }
        });
        producers.clear();
        pool.join();
        for (auto e : errors) if (e) std::rethrow_exception(e);
        for (auto& value : seen) cs::check(value == 1, "each submitted ID executes exactly once");
    }
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
    // Pool identity matters: worker of A submitting into smaller B is external.
    cs::work_stealing_pool a(4), b(1);
    auto outer = a.submit([&] { auto inner = b.submit([] { return 19; }); return inner.get(); });
    cs::check(outer.get() == 19, "cross-pool submission uses target pool dispatch");
    a.join(); b.join();
    std::vector<std::atomic<int>> seen(200);
    std::vector<int> accepted(200);
    std::vector<std::exception_ptr> errors(2);
    cs::work_stealing_pool closing(2);
    std::latch start(1);
    std::vector<std::jthread> submitters;
    try {
        for (int p = 0; p < 2; ++p) submitters.emplace_back([&, p] {
            start.wait();
            try {
                for (int id = p; id < 200; id += 2) {
                    try {
                        closing.submit([&, id] { ++seen[id]; });
                        accepted[id] = 1;
                    } catch (const std::runtime_error&) { break; }
                }
            } catch (...) { errors[p] = std::current_exception(); }
        });
    } catch (...) { start.count_down(); throw; }
    start.count_down();
    closing.shutdown().get();
    submitters.clear(); closing.join();
    for (auto e : errors) if (e) std::rethrow_exception(e);
    for (std::size_t i = 0; i < seen.size(); ++i)
        cs::check(seen[i] == accepted[i], "submit/shutdown race executes exactly the accepted tasks");
    std::cout << "scheduling runtime checks OK\n";
} catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
