#include <solution.hpp>
#include <c16/check.hpp>

#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

namespace {
QTemporaryDir make_temp_dir(const char* scenario)
{
    QTemporaryDir dir;
    c16::require(dir.isValid(),
        QString("temporary directory for %1 is invalid: %2").arg(scenario, dir.errorString()).toStdString());
    return dir;
}

c16_l07::SessionState sample()
{
    c16_l07::SessionState state;
    state.version = 1;
    state.recent_media = "clip-a";
    state.window_geometry = QByteArray("geometry");
    state.device_pixel_ratio = 1.5;
    state.locale = "zh_CN";
    return state;
}

std::string save_error(const c16_l07::SessionStore& store)
{
    return store.lastError().isEmpty() ? std::string("<empty>") : store.lastError().toStdString();
}

void check_round_trip()
{
    QTemporaryDir dir = make_temp_dir("round trip");
    const QString path = dir.filePath("session.json");
    c16_l07::SessionStore store;
    const bool saved = store.save(path, sample());
    c16::require(saved, "session saves atomically: " + save_error(store));
    auto loaded = store.load(path);
    c16::require(loaded.version == 1, "version persists");
    c16::require(loaded.recent_media == "clip-a", "recent media persists");
    c16::require(loaded.window_geometry == QByteArray("geometry"), "geometry bytes persist");
    c16::require(loaded.device_pixel_ratio == 1.5, "dpi scale persists");
    c16::require(loaded.locale == "zh_CN", "locale persists");
}

void check_failed_save_preserves_old_file()
{
    QTemporaryDir dir = make_temp_dir("failed save preservation");
    const QString path = dir.filePath("session.json");
    c16_l07::SessionStore store;
    auto original = sample();
    const bool saved = store.save(path, original);
    c16::require(saved, "initial save succeeds: " + save_error(store));
    QFile before(path);
    c16::require(before.open(QIODevice::ReadOnly), "old file readable");
    const QByteArray old_bytes = before.readAll();
    before.close();

    auto next = original;
    next.recent_media = "new";
    c16::require(!store.save(path, next, c16_l07::SaveMode::SimulateFailureBeforeCommit),
        "simulated failure is reported");
    c16::require(!store.lastError().isEmpty(), "failed save retains an actionable diagnostic");
    QFile after(path);
    c16::require(after.open(QIODevice::ReadOnly), "file remains readable after failed save");
    c16::require(after.readAll() == old_bytes, "failed QSaveFile commit preserves old file");
}

void check_bad_json_falls_back()
{
    QTemporaryDir dir = make_temp_dir("bad json fallback");
    const QString path = dir.filePath("session.json");
    QFile file(path);
    c16::require(file.open(QIODevice::WriteOnly), "can write bad json");
    file.write("{bad");
    file.close();
    c16_l07::SessionStore store;
    auto loaded = store.load(path);
    c16::require(loaded.version == 1 && loaded.recent_media.isEmpty(), "invalid session falls back to defaults");
}

void check_unknown_version_does_not_partially_apply()
{
    QTemporaryDir dir = make_temp_dir("unknown version fallback");
    const QString path = dir.filePath("session.json");
    QJsonObject root;
    root["version"] = 99;
    root["recentMedia"] = "future";
    root["geometryBase64"] = QString::fromLatin1(QByteArray("future-geometry").toBase64());
    root["devicePixelRatio"] = 3.0;
    root["locale"] = "xx_YY";
    QFile file(path);
    c16::require(file.open(QIODevice::WriteOnly), "can write future version");
    file.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    file.close();

    c16_l07::SessionStore store;
    auto loaded = store.load(path);
    c16::require(loaded.version == 1, "unknown version falls back to current default version");
    c16::require(loaded.recent_media.isEmpty(), "unknown version does not partially apply recent media");
    c16::require(loaded.window_geometry.isEmpty(), "unknown version does not partially apply geometry");
    c16::require(loaded.device_pixel_ratio == 1.0, "unknown version does not partially apply dpi");
    c16::require(loaded.locale.isEmpty(), "unknown version does not partially apply locale");
}
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    return c16::run([] {
        check_round_trip();
        check_failed_save_preserves_old_file();
        check_bad_json_falls_back();
        check_unknown_version_does_not_partially_apply();
    });
}
