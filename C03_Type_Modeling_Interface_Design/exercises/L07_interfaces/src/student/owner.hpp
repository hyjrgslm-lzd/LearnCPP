#pragma once

#include <initializer_list>
#include <span>
#include <vector>

namespace l07 {

class Owner {
public:
    Owner() = default;
    Owner(std::initializer_list<int> values) : values_(values) {}

    std::span<const int> view() const& noexcept { return values_; }
    std::span<const int> view() && = delete;

    std::vector<int> snapshot() const { return {}; }
    void replace_all(std::vector<int>) noexcept {}

private:
    std::vector<int> values_;
};

} // namespace l07

