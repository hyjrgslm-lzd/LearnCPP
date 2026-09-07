#pragma once
// 学生池与公共池共享相同契约检查；本文件不包含任何池实现。
#include "concurrency_study/exercise_check.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>

#include <functional>
#include <future>
#include <thread>
#include <type_traits>
#include <vector>

namespace pool_checks {
using cs::check;

// Releases gated tasks during unwinding too; declare AFTER the pool.
struct task_gate {
    std::promise<void> promise;
    std::shared_future<void> ready = promise.get_future().share();
    bool released = false;
    void release() { if (!released) { promise.set_value(); released = true; } }
    ~task_gate() { release(); }
};

struct ref_qualified {
    int operator()() & { return 1; }
    std::string operator()() && { return "rvalue"; }
};

template<class Pool>
void results_and_types() {
    constexpr int count = 300;
    std::array<std::atomic<int>, count> seen{}; // outlives pool even if submit throws
    Pool pool(3, 1);
    const auto caller = std::this_thread::get_id();
    check(pool.submit([] { return std::this_thread::get_id(); }).get() != caller,
          "task must run on a worker, not submitter");
    check(pool.submit([](std::unique_ptr<int> p) { return *p; },
                      std::make_unique<int>(42)).get() == 42, "move-only argument");
    check(pool.submit([p = std::make_unique<int>(7)] { return *p; }).get() == 7,
          "move-only callable");
    ref_qualified fun;
    static_assert(std::is_same_v<decltype(pool.submit(fun)), std::future<std::string>>);
    check(pool.submit(fun).get() == "rvalue", "decay storage matches result deduction");
    check(pool.submit(std::ref(fun)).get() == 1, "explicit callable borrowing");
    int value = 4;
    pool.submit([](int& x) { x += 3; }, std::ref(value)).get();
    check(value == 7, "void future and borrowed argument");
    struct object { int n = 6; int add(int x) const { return n + x; } } obj;
    check(pool.submit(&object::add, obj, 5).get() == 11, "member function invocation");
    auto failed = pool.submit([]() -> int { throw std::runtime_error("task failure"); });
    bool caught = false;
    try { (void)failed.get(); }
    catch (const std::runtime_error& e) { caught = std::string(e.what()) == "task failure"; }
    check(caught, "task exception reaches future");
    check(pool.submit([] { return 9; }).get() == 9, "worker survives task exception");

    std::vector<std::future<int>> results;
    for (int i = 0; i < count; ++i)
        results.push_back(pool.submit([&, i] { ++seen[i]; return i * i; }));
    pool.shutdown();
    pool.shutdown();
    for (int i = 0; i < count; ++i) {
        check(results[i].get() == i * i, "every result retained");
        check(seen[i] == 1, "every accepted ID executed exactly once");
    }
    bool rejected = false;
    try { (void)pool.submit([] {}); } catch (const std::runtime_error&) { rejected = true; }
    check(rejected, "shutdown rejects new work");
}

template<class Pool>
void close_full_pool() {
    Pool pool(1, 1);
    task_gate gate;
    std::promise<void> running, attempting;
    auto entered = running.get_future();
    auto attempt = attempting.get_future();
    auto first = pool.submit([&, ready = gate.ready] { running.set_value(); ready.wait(); return 1; });
    entered.get(); // the only worker is occupied until gate.release()
    auto second = pool.submit([] { return 2; }); // the only waiting slot is occupied
    auto producer = std::async(std::launch::async, [&] {
        attempting.set_value();
        try { (void)pool.submit([] { return 3; }); return false; }
        catch (const std::runtime_error&) { return true; }
    });
    attempt.get();
    // Zero-time probe cannot prove the producer is already parked inside wait.
    // The gates DO prove that a third task cannot be accepted before shutdown.
    const bool pending = producer.wait_for(std::chrono::seconds(0)) == std::future_status::timeout;
    std::future<void> closer;
    try {
        closer = std::async(std::launch::async, [&] { pool.shutdown(); });
    } catch (...) {
        // Do not unwind into producer's blocking future destructor with the
        // only worker still gated if the closer thread could not be created.
        gate.release();
        pool.shutdown();
        throw;
    }
    bool rejected = false;
    try {
        rejected = producer.get(); // close must wake a producer if it parked
    } catch (...) {
        gate.release();
        closer.wait(); // do not strand the closer while propagating worker failure
        throw;
    }
    gate.release();
    closer.get();
    check(pending && rejected, "full submit rejected by close before worker released");
    check(first.get() == 1 && second.get() == 2, "close drains accepted queued work");
}

template<class Pool>
void ownership_and_shutdown() {
    for (auto config : {std::pair{0u, 1u}, std::pair{1u, 0u}}) {
        bool rejected = false;
        try { Pool invalid(config.first, config.second); }
        catch (const std::invalid_argument&) { rejected = true; }
        check(rejected, "zero worker/capacity rejected");
    }
    Pool pool(2, 1);
    auto self_shutdown = pool.submit([&] { pool.shutdown(); });
    auto nested_submit = pool.submit([&] { (void)pool.submit([] {}); });
    for (auto* result : {&self_shutdown, &nested_submit}) {
        bool rejected = false;
        try { result->get(); } catch (const std::logic_error&) { rejected = true; }
        check(rejected, "same-worker operation rejected before deadlock");
    }
    auto a = std::async(std::launch::async, [&] { pool.shutdown(); });
    auto b = std::async(std::launch::async, [&] { pool.shutdown(); });
    a.get(); b.get(); // idle consumer wakeup and concurrent idempotence
    std::future<int> last;
    {
        Pool scoped(1, 1);
        last = scoped.submit([] { return 77; });
    }
    check(last.get() == 77, "destructor drains and result state outlives pool");
}

template<class Pool>
void run() {
    results_and_types<Pool>();
    close_full_pool<Pool>();
    ownership_and_shutdown<Pool>();
    std::cout << "thread_pool OK: asynchronous, futures, capacity=1, drain, shutdown, ownership\n";
}
} // namespace pool_checks
