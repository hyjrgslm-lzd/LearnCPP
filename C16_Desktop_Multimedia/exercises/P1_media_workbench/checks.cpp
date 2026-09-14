#include <solution.hpp>

#include <c16/check.hpp>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace {

void writeJson(const QString& path, const QJsonObject& object)
{
    QFile file(path);
    c16::require(file.open(QIODevice::WriteOnly | QIODevice::Truncate), "json fixture opens for write");
    const QByteArray payload = QJsonDocument(object).toJson();
    c16::require(file.write(payload) == payload.size(), "json fixture writes completely");
}

QJsonObject savedSession(const QString& path)
{
    QFile file(path);
    c16::require(file.open(QIODevice::ReadOnly), "saved session opens for mutation");
    const auto doc = QJsonDocument::fromJson(file.readAll());
    c16::require(doc.isObject(), "saved session is json object");
    return doc.object();
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    return c16::run([] {
        QDir temp(QCoreApplication::applicationDirPath());
        c16::require(temp.mkpath("p1_state_checks"), "build-local session directory exists");
        c16::require(temp.cd("p1_state_checks"), "build-local session directory opens");
        const QString a = temp.filePath("a.wav");
        const QString b = temp.filePath("b.avi");
        QFile fileA(a);
        QFile fileB(b);
        c16::require(fileA.open(QIODevice::WriteOnly), "fixture a can be created");
        c16::require(fileB.open(QIODevice::WriteOnly), "fixture b can be created");
        fileA.close();
        fileB.close();

        p1::ProjectState empty;
        const QString emptySession = temp.filePath("empty.json");
        c16::require(empty.save(emptySession), "empty session saves");
        p1::ProjectState emptyRestored;
        c16::require(emptyRestored.restore(emptySession), "empty session restores");
        c16::require(emptyRestored.count() == 0 && emptyRestored.markerCount() == 0
                && emptyRestored.selectedId().isEmpty(),
            "empty session round-trips as empty state");

        p1::ProjectState state;
        const QString idA = state.addMedia(a);
        const QString idB = state.addMedia(b);
        c16::require(!idA.isEmpty() && idA == state.addMedia(a), "media id is stable for the same path");
        c16::require(idA != idB, "different paths get different stable ids");
        c16::require(state.count() == 2, "duplicate add does not duplicate rows");

        state.setFilter("a.");
        c16::require(state.visibleCount() == 1, "filter applies to display name or path");
        c16::require(state.selectVisibleRow(0), "visible row can be selected");
        c16::require(state.selectedId() == idA, "selection follows filtered row mapping");
        c16::require(state.addMarker(125, "first"), "marker requires current selection");
        c16::require(!state.addMarker(-1, "bad"), "marker rejects negative timestamp");

        const QString session = temp.filePath("session.json");
        c16::require(state.save(session), "session saves through implementation");
        p1::ProjectState restored;
        c16::require(restored.restore(session), "session restores through implementation");
        c16::require(restored.count() == 2, "restore keeps media rows");
        c16::require(restored.selectedId() == idA, "restore keeps selected stable id");
        c16::require(restored.markerCount() == 1, "restore keeps timestamp markers");

        const int stableCount = restored.count();
        const QString stableSelected = restored.selectedId();
        const int stableMarkers = restored.markerCount();
        auto root = savedSession(session);

        const QString missingPath = temp.filePath("missing.json");
        auto missing = root;
        auto missingItems = missing.value("items").toArray();
        auto first = missingItems.first().toObject();
        first["path"] = temp.filePath("does-not-exist.wav");
        missingItems[0] = first;
        missing["items"] = missingItems;
        writeJson(missingPath, missing);
        c16::require(!restored.restore(missingPath), "restore rejects missing media path");
        c16::require(restored.count() == stableCount && restored.selectedId() == stableSelected
                && restored.markerCount() == stableMarkers,
            "failed missing-path restore keeps old state");

        const QString badSelectedPath = temp.filePath("bad-selected.json");
        auto badSelected = root;
        badSelected["selected"] = "missing-id";
        writeJson(badSelectedPath, badSelected);
        c16::require(!restored.restore(badSelectedPath), "restore rejects selected id outside item set");
        c16::require(restored.count() == stableCount && restored.selectedId() == stableSelected,
            "failed selected restore keeps old state");

        const QString badMarkerPath = temp.filePath("bad-marker.json");
        auto badMarker = root;
        badMarker["markers"] = QJsonArray{QJsonObject{{"id", idA}, {"ms", 1.5}, {"label", "fraction"}}};
        writeJson(badMarkerPath, badMarker);
        c16::require(!restored.restore(badMarkerPath), "restore rejects non-integer marker timestamp");
        badMarker["markers"] = QJsonArray{QJsonObject{{"id", idA}, {"ms", 9007199254740992.0}, {"label", "huge"}}};
        writeJson(badMarkerPath, badMarker);
        c16::require(!restored.restore(badMarkerPath), "restore rejects marker timestamp outside safe integer range");
        badMarker["markers"] = QJsonArray{QJsonObject{{"id", "missing-id"}, {"ms", 1}, {"label", "missing"}}};
        writeJson(badMarkerPath, badMarker);
        c16::require(!restored.restore(badMarkerPath), "restore rejects marker id outside item set");
        c16::require(restored.markerCount() == stableMarkers, "failed marker restore keeps old markers");

        const QString oversized = temp.filePath("oversized.json");
        QFile big(oversized);
        c16::require(big.open(QIODevice::WriteOnly), "oversized fixture opens");
        c16::require(big.write(QByteArray(1024 * 1024 + 1, 'x')) == 1024 * 1024 + 1, "oversized fixture writes");
        big.close();
        c16::require(!restored.restore(oversized), "restore rejects oversized session file");
        c16::require(restored.count() == stableCount && restored.markerCount() == stableMarkers,
            "failed oversized restore keeps old state");
    });
}
