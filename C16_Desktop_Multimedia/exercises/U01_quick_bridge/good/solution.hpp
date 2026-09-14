#pragma once

#include <QFileInfo>
#include <QString>
#include <QVector>

namespace u01 {

class BridgeState {
public:
    void add(const QString& path) { names_.append(QFileInfo(path).fileName()); refresh(); }
    void setFilter(const QString& value) { filter_ = value; refresh(); }
    int rowCount() const { return map_.size(); }
    bool select(int row)
    {
        if (row < 0 || row >= map_.size()) return false;
        selected_ = names_[map_[row]];
        return true;
    }
    QString selectedName() const { return selected_; }

private:
    void refresh()
    {
        map_.clear();
        for (int i = 0; i < names_.size(); ++i) {
            if (filter_.isEmpty() || names_[i].contains(filter_, Qt::CaseInsensitive)) map_.append(i);
        }
    }
    QVector<QString> names_;
    QVector<int> map_;
    QString filter_;
    QString selected_;
};

} // namespace u01
