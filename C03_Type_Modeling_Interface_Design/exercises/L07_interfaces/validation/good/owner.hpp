#pragma once

#include <initializer_list>
#include <span>
#include <vector>

namespace l07 {

class Owner {
public:
    Owner() = default;

    Owner(std::initializer_list<int> values)
    {
        values_.assign(values.begin(), values.end());
    }

    std::span<const int> view() const& noexcept { return {values_.data(), values_.size()}; }
    std::span<const int> view() && = delete;

    std::vector<int> snapshot() const
    {
        std::vector<int> copy;
        copy.reserve(values_.size());
        for (int value : values_) {
            copy.push_back(value);
        }
        return copy;
    }

    void replace_all(std::vector<int> values) noexcept
    {
        values_.swap(values);
    }

private:
    std::vector<int> values_;
};

} // namespace l07

