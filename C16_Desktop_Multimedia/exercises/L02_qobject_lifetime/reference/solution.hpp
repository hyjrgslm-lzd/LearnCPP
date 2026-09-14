#pragma once
#include <QObject>
#include <memory>

namespace c16_l02 {
class ClipObject final : public QObject {
public:
    explicit ClipObject(bool* destroyed, QObject* parent = nullptr) : QObject(parent), destroyed_(destroyed) {}
    ~ClipObject() override
    {
        if (destroyed_) *destroyed_ = true;
    }
private:
    bool* destroyed_ = nullptr;
};

inline std::unique_ptr<QObject> makeLibraryTree(bool* child_destroyed)
{
    auto root = std::make_unique<QObject>();
    root->setObjectName("library");
    auto* clip = new ClipObject(child_destroyed, root.get());
    clip->setObjectName("clip");
    return root;
}

inline bool adoptExternalClip(QObject* parent, QObject* clip)
{
    if (!parent || !clip || clip->parent() != parent) {
        return false;
    }
    return true;
}

inline void deleteWhenIdle(QObject* object)
{
    if (object) object->deleteLater();
}
}
