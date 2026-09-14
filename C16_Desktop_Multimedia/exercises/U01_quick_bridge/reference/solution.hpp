#pragma once

#include <QFileInfo>
#include <QString>
#include <QVector>

namespace u01 {

class BridgeState {
public:
    void add(const QString& name)
    {
        rows_.push_back(QFileInfo(name).fileName());
        rebuild();
    }
    void setFilter(const QString& text)
    {
        filter_ = text;
        rebuild();
    }
    int rowCount() const { return visible_.size(); }
    bool select(int row)
    {
        if (row < 0 || row >= visible_.size()) return false;
        selected_ = rows_[visible_[row]];
        return true;
    }
    QString selectedName() const { return selected_; }

private:
    void rebuild()
    {
        visible_.clear();
        for (int i = 0; i < rows_.size(); ++i) {
            if (filter_.isEmpty() || rows_[i].contains(filter_, Qt::CaseInsensitive)) visible_.push_back(i);
        }
    }
    QVector<QString> rows_;
    QVector<int> visible_;
    QString filter_;
    QString selected_;
};

} // namespace u01
