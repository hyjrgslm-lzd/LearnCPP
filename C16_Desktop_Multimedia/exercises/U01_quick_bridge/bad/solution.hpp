#pragma once

#include <QString>
#include <QVector>

namespace u01 {

class BridgeState {
public:
    void add(const QString& name) { rows_.push_back(name); }
    void setFilter(const QString&) {}
    int rowCount() const { return rows_.size(); }
    bool select(int) { selected_ = "constant"; return true; }
    QString selectedName() const { return selected_; }

private:
    QVector<QString> rows_;
    QString selected_;
};

} // namespace u01
