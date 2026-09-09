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
        values_.reserve(values.size());
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
        std::vector<TrackedValue> next;
        next.reserve(values_.size());
        for (const TrackedValue& value : source) {
            next.push_back(value);
        }
        for (std::size_t i = source.size(); i < values_.size(); ++i) {
            next.push_back(values_[i]);
        }
        values_.swap(next);
    }

    void replace_all_strong(std::span<const TrackedValue> source)
    {
        std::vector<TrackedValue> next;
        next.reserve(source.size());
        for (const TrackedValue& value : source) {
            next.push_back(value);
        }
        values_.swap(next);
    }

    void swap(Table& other) noexcept { values_.swap(other.values_); }

private:
    std::vector<TrackedValue> values_;
};

} // namespace l06
