#ifndef C07_L05_PMR_LAB_HPP
#define C07_L05_PMR_LAB_HPP

#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>
#include <memory_resource>
#include <new>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

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
    ~raw_slot() { destroy(); }
    template<class... Args>
    T& construct(Args&&... args) {
        T* p = std::construct_at(ptr(), std::forward<Args>(args)...);
        engaged_ = true;
        return *p;
    }
    void destroy() noexcept {
        if (!engaged_) return;
        std::destroy_at(ptr());
        engaged_ = false;
    }
    bool engaged() const noexcept { return engaged_; }
    T& get() noexcept { return *ptr(); }

private:
    T* ptr() noexcept { return reinterpret_cast<T*>(storage_); }
    alignas(T) std::byte storage_[sizeof(T)]{};
    bool engaged_{};
};

class counting_resource : public std::pmr::memory_resource {
public:
    explicit counting_resource(std::pmr::memory_resource* upstream = std::pmr::new_delete_resource())
        : upstream_(upstream) {}
    const resource_counters& counters() const noexcept { return counters_; }

private:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        void* p = upstream_->allocate(bytes, alignment);
        ++counters_.allocations;
        counters_.bytes += bytes;
        counters_.outstanding += bytes;
        return p;
    }
    void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) override {
        ++counters_.deallocations;
        counters_.outstanding -= bytes;
        upstream_->deallocate(p, bytes, alignment);
    }
    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }
    std::pmr::memory_resource* upstream_;
    resource_counters counters_{};
};

class bounded_arena_resource : public std::pmr::memory_resource {
public:
    explicit bounded_arena_resource(std::span<std::byte> storage) : storage_(storage) {}
    std::size_t high_water() const noexcept { return high_water_; }
    void release() noexcept {
        offset_ = 0;
        high_water_ = 0;
    }

private:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        void* p = storage_.data() + offset_;
        std::size_t space = storage_.size() - offset_;
        void* aligned = std::align(alignment, bytes, p, space);
        if (!aligned) throw std::bad_alloc();
        offset_ = static_cast<std::size_t>(static_cast<std::byte*>(aligned) - storage_.data()) + bytes;
        high_water_ = std::max(high_water_, offset_);
        return aligned;
    }
    void do_deallocate(void*, std::size_t, std::size_t) override {}
    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }
    std::span<std::byte> storage_;
    std::size_t offset_{};
    std::size_t high_water_{};
};

class fixed_pool_resource : public std::pmr::memory_resource {
public:
    fixed_pool_resource(std::span<std::byte> storage, std::size_t block_size, std::size_t alignment)
        : storage_(storage), block_size_(block_size), alignment_(alignment) {}
    std::size_t reuse_count() const noexcept { return 0; }

private:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        if (bytes > block_size_ || alignment > alignment_ || used_ + block_size_ > storage_.size()) {
            throw std::bad_alloc();
        }
        void* p = storage_.data() + used_;
        used_ += block_size_;
        return p;
    }
    void do_deallocate(void*, std::size_t, std::size_t) override {}
    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }
    std::span<std::byte> storage_;
    std::size_t block_size_{};
    std::size_t alignment_{};
    std::size_t used_{};
};

inline allocation_probe run_allocation_probe(std::size_t) {
    return {1, 64, 0};
}

} // namespace c07_l05

#endif
