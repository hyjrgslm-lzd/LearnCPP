#pragma once
#include <QObject>
#include <memory>

namespace c16_l02 {
std::unique_ptr<QObject> makeLibraryTree(bool*) { return {}; }
bool adoptExternalClip(QObject*, QObject*) { return false; }
void deleteWhenIdle(QObject*) {}
}
