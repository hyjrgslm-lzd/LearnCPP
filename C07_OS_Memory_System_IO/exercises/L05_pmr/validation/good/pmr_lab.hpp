#ifndef C07_L05_PMR_LAB_HPP
#define C07_L05_PMR_LAB_HPP

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <memory_resource>
#include <new>
#include <span>
#include <stdexcept>
#include <string>
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
        if (engaged_) throw std::logic_error("occupied");
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
        const auto next = static_cast<std::size_t>(static_cast<std::byte*>(aligned) - storage_.data()) + bytes;
        offset_ = next;
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
        : block_size_(std::max(block_size, sizeof(node))), alignment_(std::max(alignment, alignof(node))) {
        void* current = storage.data();
        std::size_t space = storage.size();
        while (void* p = std::align(alignment_, block_size_, current, space)) {
            auto* n = std::construct_at(static_cast<node*>(p), node{free_});
            free_ = n;
            current = static_cast<std::byte*>(p) + block_size_;
            space = storage.data() + storage.size() - static_cast<std::byte*>(current);
        }
    }

    std::size_t reuse_count() const noexcept { return reuse_count_; }

private:
    struct node { node* next; };
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        if (bytes > block_size_ || alignment > alignment_ || !free_) throw std::bad_alloc();
        node* n = free_;
        free_ = n->next;
        std::destroy_at(n);
        return n;
    }
    void do_deallocate(void* p, std::size_t, std::size_t) override {
        auto* n = std::construct_at(static_cast<node*>(p), node{free_});
        free_ = n;
        ++reuse_count_;
    }
    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }
    node* free_{};
    std::size_t block_size_{};
    std::size_t alignment_{};
    std::size_t reuse_count_{};
};

inline allocation_probe run_allocation_probe(std::size_t items) {
    allocation_probe out{};
    counting_resource counted;
    {
        std::pmr::vector<int> v{&counted};
        for (std::size_t i = 0; i != items; ++i) v.push_back(static_cast<int>(i));
    }
    out.vector_allocations = counted.counters().allocations;
    alignas(64) std::array<std::byte, 512> arena_storage{};
    bounded_arena_resource arena{arena_storage};
    for (std::size_t i = 0; i != items; ++i) (void)arena.allocate(8, alignof(std::uint64_t));
    out.arena_high_water = arena.high_water();
    alignas(64) std::array<std::byte, 256> pool_storage{};
    fixed_pool_resource pool{pool_storage, 32, alignof(std::max_align_t)};
    void* p = pool.allocate(16, alignof(std::max_align_t));
    pool.deallocate(p, 16, alignof(std::max_align_t));
    p = pool.allocate(16, alignof(std::max_align_t));
    pool.deallocate(p, 16, alignof(std::max_align_t));
    out.pool_reuse_count = pool.reuse_count();
    return out;
}

} // namespace c07_l05

#endif
