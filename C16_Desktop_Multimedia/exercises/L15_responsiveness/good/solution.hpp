#pragma once
#include <QMetaObject>
#include <QObject>
#include <atomic>
#include <cstdint>
#include <functional>
#include <utility>

namespace c16_l15 {
// Independent single-producer control; callers join the producer before destruction.
class PreviewMailbox final : public QObject {
public:
    explicit PreviewMailbox(std::function<void(std::uint64_t)> action) : action_(std::move(action)) {}
    bool submit(std::uint64_t value) {
        if (stopped_.load()) return false;
        value_.store(value);
        if (!queued_.exchange(true)) {
            notifications_.fetch_add(1);
            QMetaObject::invokeMethod(this, [this] {
                queued_.store(false);
                if (!stopped_.load()) action_(value_.load());
            }, Qt::QueuedConnection);
        }
        return true;
    }
    void close() { stopped_.store(true); }
    std::uint64_t posted() const { return notifications_.load(); }
private:
    std::function<void(std::uint64_t)> action_;
    std::atomic<std::uint64_t> value_{0}, notifications_{0};
    std::atomic<bool> queued_{false}, stopped_{false};
};
}
