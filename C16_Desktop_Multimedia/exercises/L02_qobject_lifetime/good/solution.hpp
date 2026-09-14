#pragma once
#include <QObject>
#include <memory>

namespace c16_l02 {
class TrackedObject final : public QObject {
public:
    TrackedObject(bool* flag, QObject* owner) : QObject(owner), flag_(flag) {}
    ~TrackedObject() override
    {
        if (flag_) *flag_ = true;
    }
private:
    bool* flag_;
};

inline std::unique_ptr<QObject> makeLibraryTree(bool* child_destroyed)
{
    auto owner = std::make_unique<QObject>();
    owner->setObjectName("library");
    auto* child = new TrackedObject(child_destroyed, owner.get());
    child->setObjectName("clip");
    return owner;
}

inline bool adoptExternalClip(QObject* parent, QObject* clip)
{
    return parent && clip && clip->parent() == parent;
}

inline void deleteWhenIdle(QObject* object)
{
    if (object != nullptr) object->deleteLater();
}
}
