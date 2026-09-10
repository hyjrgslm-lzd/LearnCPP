#ifndef C07_L05_PMR_LAB_HPP
#define C07_L05_PMR_LAB_HPP

#include <cstddef>
#include <memory>
#include <memory_resource>
#include <span>
#include <stdexcept>
#include <utility>

namespace c07_l05 {

struct resource_counters {
    std::size_t allocations{};
    std::size_t deallocations{};
    std::size_t bytes{};
    std::size_t outstanding{};
};

struct allocation_probe {
    std::size_t vector_allocations{};
    std::size_t arena_high_water{};
    std::size_t pool_reuse_count{};
};

template<class T>
class raw_slot {
public:
    template<class... Args>
    T& construct(Args&&... args) {
        alignas(T) static std::byte storage[sizeof(T)]{};
        static bool made = false;
        static T* object = nullptr;
        if (!made) {
            object = std::construct_at(reinterpret_cast<T*>(storage), std::forward<Args>(args)...);
            made = true;
        }
        return *object;
    }
    void destroy() noexcept {}
    bool engaged() const noexcept { return false; }
    T& get() noexcept { return *static_cast<T*>(nullptr); }
};

class counting_resource : public std::pmr::memory_resource {
public:
    explicit counting_resource(std::pmr::memory_resource* = std::pmr::new_delete_resource()) {}
    const resource_counters& counters() const noexcept { return counters_; }

private:
    void* do_allocate(std::size_t, std::size_t) override { throw std::bad_alloc(); }
    void do_deallocate(void*, std::size_t, std::size_t) override {}
    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override { return this == &other; }
    resource_counters counters_{};
};

class bounded_arena_resource : public std::pmr::memory_resource {
public:
    explicit bounded_arena_resource(std::span<std::byte>) {}
    std::size_t high_water() const noexcept { return 0; }
    void release() noexcept {}

private:
    void* do_allocate(std::size_t, std::size_t) override { throw std::bad_alloc(); }
    void do_deallocate(void*, std::size_t, std::size_t) override {}
    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override { return this == &other; }
};

class fixed_pool_resource : public std::pmr::memory_resource {
public:
    fixed_pool_resource(std::span<std::byte>, std::size_t, std::size_t) {}
    std::size_t reuse_count() const noexcept { return 0; }

private:
    void* do_allocate(std::size_t, std::size_t) override { throw std::bad_alloc(); }
    void do_deallocate(void*, std::size_t, std::size_t) override {}
    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override { return this == &other; }
};

inline allocation_probe run_allocation_probe(std::size_t) { return {}; }

} // namespace c07_l05

#endif
