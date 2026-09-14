#pragma once
#include <QObject>
#include <memory>

namespace c16_l02 {
inline std::unique_ptr<QObject> makeLibraryTree(bool*)
{
    auto root = std::make_unique<QObject>();
    root->setObjectName("library");
    auto* clip = new QObject;
    clip->setObjectName("clip");
    return root;
}

inline bool adoptExternalClip(QObject* parent, QObject* clip)
{
    if (!parent || !clip) return false;
    clip->setParent(parent);
    return true;
}

inline void deleteWhenIdle(QObject* object)
{
    delete object;
}
}
