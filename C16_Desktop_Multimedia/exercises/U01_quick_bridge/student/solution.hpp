#pragma once

#include <QString>

namespace u01 {

class BridgeState {
public:
    void add(const QString&) {}
    void setFilter(const QString&) {}
    int rowCount() const { return 0; }
    bool select(int) { return false; }
    QString selectedName() const { return {}; }
};

} // namespace u01
