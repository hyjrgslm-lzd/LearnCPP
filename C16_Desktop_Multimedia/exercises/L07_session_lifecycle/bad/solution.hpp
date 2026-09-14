#pragma once
#include <QByteArray>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

namespace c16_l07 {
enum class SaveMode { Normal, SimulateFailureBeforeCommit };
struct SessionState { int version = 1; QString recent_media; QByteArray window_geometry; double device_pixel_ratio = 1.0; QString locale; };
class SessionStore {
public:
    bool save(const QString& path, const SessionState& state, SaveMode mode = SaveMode::Normal)
    {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
        if (mode == SaveMode::SimulateFailureBeforeCommit) return false;
        QJsonObject root{{"version", state.version}, {"recentMedia", state.recent_media}};
        file.write(QJsonDocument(root).toJson());
        return true;
    }
    SessionState load(const QString& path) const
    {
        QFile file(path);
        file.open(QIODevice::ReadOnly);
        auto root = QJsonDocument::fromJson(file.readAll()).object();
        SessionState state;
        state.version = root.value("version").toInt();
        state.recent_media = root.value("recentMedia").toString();
        return state;
    }
    QString lastError() const { return {}; }
};
}
