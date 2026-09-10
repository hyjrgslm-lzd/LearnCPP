#pragma once

#include <algorithm>
#include <cstddef>
#include <optional>
#include <vector>

namespace c06_l06 {

class IntMaxHeap {
public:
    explicit IntMaxHeap(std::size_t capacity) : capacity_(capacity) {}

    bool push(int value) {
        if (values_.size() == capacity_) {
            return false;
        }
        values_.push_back(value);
        std::ranges::push_heap(values_);
        return true;
    }

    std::optional<int> peek_max() const {
        if (values_.empty()) {
            return std::nullopt;
        }
        return values_.front();
    }

    std::optional<int> pop_max() {
        if (values_.empty()) {
            return std::nullopt;
        }
        std::ranges::pop_heap(values_);
        int result = values_.back();
        values_.pop_back();
        return result;
    }

    std::size_t size() const { return values_.size(); }
    bool empty() const { return values_.empty(); }

private:
    std::size_t capacity_{};
    std::vector<int> values_;
};

} // namespace c06_l06
