#pragma once

#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSet>
#include <QString>
#include <QVector>

#include <cmath>

namespace p1 {

class ProjectState {
public:
    QString addMedia(const QString& path)
    {
        const QFileInfo info(path);
        if (!info.exists() || !info.isFile()) {
            return {};
        }
        const QString full = info.absoluteFilePath();
        const QString id = stableId(full);
        for (const auto& row : rows_) {
            if (row.id == id) {
                rebuild();
                return id;
            }
        }
        rows_.append({id, full});
        rebuild();
        return id;
    }

    int count() const { return rows_.size(); }
    void setFilter(const QString& text) { filter_ = text; rebuild(); }
    int visibleCount() const { return view_.size(); }

    bool selectVisibleRow(int row)
    {
        if (row < 0 || row >= view_.size()) return false;
        selected_ = rows_[view_[row]].id;
        return true;
    }

    QString selectedId() const { return selected_; }

    bool addMarker(qint64 ms, const QString& label)
    {
        if (selected_.isEmpty() || ms < 0 || label.isEmpty()) return false;
        marks_.append({selected_, ms, label});
        return true;
    }

    int markerCount() const { return marks_.size(); }

    bool save(const QString& path) const
    {
        QJsonArray rows;
        for (const auto& row : rows_) rows.append(QJsonObject{{"id", row.id}, {"path", row.path}});
        QJsonArray marks;
        for (const auto& mark : marks_) marks.append(QJsonObject{{"id", mark.id}, {"ms", double(mark.ms)}, {"label", mark.label}});
        const QByteArray payload = QJsonDocument(QJsonObject{{"version", 1}, {"items", rows}, {"markers", marks}, {"selected", selected_}, {"filter", filter_}}).toJson();
        if (payload.size() > kMaxSessionBytes) return false;
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly)) return false;
        if (file.write(payload) != payload.size()) return false;
        return file.commit();
    }

    bool restore(const QString& path)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly) || file.size() > kMaxSessionBytes) return false;
        const QByteArray payload = file.read(kMaxSessionBytes + 1);
        if (payload.size() != file.size()) return false;
        QJsonParseError parseError;
        const auto doc = QJsonDocument::fromJson(payload, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) return false;
        QVector<Row> rows;
        QVector<Mark> marks;
        QString selected;
        QString filter;
        if (!parse(doc.object(), rows, marks, selected, filter)) return false;
        rows_ = std::move(rows);
        marks_ = std::move(marks);
        selected_ = std::move(selected);
        filter_ = std::move(filter);
        rebuild();
        return true;
    }

private:
    struct Row { QString id; QString path; };
    struct Mark { QString id; qint64 ms; QString label; };
    static constexpr qint64 kMaxSessionBytes = 1024 * 1024;
    static constexpr double kMaxSafeInteger = 9007199254740991.0;

    static QString stableId(const QString& path)
    {
        return QCryptographicHash::hash(QFileInfo(path).absoluteFilePath().toUtf8(), QCryptographicHash::Sha1).toHex();
    }

    static bool validMarkerMs(const QJsonValue& value, qint64& out)
    {
        if (!value.isDouble()) return false;
        const double ms = value.toDouble();
        if (!std::isfinite(ms) || ms < 0 || ms > kMaxSafeInteger || std::floor(ms) != ms) return false;
        out = qint64(ms);
        return true;
    }

    static bool parse(const QJsonObject& root, QVector<Row>& rows, QVector<Mark>& marks, QString& selected, QString& filter)
    {
        if (root.value("version").toInt(-1) != 1 || !root.value("items").isArray() || !root.value("markers").isArray()
            || !root.value("selected").isString() || !root.value("filter").isString()) {
            return false;
        }
        QSet<QString> ids;
        for (const auto value : root.value("items").toArray()) {
            if (!value.isObject()) return false;
            const auto item = value.toObject();
            if (!item.value("path").isString() || !item.value("id").isString()) return false;
            const QFileInfo info(item.value("path").toString());
            if (!info.exists() || !info.isFile()) return false;
            const QString id = stableId(info.absoluteFilePath());
            if (item.value("id").toString() != id || ids.contains(id)) return false;
            ids.insert(id);
            rows.append({id, info.absoluteFilePath()});
        }
        selected = root.value("selected").toString();
        if (!selected.isEmpty() && !ids.contains(selected)) return false;
        filter = root.value("filter").toString();
        for (const auto value : root.value("markers").toArray()) {
            if (!value.isObject()) return false;
            const auto marker = value.toObject();
            if (!marker.value("id").isString() || !marker.value("label").isString()) return false;
            const QString id = marker.value("id").toString();
            qint64 ms = 0;
            if (!ids.contains(id) || !validMarkerMs(marker.value("ms"), ms)) return false;
            marks.append({id, ms, marker.value("label").toString()});
        }
        return true;
    }

    void rebuild()
    {
        view_.clear();
        for (int i = 0; i < rows_.size(); ++i) {
            if (filter_.isEmpty() || rows_[i].path.contains(filter_, Qt::CaseInsensitive)
                || QFileInfo(rows_[i].path).fileName().contains(filter_, Qt::CaseInsensitive)) view_.append(i);
        }
    }

    QVector<Row> rows_;
    QVector<Mark> marks_;
    QVector<int> view_;
    QString selected_;
    QString filter_;
};

} // namespace p1
