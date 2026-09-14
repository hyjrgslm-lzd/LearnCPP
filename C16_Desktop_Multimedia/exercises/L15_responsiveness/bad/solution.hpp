#pragma once
#include <QMetaObject>
#include <QObject>
#include <cstdint>
#include <functional>
#include <mutex>
#include <utility>
namespace c16_l15 {
class PreviewMailbox final : public QObject {
public:
    explicit PreviewMailbox(std::function<void(std::uint64_t)> apply) : apply_(std::move(apply)) {}
    bool submit(std::uint64_t value) {
        const std::lock_guard lock(mutex_);
        if (closed_) return false;
        if (!pending_) {
            pending_ = true;
            ++posted_;
            // Deliberate error: captures the first value, discards newer previews.
            QMetaObject::invokeMethod(this, [this, value] {
                { const std::lock_guard lock(mutex_); pending_ = false; if (closed_) return; }
                apply_(value);
            }, Qt::QueuedConnection);
        }
        return true;
    }
    void close() { const std::lock_guard lock(mutex_); closed_ = true; }
    std::uint64_t posted() const { const std::lock_guard lock(mutex_); return posted_; }
private:
    std::function<void(std::uint64_t)> apply_;
    mutable std::mutex mutex_;
    std::uint64_t posted_ = 0;
    bool pending_ = false, closed_ = false;
};
}
