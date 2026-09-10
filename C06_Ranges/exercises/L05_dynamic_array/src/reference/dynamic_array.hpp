#pragma once

#include <algorithm>
#include <memory>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace c06_l05 {

template <class T>
class dynamic_array {
    static_assert(!std::is_const_v<T> && !std::is_volatile_v<T>);
    static_assert(std::is_nothrow_destructible_v<T>);
    static_assert(std::is_copy_constructible_v<T> || std::is_nothrow_move_constructible_v<T>,
        "T must be copy constructible or nothrow move constructible");

    using allocator_type = std::allocator<T>;
    using traits = std::allocator_traits<allocator_type>;

public:
    dynamic_array() = default;
    dynamic_array(const dynamic_array&) = delete;
    dynamic_array& operator=(const dynamic_array&) = delete;

    dynamic_array(dynamic_array&& other) noexcept
        : data_(std::exchange(other.data_, nullptr)),
          size_(std::exchange(other.size_, 0)),
          capacity_(std::exchange(other.capacity_, 0)) {}

    dynamic_array& operator=(dynamic_array&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        release_storage();
        data_ = std::exchange(other.data_, nullptr);
        size_ = std::exchange(other.size_, 0);
        capacity_ = std::exchange(other.capacity_, 0);
        return *this;
    }

    ~dynamic_array() noexcept {
        release_storage();
    }

    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }
    [[nodiscard]] bool empty() const noexcept { return size_ == 0; }

    T& operator[](std::size_t index) noexcept { return data_[index]; }
    const T& operator[](std::size_t index) const noexcept { return data_[index]; }

    [[nodiscard]] std::span<T> view() noexcept { return {data_, size_}; }
    [[nodiscard]] std::span<const T> view() const noexcept { return {data_, size_}; }

    void reserve(std::size_t requested) {
        if (requested <= capacity_) {
            return;
        }
        if (requested > max_size()) {
            throw std::length_error("dynamic_array capacity too large");
        }

        T* next = traits::allocate(alloc_, requested);
        std::size_t built = 0;
        auto rollback = [this, &built, requested](T* ptr) noexcept {
            destroy_prefix(ptr, built);
            traits::deallocate(alloc_, ptr, requested);
        };
        std::unique_ptr<T, decltype(rollback)> cleanup(next, rollback);

        for (; built != size_; ++built) {
            construct_relocated(next + built, data_[built]);
        }
        cleanup.release();

        destroy_prefix(data_, size_);
        if (data_ != nullptr) {
            traits::deallocate(alloc_, data_, capacity_);
        }
        data_ = next;
        capacity_ = requested;
    }

    void push_back(T value) {
        if (size_ != capacity_) {
            traits::construct(alloc_, data_ + size_, append_source(value));
            ++size_;
            return;
        }
        grow_and_append(value);
    }

    void pop_back() {
        if (size_ == 0) {
            throw std::out_of_range("dynamic_array::pop_back on empty array");
        }
        --size_;
        traits::destroy(alloc_, data_ + size_);
    }

    void clear() noexcept {
        destroy_prefix(data_, size_);
        size_ = 0;
    }

private:
    static std::size_t max_size() noexcept {
        return traits::max_size(allocator_type{});
    }

    std::size_t next_capacity() const {
        if (capacity_ == max_size()) {
            throw std::length_error("dynamic_array capacity too large");
        }
        return std::max<std::size_t>(1, capacity_ > max_size() / 2 ? max_size() : capacity_ * 2);
    }

    static decltype(auto) append_source(T& value) noexcept {
        if constexpr (std::is_nothrow_move_constructible_v<T>) {
            return std::move(value);
        } else {
            return std::as_const(value);
        }
    }

    void construct_relocated(T* target, T& source) {
        if constexpr (std::is_nothrow_move_constructible_v<T>) {
            traits::construct(alloc_, target, std::move(source));
        } else {
            traits::construct(alloc_, target, std::as_const(source));
        }
    }

    void grow_and_append(T& value) {
        const std::size_t old_size = size_;
        const std::size_t requested = next_capacity();
        T* next = traits::allocate(alloc_, requested);
        bool tail_built = false;
        std::size_t prefix_built = 0;
        auto rollback = [this, &prefix_built, &tail_built, old_size, requested](T* ptr) noexcept {
            destroy_prefix(ptr, prefix_built);
            if (tail_built) {
                traits::destroy(alloc_, ptr + old_size);
            }
            traits::deallocate(alloc_, ptr, requested);
        };
        std::unique_ptr<T, decltype(rollback)> cleanup(next, rollback);

        traits::construct(alloc_, next + old_size, append_source(value));
        tail_built = true;
        for (; prefix_built != old_size; ++prefix_built) {
            construct_relocated(next + prefix_built, data_[prefix_built]);
        }
        cleanup.release();

        destroy_prefix(data_, old_size);
        if (data_ != nullptr) {
            traits::deallocate(alloc_, data_, capacity_);
        }
        data_ = next;
        size_ = old_size + 1;
        capacity_ = requested;
    }

    void destroy_prefix(T* first, std::size_t count) noexcept {
        while (count != 0) {
            --count;
            traits::destroy(alloc_, first + count);
        }
    }

    void release_storage() noexcept {
        clear();
        if (data_ != nullptr) {
            traits::deallocate(alloc_, data_, capacity_);
            data_ = nullptr;
            capacity_ = 0;
        }
    }

    allocator_type alloc_{};
    T* data_ = nullptr;
    std::size_t size_ = 0;
    std::size_t capacity_ = 0;
};

} // namespace c06_l05
