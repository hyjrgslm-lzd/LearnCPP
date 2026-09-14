#pragma once

#include <QString>

namespace p1 {

class ProjectState {
public:
    QString addMedia(const QString&) { return {}; }
    int count() const { return 0; }
    void setFilter(const QString&) {}
    int visibleCount() const { return 0; }
    bool selectVisibleRow(int) { return false; }
    QString selectedId() const { return {}; }
    bool addMarker(qint64, const QString&) { return false; }
    int markerCount() const { return 0; }
    bool save(const QString&) const { return false; }
    bool restore(const QString&) { return false; }
};

} // namespace p1
