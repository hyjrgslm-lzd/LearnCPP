#pragma once

#include "resource_model.hpp"

namespace l07 {

class two_resource_owner {
public:
    two_resource_owner() = default;
    two_resource_owner(const two_resource_owner&) = delete;
    two_resource_owner& operator=(const two_resource_owner&) = delete;
    two_resource_owner(two_resource_owner&&) noexcept = default;
    two_resource_owner& operator=(two_resource_owner&&) noexcept = default;

    [[nodiscard]] bool owns_first() const noexcept { return true; }
    [[nodiscard]] bool owns_second() const noexcept { return true; }
    [[nodiscard]] int first_id() const noexcept { return 1; }
    [[nodiscard]] int second_id() const noexcept { return 2; }
    void reset() noexcept {}
};

} // namespace l07
