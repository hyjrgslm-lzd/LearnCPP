#pragma once

#include <tracked_value.hpp>

#include <initializer_list>
#include <span>
#include <stdexcept>
#include <vector>

namespace l06 {

class Table {
public:
    Table() = default;

    Table(std::initializer_list<int> values)
    {
        for (int value : values) {
            values_.emplace_back(value);
        }
    }

    std::span<const TrackedValue> view() const noexcept { return values_; }

    void replace_prefix_basic(std::span<const TrackedValue> source)
    {
        if (source.size() > values_.size()) {
            throw std::out_of_range("prefix too large");
        }
        for (std::size_t i = 0; i < source.size(); ++i) {
            values_[i] = source[i];
        }
    }

    void replace_prefix_strong(std::span<const TrackedValue> source)
    {
        if (source.size() > values_.size()) {
            throw std::out_of_range("prefix too large");
        }
        auto next = values_;
        for (std::size_t i = 0; i < source.size(); ++i) {
            next[i] = source[i];
        }
        values_.swap(next);
    }

    void replace_all_strong(std::span<const TrackedValue> source)
    {
        if (source.data() == values_.data()) {
            return;
        }
        values_.clear();
        for (const TrackedValue& value : source) {
            values_.push_back(value);
        }
    }

    void swap(Table& other) noexcept { values_.swap(other.values_); }

private:
    std::vector<TrackedValue> values_;
};

} // namespace l06
