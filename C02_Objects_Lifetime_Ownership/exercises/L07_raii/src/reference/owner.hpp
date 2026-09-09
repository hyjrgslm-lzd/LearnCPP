#pragma once

#include "../../checks/support/resource_model.hpp"

#include <utility>

namespace l07 {

class first_owner {
public:
    first_owner() = default;
    explicit first_owner(l07_support::ResourceHandle handle) noexcept : handle_(handle) {}
    ~first_owner() { reset(); }

    first_owner(const first_owner&) = delete;
    first_owner& operator=(const first_owner&) = delete;

    first_owner(first_owner&& other) noexcept : handle_(std::exchange(other.handle_, {})) {}

    first_owner& operator=(first_owner&& other) noexcept {
        if (this != &other) {
            reset();
            handle_ = std::exchange(other.handle_, {});
        }
        return *this;
    }

    [[nodiscard]] bool owns() const noexcept { return handle_.owns(); }
    [[nodiscard]] int id() const noexcept { return handle_.id(); }

    void reset() noexcept {
        if (handle_.owns()) {
            l07_support::release_first(std::exchange(handle_, {}));
        }
    }

private:
    l07_support::ResourceHandle handle_;
};

class second_owner {
public:
    second_owner() = default;
    explicit second_owner(l07_support::ResourceHandle handle) noexcept : handle_(handle) {}
    ~second_owner() { reset(); }

    second_owner(const second_owner&) = delete;
    second_owner& operator=(const second_owner&) = delete;

    second_owner(second_owner&& other) noexcept : handle_(std::exchange(other.handle_, {})) {}

    second_owner& operator=(second_owner&& other) noexcept {
        if (this != &other) {
            reset();
            handle_ = std::exchange(other.handle_, {});
        }
        return *this;
    }

    [[nodiscard]] bool owns() const noexcept { return handle_.owns(); }
    [[nodiscard]] int id() const noexcept { return handle_.id(); }

    void reset() noexcept {
        if (handle_.owns()) {
            l07_support::release_second(std::exchange(handle_, {}));
        }
    }

private:
    l07_support::ResourceHandle handle_;
};

class two_resource_owner {
public:
    two_resource_owner()
        : first_(l07_support::acquire_first()),
          second_(l07_support::acquire_second()) {}

    ~two_resource_owner() = default;

    two_resource_owner(const two_resource_owner&) = delete;
    two_resource_owner& operator=(const two_resource_owner&) = delete;
    two_resource_owner(two_resource_owner&&) noexcept = default;

    two_resource_owner& operator=(two_resource_owner&& other) noexcept {
        if (this != &other) {
            reset();
            first_ = std::move(other.first_);
            second_ = std::move(other.second_);
        }
        return *this;
    }

    [[nodiscard]] bool owns_first() const noexcept { return first_.owns(); }
    [[nodiscard]] bool owns_second() const noexcept { return second_.owns(); }
    [[nodiscard]] int first_id() const noexcept { return first_.id(); }
    [[nodiscard]] int second_id() const noexcept { return second_.id(); }

    void reset() noexcept {
        second_.reset();
        first_.reset();
    }

private:
    first_owner first_;
    second_owner second_;
};

} // namespace l07
