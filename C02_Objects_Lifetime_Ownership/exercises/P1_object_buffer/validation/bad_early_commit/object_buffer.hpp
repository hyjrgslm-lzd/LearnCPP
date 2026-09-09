#pragma once

#include <memory>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace p1 {

template <class T>
class object_buffer {
public:
    object_buffer() = default;
    object_buffer(const object_buffer&) = delete;
    object_buffer& operator=(const object_buffer&) = delete;
    object_buffer(object_buffer&& other) noexcept
        : data_(std::exchange(other.data_, nullptr)),
          size_(std::exchange(other.size_, 0)),
          capacity_(std::exchange(other.capacity_, 0))
    {
    }
    object_buffer& operator=(object_buffer&& other) noexcept
    {
        if (this != &other) {
            clear();
            if (data_) {
                alloc_.deallocate(data_, capacity_);
            }
            data_ = std::exchange(other.data_, nullptr);
            size_ = std::exchange(other.size_, 0);
            capacity_ = std::exchange(other.capacity_, 0);
        }
        return *this;
    }
    ~object_buffer() noexcept
    {
        clear();
        if (data_) {
            alloc_.deallocate(data_, capacity_);
        }
    }

    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }
    [[nodiscard]] std::span<const T> view() const noexcept { return {data_, size_}; }

    void reserve(std::size_t requested)
    {
        if (requested <= capacity_) {
            return;
        }
        T* next = alloc_.allocate(requested);
        std::size_t built = 0;
        try {
            for (; built != size_; ++built) {
                if constexpr (std::is_nothrow_move_constructible_v<T>) {
                    std::construct_at(next + built, std::move(data_[built]));
                } else {
                    std::construct_at(next + built, std::as_const(data_[built]));
                }
            }
        } catch (...) {
            while (built != 0) {
                --built;
                std::destroy_at(next + built);
            }
            alloc_.deallocate(next, requested);
            throw;
        }
        clear();
        if (data_) {
            alloc_.deallocate(data_, capacity_);
        }
        data_ = next;
        size_ = built;
        capacity_ = requested;
    }

    void push_back(T value)
    {
        if (size_ == capacity_) {
            const auto old_data = data_;
            const auto old_size = size_;
            const auto old_capacity = capacity_;
            const auto next_capacity = capacity_ == 0 ? 1 : capacity_ * 2;
            T* next = alloc_.allocate(next_capacity);
            data_ = next;
            capacity_ = next_capacity;
            try {
                if constexpr (std::is_copy_constructible_v<T>) {
                    std::construct_at(data_ + old_size, std::as_const(value));
                } else {
                    std::construct_at(data_ + old_size, std::move(value));
                }
            } catch (...) {
                alloc_.deallocate(next, next_capacity);
                data_ = old_data;
                size_ = old_size;
                capacity_ = next_capacity;
                throw;
            }
            for (std::size_t index = 0; index != old_size; ++index) {
                if constexpr (std::is_nothrow_move_constructible_v<T>) {
                    std::construct_at(data_ + index, std::move(old_data[index]));
                } else {
                    std::construct_at(data_ + index, std::as_const(old_data[index]));
                }
            }
            for (std::size_t index = old_size; index != 0; --index) {
                std::destroy_at(old_data + index - 1);
            }
            if (old_data) {
                alloc_.deallocate(old_data, old_capacity);
            }
            size_ = old_size + 1;
            return;
        }
        if constexpr (std::is_copy_constructible_v<T>) {
            std::construct_at(data_ + size_, std::as_const(value));
        } else {
            std::construct_at(data_ + size_, std::move(value));
        }
        ++size_;
    }

    void pop_back()
    {
        if (size_ == 0) {
            throw std::out_of_range("empty");
        }
        --size_;
        std::destroy_at(data_ + size_);
    }

    void clear() noexcept
    {
        while (size_ != 0) {
            --size_;
            std::destroy_at(data_ + size_);
        }
    }

private:
    std::allocator<T> alloc_{};
    T* data_ = nullptr;
    std::size_t size_ = 0;
    std::size_t capacity_ = 0;
};

} // namespace p1
