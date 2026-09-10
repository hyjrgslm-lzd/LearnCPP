#ifndef CS_QUEUE_DIAGNOSTICS
#define CS_QUEUE_DIAGNOSTICS 1
#endif

#include "concurrency_study/exercise_check.hpp"
#include "concurrency_study/queue_baseline.hpp"
#include "concurrency_study/queue_checks.hpp"

#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <new>
#include <span>
#include <string_view>

namespace {
std::atomic<bool> count_allocations{false};
std::atomic<std::size_t> allocation_calls{0};

void* allocate(std::size_t size) {
    if (count_allocations.load(std::memory_order_relaxed))
        allocation_calls.fetch_add(1, std::memory_order_relaxed);
    if (void* p = std::malloc(size)) return p;
    throw std::bad_alloc{};
}
} // namespace

void* operator new(std::size_t size) { return allocate(size); }
void* operator new[](std::size_t size) { return allocate(size); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

namespace {
struct counters {
    std::atomic<std::size_t> push_calls{0};
    std::atomic<std::size_t> push_success{0};
    std::atomic<std::size_t> push_items{0};
    std::atomic<std::size_t> pop_calls{0};
    std::atomic<std::size_t> pop_success{0};
    std::atomic<std::size_t> pop_items{0};
};

template<class Queue>
class single_counted {
public:
    explicit single_counted(std::size_t capacity) : queue_(capacity) {}

    bool try_push(std::size_t value) {
        stats_.push_calls.fetch_add(1, std::memory_order_relaxed);
        const bool ok = queue_.try_push(value);
        if (ok) {
            stats_.push_success.fetch_add(1, std::memory_order_relaxed);
            stats_.push_items.fetch_add(1, std::memory_order_relaxed);
        }
        return ok;
    }

    bool try_pop(std::size_t& value) {
        stats_.pop_calls.fetch_add(1, std::memory_order_relaxed);
        const bool ok = queue_.try_pop(value);
        if (ok) {
            stats_.pop_success.fetch_add(1, std::memory_order_relaxed);
            stats_.pop_items.fetch_add(1, std::memory_order_relaxed);
        }
        return ok;
    }

    const counters& stats() const noexcept { return stats_; }

private:
    Queue queue_;
    counters stats_;
};

template<class Queue>
class batch_counted {
public:
    explicit batch_counted(std::size_t capacity) : queue_(capacity) {}

    std::size_t push_batch(std::span<const std::size_t> values) {
        stats_.push_calls.fetch_add(1, std::memory_order_relaxed);
        const auto n = queue_.push_batch(values);
        if (n) stats_.push_success.fetch_add(1, std::memory_order_relaxed);
        stats_.push_items.fetch_add(n, std::memory_order_relaxed);
        return n;
    }

    std::size_t pop_batch(std::span<std::size_t> values) {
        stats_.pop_calls.fetch_add(1, std::memory_order_relaxed);
        const auto n = queue_.pop_batch(values);
        if (n) stats_.pop_success.fetch_add(1, std::memory_order_relaxed);
        stats_.pop_items.fetch_add(n, std::memory_order_relaxed);
        return n;
    }

    const counters& stats() const noexcept { return stats_; }

private:
    Queue queue_;
    counters stats_;
};

template<class Queue>
std::size_t hot_allocations(std::size_t size, std::size_t capacity) {
    Queue queue(capacity);
    std::size_t value = 0;
    allocation_calls.store(0, std::memory_order_relaxed);
    count_allocations.store(true, std::memory_order_relaxed);
    for (std::size_t i = 0; i < size; ++i) {
        cs::check(queue.try_push(i), "single-thread allocation push");
        cs::check(queue.try_pop(value) && value == i, "single-thread allocation pop");
    }
    count_allocations.store(false, std::memory_order_relaxed);
    return allocation_calls.load(std::memory_order_relaxed);
}

template<class Queue>
void emit(std::string_view name, Queue& queue, std::size_t size, std::size_t producers,
          std::size_t consumers, std::size_t capacity, std::size_t batch,
          std::size_t hot_allocs, std::string_view note) {
    cs::queue_lab::reset_queue_diagnostics();
    const auto result = cs::queue_lab::transfer(queue, size, producers, consumers, false, batch);
    const auto internal = cs::queue_lab::read_queue_diagnostics();
    const auto& s = queue.stats();
    cs::check(s.push_items.load(std::memory_order_relaxed) == size, "all items pushed");
    cs::check(s.pop_items.load(std::memory_order_relaxed) == size, "all items popped");
    std::cout << name << ',' << size << ',' << producers << ',' << consumers << ',' << capacity
              << ',' << batch << ',' << result.completed << ','
              << s.push_calls.load(std::memory_order_relaxed) << ','
              << s.push_success.load(std::memory_order_relaxed) << ','
              << s.push_items.load(std::memory_order_relaxed) << ','
              << s.pop_calls.load(std::memory_order_relaxed) << ','
              << s.pop_success.load(std::memory_order_relaxed) << ','
              << s.pop_items.load(std::memory_order_relaxed) << ','
              << internal.mutex_acquisitions << ','
              << internal.spsc_remote_loads << ','
              << hot_allocs << ',' << note << '\n';
}
} // namespace

int main() try {
    constexpr std::size_t size = 10003;
    constexpr std::size_t capacity = 64;
    std::cout << "variant,size,producers,consumers,capacity,batch,completed,"
                 "push_calls,push_success,push_items,pop_calls,pop_success,pop_items,"
                 "mutex_acquisitions,spsc_remote_loads,hot_operator_new_calls,note\n";
    single_counted<cs::queue_lab::mutex_queue<std::size_t>> mutex{capacity};
    emit("mutex", mutex, size, 3, 4, capacity, 1,
         hot_allocations<cs::queue_lab::mutex_queue<std::size_t>>(size, capacity),
         "strict bounded baseline; lock count is instrumented after acquisition");

    single_counted<cs::queue_lab::mutex_ring<std::size_t>> ring{capacity};
    emit("ring", ring, size, 3, 4, capacity, 1,
         hot_allocations<cs::queue_lab::mutex_ring<std::size_t>>(size, capacity),
         "preallocated storage; construction allocation excluded");

    batch_counted<cs::queue_lab::mutex_ring<std::size_t>> batch{capacity};
    emit("batch8", batch, size, 3, 4, capacity, 8, 0,
         "one public batch call maps to one instrumented critical section");

    single_counted<cs::queue_lab::spsc_ring<std::size_t>> spsc{capacity};
    emit("spsc", spsc, size, 1, 1, capacity, 1, 0,
         "noncached SPSC loads the remote index on every try");

    single_counted<cs::queue_lab::spsc_ring<std::size_t, true>> cached{capacity};
    emit("spsc-cached", cached, size, 1, 1, capacity, 1, 0,
         "cached SPSC loads the remote index only when cache boundary is reached");
} catch (const std::exception& error) {
    std::cerr << "queue_diagnostics: " << error.what() << '\n';
    return 1;
}
