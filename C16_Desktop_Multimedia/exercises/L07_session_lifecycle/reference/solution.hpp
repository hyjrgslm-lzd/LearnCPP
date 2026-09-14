#pragma once
#include <QByteArray>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QString>

namespace c16_l07 {
enum class SaveMode { Normal, SimulateFailureBeforeCommit };
struct SessionState {
    int version = 1;
    QString recent_media;
    QByteArray window_geometry;
    double device_pixel_ratio = 1.0;
    QString locale;
};

class SessionStore {
public:
    bool save(const QString& path, const SessionState& state, SaveMode mode = SaveMode::Normal)
    {
        last_error_.clear();
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
            last_error_ = "open " + path + ": " + file.errorString();
            return false;
        }
        QJsonObject root;
        root["version"] = state.version;
        root["recentMedia"] = state.recent_media;
        root["geometryBase64"] = QString::fromLatin1(state.window_geometry.toBase64());
        root["devicePixelRatio"] = state.device_pixel_ratio;
        root["locale"] = state.locale;
        const QByteArray bytes = QJsonDocument(root).toJson(QJsonDocument::Compact);
        if (file.write(bytes) != bytes.size()) {
            last_error_ = "write " + path + ": " + file.errorString();
            file.cancelWriting();
            return false;
        }
        if (mode == SaveMode::SimulateFailureBeforeCommit) {
            last_error_ = "simulated failure before commit for " + path;
            file.cancelWriting();
            return false;
        }
        if (!file.commit()) {
            last_error_ = "commit " + path + ": " + file.errorString();
            return false;
        }
        return true;
    }

    SessionState load(const QString& path) const
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) return {};
        const auto doc = QJsonDocument::fromJson(file.readAll());
        if (!doc.isObject()) return {};
        const auto root = doc.object();
        if (root.value("version").toInt(1) != 1) return {};
        SessionState state;
        state.version = 1;
        state.recent_media = root.value("recentMedia").toString();
        state.window_geometry = QByteArray::fromBase64(root.value("geometryBase64").toString().toLatin1());
        state.device_pixel_ratio = root.value("devicePixelRatio").toDouble(1.0);
        state.locale = root.value("locale").toString();
        return state;
    }
    QString lastError() const { return last_error_; }
private:
    QString last_error_;
};
}
