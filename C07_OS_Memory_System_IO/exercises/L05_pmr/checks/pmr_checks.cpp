#include <pmr_lab.hpp>

#include <check.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory_resource>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct throwing_type {
    static inline int alive = 0;
    static inline int destroyed = 0;
    static inline int throw_value = -1;
    int value{};

    explicit throwing_type(int next) : value(next) {
        if (next == throw_value) throw std::runtime_error("planned construction failure");
        ++alive;
    }

    throwing_type(const throwing_type&) = delete;
    throwing_type& operator=(const throwing_type&) = delete;

    ~throwing_type() noexcept {
        --alive;
        ++destroyed;
    }

    static void reset() noexcept {
        alive = 0;
        destroyed = 0;
        throw_value = -1;
    }
};

class tracking_upstream : public std::pmr::memory_resource {
public:
    std::size_t allocations{};
    std::size_t deallocations{};
    std::size_t outstanding{};

private:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        void* p = std::pmr::new_delete_resource()->allocate(bytes, alignment);
        ++allocations;
        outstanding += bytes;
        return p;
    }

    void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) override {
        ++deallocations;
        outstanding -= bytes;
        std::pmr::new_delete_resource()->deallocate(p, bytes, alignment);
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }
};

void check_raw_slot() {
    throwing_type::reset();
    {
        c07_l05::raw_slot<throwing_type> slot;
        check(!slot.engaged(), "raw slot starts empty");
        throwing_type& item = slot.construct(7);
        check(slot.engaged(), "raw slot commits after successful construct");
        check(&slot.get() == &item && item.value == 7, "raw slot returns the live object");
        slot.destroy();
        check(!slot.engaged(), "raw slot clears engaged after destroy");
        check(throwing_type::alive == 0 && throwing_type::destroyed == 1,
            "raw storage constructs and destroys exactly one live object");
    }

    throwing_type::reset();
    throwing_type::throw_value = 9;
    c07_l05::raw_slot<throwing_type> slot;
    bool threw = false;
    try {
        (void)slot.construct(9);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    check(threw, "throwing constructor is visible");
    check(!slot.engaged(), "failed construct does not commit an object");
    check(throwing_type::alive == 0 && throwing_type::destroyed == 0,
        "failed construct must not destroy an unconstructed object");
}

void check_counting_resource() {
    tracking_upstream upstream;
    c07_l05::counting_resource counted{&upstream};
    {
        std::pmr::vector<std::pmr::string> words{&counted};
        words.emplace_back("long enough string to force a dynamic string allocation through pmr");
        words.emplace_back("another long enough string to avoid small-string-only false positives");
        check(counted.counters().allocations > 1, "nested pmr object uses the supplied resource");
        check(upstream.allocations == counted.counters().allocations, "counting resource forwards to upstream");
    }
    check(counted.counters().allocations == counted.counters().deallocations,
        "counting resource balances allocate/deallocate");
    check(counted.counters().outstanding == 0 && upstream.outstanding == 0,
        "counting resource leaves no outstanding bytes");
    c07_l05::counting_resource other;
    std::pmr::memory_resource& base = counted;
    check(base.is_equal(counted), "memory_resource is equal to itself");
    check(!base.is_equal(other), "memory_resource equality stays identity based");
}

void check_arena() {
    alignas(64) std::array<std::byte, 128> storage{};
    c07_l05::bounded_arena_resource arena{storage};
    void* first = arena.allocate(16, 64);
    const auto used_after_first = arena.high_water();
    bool exhausted = false;
    try {
        (void)arena.allocate(1024, alignof(std::max_align_t));
    } catch (const std::bad_alloc&) {
        exhausted = true;
    }
    check(reinterpret_cast<std::uintptr_t>(first) % 64 == 0, "bounded arena honors alignment");
    check(exhausted, "bounded arena reports exhaustion");
    check(arena.high_water() == used_after_first, "failed arena allocation leaves state unchanged");
    arena.release();
    void* again = arena.allocate(16, 64);
    check(again == first, "arena release resets the bump pointer");
}

void check_pool() {
    struct pool_object {
        int value;
    };
    alignas(64) std::array<std::byte, 192> storage{};
    c07_l05::fixed_pool_resource pool{storage, 32, alignof(std::max_align_t)};
    void* first = pool.allocate(16, alignof(std::max_align_t));
    auto* object = std::construct_at(static_cast<pool_object*>(first), pool_object{42});
    check(object->value == 42, "fixed pool block can hold a user object");
    std::destroy_at(object);
    pool.deallocate(first, 16, alignof(std::max_align_t));
    void* second = pool.allocate(16, alignof(std::max_align_t));
    check(first == second, "fixed pool reuses freed blocks");
    bool oversize_rejected = false;
    try {
        (void)pool.allocate(128, alignof(std::max_align_t));
    } catch (const std::bad_alloc&) {
        oversize_rejected = true;
    }
    check(oversize_rejected, "fixed pool rejects over-size requests");
    pool.deallocate(second, 16, alignof(std::max_align_t));
}

} // namespace

int main() {
    check_raw_slot();
    check_counting_resource();
    check_arena();
    check_pool();

    const auto probe = c07_l05::run_allocation_probe(8);
    check(probe.vector_allocations > 0, "probe records vector allocation count");
    check(probe.arena_high_water > 0, "probe records arena high water");
    check(probe.pool_reuse_count > 0, "probe records fixed-pool reuse");
    std::cout << "L05 pmr checks passed\n";
}
