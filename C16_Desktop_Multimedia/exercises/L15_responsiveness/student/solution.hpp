#pragma once
#include <QObject>
#include <cstdint>
#include <functional>
namespace c16_l15 {
class PreviewMailbox final : public QObject {
public:
    explicit PreviewMailbox(std::function<void(std::uint64_t)>) {}
    // Implement a latest-value slot and at most one queued Qt notification.
    // Keep client callbacks outside locks; join producers before QObject destruction.
    bool submit(std::uint64_t) { return false; }
    void close() {}
    std::uint64_t posted() const { return 0; }
};
}
