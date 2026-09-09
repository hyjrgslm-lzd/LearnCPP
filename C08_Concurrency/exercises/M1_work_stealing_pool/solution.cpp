#include "concurrency_study/exercise_check.hpp"
#include "concurrency_study/work_stealing_pool.hpp"
#include <algorithm>
#include <iostream>
#include <latch>

static std::uint64_t sum(cs::work_stealing_pool& pool, std::size_t lo, std::size_t hi) {
    if (hi - lo <= 32) {
        std::uint64_t value = 0;
        for (auto i = lo; i < hi; ++i) value += i;
        return value;
    }
    const auto mid = lo + (hi - lo) / 2;
    auto left = pool.submit([&pool, lo, mid] { return sum(pool, lo, mid); });
    const auto right = sum(pool, mid, hi);
    return pool.wait(left) + right;
}

int main() try {
    using cs::check;
    bool zero_rejected = false;
    try { cs::work_stealing_pool invalid(0); } catch (const std::invalid_argument&) { zero_rejected = true; }
    check(zero_rejected, "zero workers rejected");
    for (auto variant : {"static", "dynamic", "stealing"}) {
        for (std::size_t size : {0, 1, 257}) {
            auto values = cs::scheduling::run(variant, size, 3);
            check(values.size() == size, "size preserved");
            for (std::size_t id = 0; id < size; ++id)
                check(values[id] == cs::scheduling::work(id, size), "each skewed task result checked");
        }
    }
    for (std::size_t threads : {1, 4}) {
        cs::work_stealing_pool pool(threads);
        auto answer = pool.submit([&] { return sum(pool, 0, 4096); });
        check(answer.get() == 4095ULL * 4096 / 2, "recursive join with one or many workers");
        auto moved = pool.submit([value = std::make_unique<int>(42)] { return *value; });
        check(moved.get() == 42, "move-only callable");
        auto error = pool.submit([]() -> int { throw std::runtime_error("task-error"); });
        bool caught = false;
        try { error.get(); } catch (const std::runtime_error& e) { caught = std::string_view(e.what()) == "task-error"; }
        check(caught, "task exception delivered");
        auto again = pool.submit([] { return 7; });
        check(again.get() == 7, "pool survives user exception");
        std::latch entered(1), release(1);
        auto blocked = pool.submit([&] { entered.count_down(); release.wait(); });
        entered.wait();
        auto draining = pool.shutdown();
        const bool premature = draining.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
        release.count_down(); // Always release before any throwing check.
        check(!premature, "active task keeps drain pending even if queues empty");
        blocked.get(); draining.get(); pool.join(); pool.join();
        bool rejected = false;
        try { pool.submit([] {}); } catch (const std::runtime_error&) { rejected = true; }
        check(rejected, "submit after shutdown rejected");
    }
    // Every root-owned child must be stolen: root blocks on an external release,
    // main waits for child completion before releasing root. Exactly two workers.
    cs::work_stealing_pool pool(2);
    std::promise<void> child_done, release_root;
    auto released = release_root.get_future().share();
    auto child_future = child_done.get_future();
    auto root = pool.submit([&] {
        try { pool.submit([&] { child_done.set_value(); }); }
        catch (...) { child_done.set_exception(std::current_exception()); }
        released.wait();
    });
    try { child_future.get(); }
    catch (...) { release_root.set_value(); root.get(); throw; }
    release_root.set_value(); root.get(); pool.join();
    check(pool.steals() >= 1, "another worker stole the owner's queued child");
    std::cout << "M1 reference OK: variants, recursion, errors, drain, stealing\n";
} catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
