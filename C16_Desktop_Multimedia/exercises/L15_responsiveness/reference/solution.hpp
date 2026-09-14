#pragma once
#include <QMetaObject>
#include <QObject>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <utility>

namespace c16_l15 {
// The producer must be joined before destroying this QObject. close() cancels UI delivery.
class PreviewMailbox final : public QObject {
public:
    explicit PreviewMailbox(std::function<void(std::uint64_t)> apply) : apply_(std::move(apply)) {}
    bool submit(std::uint64_t value) {
        const std::lock_guard lock(mutex_);
        if (closed_) return false;
        latest_ = value;
        if (!pending_) {
            pending_ = true;
            ++posted_;
            QMetaObject::invokeMethod(this, [this] {
                std::optional<std::uint64_t> value;
                {
                    const std::lock_guard lock(mutex_);
                    pending_ = false;
                    if (closed_) return;
                    value = std::exchange(latest_, std::nullopt);
                }
                // Never invoke client code under the producer lock: apply may submit again.
                if (value) apply_(*value);
            }, Qt::QueuedConnection);
        }
        return true;
    }
    void close() {
        const std::lock_guard lock(mutex_);
        closed_ = true;
        latest_.reset();
    }
    std::uint64_t posted() const { const std::lock_guard lock(mutex_); return posted_; }
private:
    std::function<void(std::uint64_t)> apply_;
    mutable std::mutex mutex_;
    std::optional<std::uint64_t> latest_;
    std::uint64_t posted_ = 0;
    bool pending_ = false;
    bool closed_ = false;
};
}
