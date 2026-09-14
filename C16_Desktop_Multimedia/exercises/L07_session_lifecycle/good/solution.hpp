#pragma once
#include <QByteArray>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QString>

namespace c16_l07 {
enum class SaveMode { Normal, SimulateFailureBeforeCommit };
struct SessionState { int version = 1; QString recent_media; QByteArray window_geometry; double device_pixel_ratio = 1.0; QString locale; };
class SessionStore {
public:
    bool save(const QString& path, const SessionState& s, SaveMode mode = SaveMode::Normal)
    {
        last_error_.clear();
        QSaveFile out(path);
        if (!out.open(QIODevice::WriteOnly)) {
            last_error_ = "open " + path + ": " + out.errorString();
            return false;
        }
        QJsonObject obj{{"version", 1}, {"recentMedia", s.recent_media},
            {"geometryBase64", QString::fromLatin1(s.window_geometry.toBase64())},
            {"devicePixelRatio", s.device_pixel_ratio}, {"locale", s.locale}};
        const QByteArray bytes = QJsonDocument(obj).toJson(QJsonDocument::Compact);
        if (out.write(bytes) != bytes.size()) {
            last_error_ = "write " + path + ": " + out.errorString();
            out.cancelWriting();
            return false;
        }
        if (mode == SaveMode::SimulateFailureBeforeCommit) {
            last_error_ = "simulated failure before commit for " + path;
            out.cancelWriting();
            return false;
        }
        if (!out.commit()) {
            last_error_ = "commit " + path + ": " + out.errorString();
            return false;
        }
        return true;
    }
    SessionState load(const QString& path) const
    {
        QFile in(path);
        if (!in.open(QIODevice::ReadOnly)) return {};
        auto doc = QJsonDocument::fromJson(in.readAll());
        if (!doc.isObject()) return {};
        auto obj = doc.object();
        if (obj.value("version").toInt(1) != 1) return {};
        SessionState s;
        s.recent_media = obj.value("recentMedia").toString();
        s.window_geometry = QByteArray::fromBase64(obj.value("geometryBase64").toString().toLatin1());
        s.device_pixel_ratio = obj.value("devicePixelRatio").toDouble(1.0);
        s.locale = obj.value("locale").toString();
        return s;
    }
    QString lastError() const { return last_error_; }
private:
    QString last_error_;
};
}
