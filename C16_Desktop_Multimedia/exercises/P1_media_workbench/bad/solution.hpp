#pragma once

#include <QFileInfo>
#include <QString>
#include <QVector>

namespace p1 {

class ProjectState {
public:
    QString addMedia(const QString& path)
    {
        const QString id = QFileInfo(path).fileName(); // bad: not stable across equal basenames in different dirs.
        rows_.push_back({id, path}); // bad: duplicates are accepted.
        return id;
    }
    int count() const { return rows_.size(); }
    void setFilter(const QString&) {}
    int visibleCount() const { return rows_.size(); }
    bool selectVisibleRow(int row) { selected_ = rows_.value(row).id; return row >= 0 && row < rows_.size(); }
    QString selectedId() const { return selected_; }
    bool addMarker(qint64, const QString&) { return !selected_.isEmpty(); }
    int markerCount() const { return 0; }
    bool save(const QString&) const { return true; }
    bool restore(const QString&) { return true; }

private:
    struct Row { QString id; QString path; };
    QVector<Row> rows_;
    QString selected_;
};

} // namespace p1
