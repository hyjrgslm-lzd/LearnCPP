#ifndef CONCURRENCY_STUDY_QUEUE_CHECKS_HPP
#define CONCURRENCY_STUDY_QUEUE_CHECKS_HPP
#include "exercise_check.hpp"
#include "queue_versions.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <exception>
#include <future>
#include <numeric>
#include <thread>
#include <vector>

namespace cs::queue_lab {
// Serial regression probe: the trait promises assignment from const T&, not T&.
struct copy_overload_probe {
    inline static int live = 0;
    inline static int mutable_assignments = 0;
    int id = 0;
    copy_overload_probe() noexcept { ++live; }
    copy_overload_probe(const copy_overload_probe& other) noexcept : id(other.id) { ++live; }
    copy_overload_probe& operator=(const copy_overload_probe& other) noexcept {
        id = other.id;
        return *this;
    }
    copy_overload_probe& operator=(copy_overload_probe&) {
        ++mutable_assignments;
        throw std::runtime_error("selected throwing assignment from mutable source");
    }
    ~copy_overload_probe() { --live; }
};
static_assert(std::is_nothrow_copy_assignable_v<copy_overload_probe>);
static_assert(!std::is_nothrow_assignable_v<copy_overload_probe&, copy_overload_probe&>);

template<class MakeQueue>
void check_const_copy(MakeQueue make_queue) {
    cs::check(copy_overload_probe::live == 0, "previous probe storage released");
    copy_overload_probe::mutable_assignments = 0;
    {
        auto queue = make_queue();
        copy_overload_probe input, output;
        output.id = 99;
        cs::check(!queue.try_pop(output) && output.id == 99, "empty preserves overloaded output");
        for (int id = 0; id < 4; ++id) {
            input.id = id;
            cs::check(queue.try_push(input) && queue.try_pop(output) && output.id == id,
                      "push/pop copy through const overload across slot reuse");
        }
        if constexpr (requires { queue.pop_batch(std::span<copy_overload_probe>{}); }) {
            std::array<copy_overload_probe, 3> in, out;
            for (int i = 0; i < 3; ++i) { in[i].id = 10 + i; out[i].id = 99; }
            cs::check(queue.push_batch(in) == 2, "overloaded batch accepts capacity-two prefix");
            cs::check(queue.pop_batch(out) == 2 && out[0].id == 10 && out[1].id == 11
                      && out[2].id == 99, "const batch copy, untouched suffix");
            cs::check(queue.push_batch(std::span<const copy_overload_probe>{in}.subspan(2)) == 1,
                      "retry unaccepted batch suffix");
            cs::check(queue.try_pop(output) && output.id == 12, "batch-to-single copy");
        }
        cs::check(queue.try_push(input), "destructor handles a remaining payload");
    } // Linked queue destructors also drain their retired nodes.
    cs::check(copy_overload_probe::mutable_assignments == 0, "never call mutable-source assignment");
    cs::check(copy_overload_probe::live == 0, "all overloaded payloads reclaimed");
}

template<bool Cached>
void check_spsc_bool(std::size_t capacity) {
    spsc_ring<bool, Cached> queue(capacity);
    const auto bit = [](std::size_t i) { return ((i ^ (i >> 3) ^ (i >> 7)) & 1) != 0; };
    bool output = true;
    cs::check(!queue.try_pop(output) && output, "bool empty preserves output");
    for (std::size_t i = 0; i < capacity; ++i) cs::check(queue.try_push(bit(i)), "fill bool slots");
    cs::check(!queue.try_push(true), "bool full");
    for (std::size_t i = 0; i < capacity; ++i)
        cs::check(queue.try_pop(output) && output == bit(i), "bool sequential FIFO");
    constexpr std::size_t count = 20003;
    std::atomic<bool> cancel{false};
    // This worker uses only noexcept bool queue/atomic operations; checks and
    // potentially throwing code run in the caller. Failure cancels and joins it.
    std::jthread producer([&]() noexcept {
        for (std::size_t i = 0; i < count; ++i) {
            while (!queue.try_push(bit(i))) {
                if (cancel.load(std::memory_order_relaxed)) return;
                std::this_thread::yield();
            }
        }
    });
    try {
        for (std::size_t i = 0; i < count; ++i) {
            while (!queue.try_pop(output)) std::this_thread::yield();
            cs::check(output == bit(i), "bool concurrent FIFO across adjacent slot reuse");
        }
    } catch (...) {
        cancel.store(true, std::memory_order_relaxed);
        throw; // jthread joins before cancel/queue are destroyed
    }
    producer.join();
    cs::check(!queue.try_pop(output), "bool stream drained");
}

struct transfer_result {
    std::size_t completed = 0;
    std::vector<std::vector<std::size_t>> received;
};

// Small queue-specific driver shared by References and the benchmark. Producers
// own contiguous ID ranges. No shared per-element counter. Timing callers may
// disable trace collection; correctness runs retain every ID.
template<class Queue>
transfer_result transfer(Queue& queue, std::size_t size, std::size_t producers,
                         std::size_t consumers, bool record = true,
                         std::size_t batch = 1) {
    cs::check(producers > 0 && consumers > 0 && batch > 0, "positive roles and batch");
    transfer_result result;
    result.received.resize(consumers);
    std::vector<std::size_t> counts(consumers);
    std::vector<std::exception_ptr> errors(producers + consumers);
    std::atomic<bool> cancel{false};
    std::atomic<std::size_t> done{0};
    std::promise<void> release;
    auto start = release.get_future().share();
    std::vector<std::jthread> workers;
    workers.reserve(producers + consumers);
    auto launch = [&](std::size_t index, auto action) {
        workers.emplace_back([&, index, action, start] {
            start.wait();
            try { if (!cancel.load()) action(); }
            catch (...) {
                errors[index] = std::current_exception();
                cancel.store(true);
            }
        });
    };
    try {
        for (std::size_t p = 0; p < producers; ++p) {
            launch(p, [&, p] {
                const auto count = size / producers + (p < size % producers ? 1 : 0);
                const auto begin = p * (size / producers) + std::min(p, size % producers);
                std::vector<std::size_t> values(batch);
                std::size_t sent = 0;
                while (sent < count && !cancel.load(std::memory_order_relaxed)) {
                    std::size_t accepted;
                    if constexpr (requires { queue.push_batch(std::span<const std::size_t>{}); }) {
                        const auto n = std::min(batch, count - sent);
                        std::iota(values.begin(), values.begin() + n, begin + sent);
                        accepted = queue.push_batch({values.data(), n});
                    } else accepted = queue.try_push(begin + sent) ? 1 : 0;
                    sent += accepted;
                    if (!accepted) std::this_thread::yield();
                }
                done.fetch_add(1, std::memory_order_release); // once per producer
            });
        }
        for (std::size_t c = 0; c < consumers; ++c) {
            launch(producers + c, [&, c] {
                std::vector<std::size_t> values(batch);
                auto receive = [&] {
                    std::size_t n;
                    if constexpr (requires { queue.pop_batch(std::span<std::size_t>{}); })
                        n = queue.pop_batch(values);
                    else n = queue.try_pop(values[0]) ? 1 : 0;
                    counts[c] += n;
                    if (record) result.received[c].insert(result.received[c].end(),
                                                        values.begin(), values.begin() + n);
                    return n;
                };
                while (!cancel.load(std::memory_order_relaxed)) {
                    if (receive()) continue;
                    // Acquire completion BEFORE the final queue observation.
                    if (done.load(std::memory_order_acquire) == producers && !receive()) break;
                    std::this_thread::yield();
                }
            });
        }
    } catch (...) {
        cancel.store(true);
        release.set_value();
        throw; // jthreads join while their dependencies still exist
    }
    release.set_value();
    workers.clear();
    for (const auto& error : errors) if (error) std::rethrow_exception(error);
    result.completed = std::accumulate(counts.begin(), counts.end(), std::size_t{0});
    cs::check(result.completed == size, "successful received element count");
    if (record) {
        std::vector<std::size_t> all;
        for (const auto& part : result.received) all.insert(all.end(), part.begin(), part.end());
        std::sort(all.begin(), all.end());
        for (std::size_t id = 0; id < size; ++id) cs::check(all[id] == id, "unique exact ID set");
    }
    return result;
}

inline void check_producer_order(const transfer_result& result, std::size_t size,
                                 std::size_t producers) {
    cs::check(result.received.size() == 1, "order trace uses one consumer");
    std::vector<std::size_t> next(producers), end(producers);
    for (std::size_t p = 0; p < producers; ++p) {
        next[p] = p * (size / producers) + std::min(p, size % producers);
        end[p] = next[p] + size / producers + (p < size % producers ? 1 : 0);
    }
    for (auto id : result.received[0]) {
        bool matched = false;
        for (std::size_t p = 0; p < producers; ++p) {
            if (next[p] < end[p] && id == next[p]) { ++next[p]; matched = true; break; }
        }
        cs::check(matched, "each producer's nonoverlapping pushes remain ordered");
    }
}

template<class Queue>
void check_capacity(Queue& q, std::size_t capacity) {
    std::size_t value = 99;
    cs::check(!q.try_pop(value) && value == 99, "empty preserves output");
    for (std::size_t i = 0; i < capacity; ++i) cs::check(q.try_push(i), "fill usable capacity");
    cs::check(!q.try_push(88), "reject full without changing queue");
    for (std::size_t i = 0; i < capacity; ++i)
        cs::check(q.try_pop(value) && value == i, "sequential FIFO");
    cs::check(!q.try_pop(value), "drained");
}

template<class Queue>
void check_reservation_gap() {
    Queue q(4);
    std::atomic<bool> reserved{false}, resume{false};
    bool first = false;
    std::jthread producer([&] {
        // Only noexcept queue operations and atomics in this worker.
        first = q.try_push(10, [&]() noexcept {
            reserved.store(true, std::memory_order_release);
            while (!resume.load(std::memory_order_acquire)) std::this_thread::yield();
        });
    });
    while (!reserved.load(std::memory_order_acquire)) std::this_thread::yield();
    const bool second = q.try_push(20); // completes behind the unpublished ticket
    std::size_t value = 77;
    const bool popped = q.try_pop(value);
    const auto preserved = value;
    resume.store(true, std::memory_order_release); // always release before any check can throw
    producer.join();
    cs::check(first && second && !popped && preserved == 77,
              "completed later enqueue does not close earlier publication gap");
    cs::check(q.try_pop(value) && value == 10, "first reserved ticket first");
    cs::check(q.try_pop(value) && value == 20, "second ticket follows");
    cs::check(!q.try_pop(value), "gap scenario drained");
}

inline void check_narrow_wrap() {
    // Sixteen physical wraps: sequential operations satisfy the stale-snapshot
    // bound. This does not license an arbitrarily stalled concurrent snapshot.
    sequence_ring<std::size_t, false, std::uint8_t> q(4);
    for (std::size_t i = 0; i < 4096; ++i) {
        std::size_t value = 0;
        cs::check(q.try_push(i) && q.try_pop(value) && value == i, "8-bit ring wrap");
    }
    for (unsigned base = 0; base < 256; ++base) {
        for (unsigned distance = 1; distance < 128; ++distance) {
            const auto a = std::uint8_t(base), b = std::uint8_t(base + distance);
            cs::check(sequence_behind(a, b) && !sequence_behind(b, a), "modular half-range order");
        }
    }
    std::uint8_t ticket = 0;
    const auto stale = ticket;
    for (unsigned step = 0; step < 256; ++step) ++ticket;
    cs::check(ticket == stale, "full-cycle stale ticket aliases");
}
} // namespace cs::queue_lab
#endif
