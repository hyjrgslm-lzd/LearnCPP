#include "concurrency_study/exercise_check.hpp"
#include "concurrency_study/work_stealing_pool.hpp"
#include <atomic>
#include <exception>
#include <future>
#include <iostream>
#include <latch>
#include <vector>

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
