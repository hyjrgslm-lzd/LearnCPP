#pragma once
#include <QByteArray>
#include <QString>

namespace c16_l07 {
enum class SaveMode { Normal, SimulateFailureBeforeCommit };
struct SessionState { int version = 1; QString recent_media; QByteArray window_geometry; double device_pixel_ratio = 1.0; QString locale; };
class SessionStore {
public:
    bool save(const QString&, const SessionState&, SaveMode = SaveMode::Normal) { return false; }
    SessionState load(const QString&) const { return {}; }
    QString lastError() const { return "unfinished"; }
};
}
