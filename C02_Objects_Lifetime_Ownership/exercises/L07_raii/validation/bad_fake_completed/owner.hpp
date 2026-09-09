#pragma once

#include "../../checks/support/resource_model.hpp"

#include <utility>

namespace l07 {

class two_resource_owner {
public:
    two_resource_owner()
        : first_(l07_support::acquire_first()),
          second_(l07_support::acquire_second()) {}

    ~two_resource_owner() { reset(); }

    two_resource_owner(const two_resource_owner&) = delete;
    two_resource_owner& operator=(const two_resource_owner&) = delete;

    two_resource_owner(two_resource_owner&& other) noexcept
        : first_(std::exchange(other.first_, {})),
          second_(std::exchange(other.second_, {})) {}

    two_resource_owner& operator=(two_resource_owner&& other) noexcept {
        if (this != &other) {
            first_ = std::exchange(other.first_, {});
            second_ = std::exchange(other.second_, {});
        }
        return *this;
    }

    [[nodiscard]] bool owns_first() const noexcept { return first_.owns(); }
    [[nodiscard]] bool owns_second() const noexcept { return second_.owns(); }
    [[nodiscard]] int first_id() const noexcept { return first_.id(); }
    [[nodiscard]] int second_id() const noexcept { return second_.id(); }

    void reset() noexcept {
        if (first_.owns()) {
            l07_support::release_first(std::exchange(first_, {}));
        }
    }

private:
    l07_support::ResourceHandle first_;
    l07_support::ResourceHandle second_;
};

} // namespace l07
