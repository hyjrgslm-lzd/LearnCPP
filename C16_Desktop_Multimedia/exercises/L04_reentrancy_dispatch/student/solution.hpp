#pragma once
#include <QString>
#include <functional>

namespace c16_l04 {
class DispatchQueue {
public:
    void post(QString, std::function<void()>) {}
    void drain() {}
    void clear() {}
};
}
