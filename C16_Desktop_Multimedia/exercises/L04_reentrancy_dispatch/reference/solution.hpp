#pragma once
#include <QString>
#include <deque>
#include <functional>

namespace c16_l04 {
class DispatchQueue {
public:
    void post(QString name, std::function<void()> task) { pending_.push_back({std::move(name), std::move(task)}); }
    void drain()
    {
        if (draining_) {
            retry_ = true;
            return;
        }
        draining_ = true;
        do {
            retry_ = false;
            const auto initial = pending_.size();
            for (std::size_t i = 0; i < initial && !pending_.empty(); ++i) {
                auto task = std::move(pending_.front().task);
                pending_.pop_front();
                task();
            }
        } while (retry_ && !pending_.empty());
        draining_ = false;
    }
    void clear() { pending_.clear(); }
private:
    struct Item { QString name; std::function<void()> task; };
    std::deque<Item> pending_;
    bool draining_ = false;
    bool retry_ = false;
};
}
