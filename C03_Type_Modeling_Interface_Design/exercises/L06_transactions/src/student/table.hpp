#pragma once

#include <tracked_value.hpp>

#include <initializer_list>
#include <span>
#include <vector>

namespace l06 {

class Table {
public:
    Table(std::initializer_list<int> values)
    {
        for (int value : values) {
            values_.emplace_back(value);
        }
    }

    std::span<const TrackedValue> view() const noexcept { return values_; }
    void replace_prefix_basic(std::span<const TrackedValue>) {}
    void replace_prefix_strong(std::span<const TrackedValue>) {}
    void replace_all_strong(std::span<const TrackedValue>) {}
    void swap(Table&) noexcept {}

private:
    std::vector<TrackedValue> values_;
};

} // namespace l06

