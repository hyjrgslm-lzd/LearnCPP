#ifndef CONCURRENCY_STUDY_QUEUE_VERSIONS_HPP
#define CONCURRENCY_STUDY_QUEUE_VERSIONS_HPP

#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace cs::queue_lab {

#if defined(CS_QUEUE_DIAGNOSTICS) && !defined(CS_QUEUE_DIAGNOSTIC_SUPPORT_HPP)
#define CS_QUEUE_DIAGNOSTIC_SUPPORT_HPP
struct queue_diagnostic_counters {
    std::atomic<std::size_t> mutex_acquisitions{0};
    std::atomic<std::size_t> spsc_remote_loads{0};
};

struct queue_diagnostic_snapshot {
    std::size_t mutex_acquisitions = 0;
    std::size_t spsc_remote_loads = 0;
};

inline queue_diagnostic_counters queue_diagnostics;

inline void reset_queue_diagnostics() noexcept {
    queue_diagnostics.mutex_acquisitions.store(0, std::memory_order_relaxed);
    queue_diagnostics.spsc_remote_loads.store(0, std::memory_order_relaxed);
}

inline queue_diagnostic_snapshot read_queue_diagnostics() noexcept {
    return {
        queue_diagnostics.mutex_acquisitions.load(std::memory_order_relaxed),
        queue_diagnostics.spsc_remote_loads.load(std::memory_order_relaxed),
    };
}

inline void note_mutex_acquired() noexcept {
    queue_diagnostics.mutex_acquisitions.fetch_add(1, std::memory_order_relaxed);
}

inline void note_spsc_remote_load() noexcept {
    queue_diagnostics.spsc_remote_loads.fetch_add(1, std::memory_order_relaxed);
}
#elif !defined(CS_QUEUE_DIAGNOSTIC_SUPPORT_HPP)
#define CS_QUEUE_DIAGNOSTIC_SUPPORT_HPP
inline void note_mutex_acquired() noexcept {}
inline void note_spsc_remote_load() noexcept {}
#endif

// All versions: stop and join participants before destruction. Output objects
// belong to the caller. No concurrent access to the same input/output object.
template<class T>
class mutex_ring {
    static_assert(std::is_nothrow_copy_assignable_v<T>);
    static_assert(std::is_nothrow_destructible_v<T>);
public:
    explicit mutex_ring(std::size_t capacity) : data_(valid(capacity)) {}
    bool try_push(const T& value) { return push_batch({&value, 1}) == 1; }
    bool try_pop(T& value) { return pop_batch({&value, 1}) == 1; }
    // One critical section; returns the accepted prefix length, possibly zero.
    std::size_t push_batch(std::span<const T> values) {
        std::lock_guard lock(mutex_);
        note_mutex_acquired();
        std::size_t n = 0;
        while (n < values.size() && size_ < data_.size()) {
            data_[(head_ + size_) % data_.size()] = values[n++];
            ++size_;
        }
        return n;
    }
    std::size_t pop_batch(std::span<T> values) {
        std::lock_guard lock(mutex_);
        note_mutex_acquired();
        std::size_t n = 0;
        while (n < values.size() && size_ != 0) {
            // const container access also handles vector<bool>'s value proxy.
            values[n++] = std::as_const(data_)[head_];
            head_ = (head_ + 1) % data_.size();
            --size_;
        }
        return n;
    }
private:
    static std::size_t valid(std::size_t n) {
        if (!n || n > std::numeric_limits<std::size_t>::max() / 2)
            throw std::invalid_argument("invalid ring capacity");
        return n;
    }
    std::vector<T> data_;
    std::mutex mutex_;
    std::size_t head_ = 0, size_ = 0;
};

// capacity is usable capacity; allocate one extra slot. Cached=true reduces
// remote-index reads, without changing the one-producer/one-consumer contract.
template<class T, bool Cached = false>
class spsc_ring {
    static_assert(std::is_nothrow_copy_assignable_v<T>);
    static_assert(std::is_nothrow_destructible_v<T>);
public:
    explicit spsc_ring(std::size_t capacity)
        : storage_size_(storage(capacity)), data_(std::make_unique<T[]>(storage_size_)) {}
    bool try_push(const T& value) noexcept {
        const auto tail = tail_.load(std::memory_order_relaxed);
        const auto next = advance(tail);
        if constexpr (Cached) {
            if (next == cached_head_) {
                note_spsc_remote_load();
                cached_head_ = head_.load(std::memory_order_acquire);
                if (next == cached_head_) return false;
            }
        } else {
            note_spsc_remote_load();
            if (next == head_.load(std::memory_order_acquire)) return false;
        }
        data_[tail] = value;
        tail_.store(next, std::memory_order_release);
        return true;
    }
    bool try_pop(T& value) noexcept {
        const auto head = head_.load(std::memory_order_relaxed);
        if constexpr (Cached) {
            if (head == cached_tail_) {
                note_spsc_remote_load();
                cached_tail_ = tail_.load(std::memory_order_acquire);
                if (head == cached_tail_) return false;
            }
        } else {
            note_spsc_remote_load();
            if (head == tail_.load(std::memory_order_acquire)) return false;
        }
        value = std::as_const(data_[head]);
        head_.store(advance(head), std::memory_order_release);
        return true;
    }
    bool atomics_lock_free() const noexcept {
        return head_.is_lock_free() && tail_.is_lock_free();
    }
private:
    static std::size_t storage(std::size_t n) {
        if (!n || n == std::numeric_limits<std::size_t>::max())
            throw std::invalid_argument("invalid SPSC usable capacity");
        return n + 1;
    }
    std::size_t advance(std::size_t p) const noexcept {
        return p + 1 == storage_size_ ? 0 : p + 1;
    }
    const std::size_t storage_size_;
    std::unique_ptr<T[]> data_; // real separate objects, including T=bool
    std::atomic<std::size_t> head_{0}, tail_{0};
    std::size_t cached_head_ = 0; // producer only
    std::size_t cached_tail_ = 0; // consumer only
};

// Modular ordering, meaningful only for distances strictly below half range.
// uint8_t is intentionally supported for the executable wraparound model.
template<class Counter>
constexpr bool sequence_behind(Counter observed, Counter expected) noexcept {
    static_assert(std::is_unsigned_v<Counter>);
    constexpr auto half = Counter(Counter{1} << (std::numeric_limits<Counter>::digits - 1));
    return Counter(observed - expected) >= half;
}

// Vyukov-style reservation/sequence protocol. SingleConsumer removes the
// dequeue CAS, producing a bounded MPSC specialization. false may be transient:
// it is NOT proof of abstract empty/full and this is NOT a strict lock-free FIFO.
// Precondition: no stale operation/sequence observation spans half a counter
// range of progress; no outstanding snapshot survives a complete counter cycle.
template<class T, bool SingleConsumer = false, class Counter = std::uint64_t>
class sequence_ring {
    static_assert(std::is_unsigned_v<Counter> && !std::is_same_v<Counter, bool>);
    static_assert(std::is_nothrow_copy_assignable_v<T>);
    static_assert(std::is_nothrow_destructible_v<T>);
    struct cell { std::atomic<Counter> sequence{0}; T data{}; };
public:
    explicit sequence_ring(std::size_t capacity)
        : capacity_(valid(capacity)), cells_(std::make_unique<cell[]>(capacity_)) {
        for (std::size_t i = 0; i < capacity_; ++i)
            cells_[i].sequence.store(Counter(i), std::memory_order_relaxed);
    }
    // Test-only hook runs after reservation, before publication. A noexcept
    // callable is mandatory: abandoning an owned ticket would leave a hole.
    bool try_push(const T& value) noexcept { return try_push(value, []() noexcept {}); }
    template<class Hook>
    bool try_push(const T& value, Hook reserved) noexcept {
        static_assert(std::is_nothrow_invocable_v<Hook>);
        Counter pos = enqueue_.load(std::memory_order_relaxed);
        cell* slot;
        for (;;) {
            slot = &cells_[std::size_t(pos) & (capacity_ - 1)];
            const auto seq = slot->sequence.load(std::memory_order_acquire);
            if (seq == pos) {
                if (enqueue_.compare_exchange_weak(pos, Counter(pos + 1),
                                                   std::memory_order_relaxed)) break;
            } else if (sequence_behind(seq, pos)) return false;
            else pos = enqueue_.load(std::memory_order_relaxed);
        }
        reserved();
        slot->data = value;
        slot->sequence.store(Counter(pos + 1), std::memory_order_release);
        return true;
    }
    bool try_pop(T& value) noexcept {
        Counter pos = dequeue_.load(std::memory_order_relaxed);
        cell* slot;
        for (;;) {
            slot = &cells_[std::size_t(pos) & (capacity_ - 1)];
            const auto expected = Counter(pos + 1);
            const auto seq = slot->sequence.load(std::memory_order_acquire);
            if (seq == expected) {
                if constexpr (SingleConsumer) {
                    dequeue_.store(Counter(pos + 1), std::memory_order_relaxed);
                    break;
                } else if (dequeue_.compare_exchange_weak(pos, Counter(pos + 1),
                                                         std::memory_order_relaxed)) break;
            } else if (sequence_behind(seq, expected)) return false;
            else pos = dequeue_.load(std::memory_order_relaxed);
        }
        value = std::as_const(slot->data);
        slot->sequence.store(Counter(pos + capacity_), std::memory_order_release);
        return true;
    }
    bool atomics_lock_free() const noexcept {
        return enqueue_.is_lock_free() && dequeue_.is_lock_free()
            && cells_[0].sequence.is_lock_free();
    }
private:
    static std::size_t valid(std::size_t n) {
        if (n < 2 || !std::has_single_bit(n)
            || n > std::numeric_limits<Counter>::max() / 2)
            throw std::invalid_argument("capacity must be power of two >=2 and < counter half range");
        return n;
    }
    const std::size_t capacity_;
    std::unique_ptr<cell[]> cells_;
    std::atomic<Counter> enqueue_{0}, dequeue_{0};
};
template<class T> using mpsc_ring = sequence_ring<T, true>;
template<class T> using mpmc_ring = sequence_ring<T, false>;
} // namespace cs::queue_lab
#endif
