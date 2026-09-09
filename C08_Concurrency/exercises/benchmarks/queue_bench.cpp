#include "concurrency_study/benchmark.hpp"
#include "concurrency_study/queue_baseline.hpp"
#include "concurrency_study/queue_checks.hpp"
#include "concurrency_study/queue_linked.hpp"
#include <iostream>
#include <string>

int main(int argc, char** argv) try {
    cs::bench::arguments args(argc, argv);
    const auto size = args.number("--size", 100003);
    const auto producers = args.number("--producers", 2);
    const auto consumers = args.number("--consumers", 2);
    const auto capacity = args.number("--capacity", 1024);
    const auto batch = args.number("--batch", 1);
    const auto variant = args.text("--variant", "mutex");
    args.finish();
    cs::check(size > 0 && producers > 0 && consumers > 0 && batch > 0,
              "size, roles and batch must be positive");
    cs::check(producers <= 32 && consumers <= 32, "max 32 producers/consumers (HP slot budget)");
    cs::check(batch <= 1048576, "batch allocation bound");
    cs::check(variant == "batch" || batch == 1, "only batch variant accepts batch > 1");
    auto run = [&](auto& queue, const std::string& semantics) {
        cs::queue_lab::transfer_result result;
        const double ms = cs::bench::measure_ms([&] {
            result = cs::queue_lab::transfer(queue, size, producers, consumers, false, batch);
        });
        std::string atomic_info = "n/a";
        if constexpr (requires { queue.atomics_lock_free(); })
            atomic_info = queue.atomics_lock_free() ? "true" : "false";
        cs::bench::emit_row("queue", variant, size, producers + consumers, ms, result.completed,
            "P=" + std::to_string(producers) + ";C=" + std::to_string(consumers)
            + ";capacity=" + std::to_string(capacity) + ";batch=" + std::to_string(batch)
            + ";" + semantics + ";atomic_lock_free=" + atomic_info
            + ";retry=yield;timed=thread-create+transfer+join;final-destruction=excluded");
    };
    using namespace cs::queue_lab;
    if (variant == "mutex") { mutex_queue<std::size_t> q(capacity); run(q, "bounded;strict-try"); }
    else if (variant == "ring" || variant == "batch") {
        mutex_ring<std::size_t> q(capacity); run(q, "bounded;strict-try;prefix-batch");
    } else if (variant == "spsc" || variant == "spsc-cached") {
        cs::check(producers == 1 && consumers == 1, "SPSC requires P=1 C=1");
        if (variant == "spsc") { spsc_ring<std::size_t> q(capacity); run(q, "bounded;observed-empty-full"); }
        else { spsc_ring<std::size_t, true> q(capacity); run(q, "bounded;observed-empty-full"); }
    } else if (variant == "mpsc") {
        cs::check(consumers == 1, "MPSC requires C=1");
        mpsc_ring<std::size_t> q(capacity); run(q, "bounded;transient-false;not-strict-lock-free-FIFO");
    } else if (variant == "mpmc") {
        mpmc_ring<std::size_t> q(capacity); run(q, "bounded;transient-false;not-strict-lock-free-FIFO");
    } else if (variant == "ms") {
        cs::check(capacity == 0, "MS is unbounded: explicitly use --capacity 0; separate comparison group");
        ms_queue<std::size_t> q; run(q, "unbounded;strict-try;HP-locks+allocation-included");
    } else throw std::invalid_argument("choose one variant: mutex/ring/batch/spsc/spsc-cached/mpsc/mpmc/ms");
} catch (const std::exception& e) {
    std::cerr << "queue_bench: " << e.what() << '\n';
    return 1;
}
