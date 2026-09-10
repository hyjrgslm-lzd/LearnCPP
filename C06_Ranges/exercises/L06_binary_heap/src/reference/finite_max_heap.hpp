#pragma once

#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

namespace c06_l06 {

class IntMaxHeap {
public:
    explicit IntMaxHeap(std::size_t capacity) : capacity_(capacity) {
        data_.reserve(capacity);
    }

    bool push(int value) {
        if (data_.size() == capacity_) {
            return false;
        }
        data_.push_back(value);
        sift_up(data_.size() - 1);
        return true;
    }

    std::optional<int> peek_max() const {
        if (data_.empty()) {
            return std::nullopt;
        }
        return data_.front();
    }

    std::optional<int> pop_max() {
        if (data_.empty()) {
            return std::nullopt;
        }
        int result = data_.front();
        data_.front() = data_.back();
        data_.pop_back();
        if (!data_.empty()) {
            sift_down(0);
        }
        return result;
    }

    std::size_t size() const {
        return data_.size();
    }

    bool empty() const {
        return data_.empty();
    }

private:
    void sift_up(std::size_t index) {
        while (index != 0) {
            auto parent = (index - 1) / 2;
            if (data_[parent] >= data_[index]) {
                break;
            }
            std::swap(data_[parent], data_[index]);
            index = parent;
        }
    }

    void sift_down(std::size_t index) {
        while (true) {
            auto left = index * 2 + 1;
            auto right = left + 1;
            auto largest = index;
            if (left < data_.size() && data_[largest] < data_[left]) {
                largest = left;
            }
            if (right < data_.size() && data_[largest] < data_[right]) {
                largest = right;
            }
            if (largest == index) {
                break;
            }
            std::swap(data_[index], data_[largest]);
            index = largest;
        }
    }

    std::size_t capacity_{};
    std::vector<int> data_;
};

} // namespace c06_l06
