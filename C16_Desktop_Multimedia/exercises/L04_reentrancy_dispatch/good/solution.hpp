#pragma once
#include <QString>
#include <deque>
#include <functional>

namespace c16_l04 {
class DispatchQueue {
public:
    void post(QString key, std::function<void()> fn) { queue_.push_back({std::move(key), std::move(fn)}); }
    void drain()
    {
        if (inside_) {
            rerun_ = true;
            return;
        }
        inside_ = true;
        while (!queue_.empty()) {
            rerun_ = false;
            const auto batch = queue_.size();
            for (std::size_t i = 0; i < batch && !queue_.empty(); ++i) {
                auto fn = std::move(queue_.front().fn);
                queue_.pop_front();
                fn();
            }
            if (!rerun_) break;
        }
        inside_ = false;
    }
    void clear() { queue_.clear(); }
private:
    struct Entry { QString key; std::function<void()> fn; };
    std::deque<Entry> queue_;
    bool inside_ = false;
    bool rerun_ = false;
};
}
