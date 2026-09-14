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
        while (!pending_.empty()) {
            auto task = std::move(pending_.front().task);
            pending_.pop_front();
            task();
        }
    }
    void clear() { pending_.clear(); }
private:
    struct Item { QString name; std::function<void()> task; };
    std::deque<Item> pending_;
};
}
