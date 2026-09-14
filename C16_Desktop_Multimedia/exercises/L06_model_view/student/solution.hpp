#pragma once
#include <QAbstractListModel>
#include <QString>

namespace c16_l06 {
struct MediaClip { QString id; QString title; int duration_seconds = 0; };
class MediaListModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles { IdRole = Qt::UserRole + 1, TitleRole, DurationRole };
    int rowCount(const QModelIndex& = {}) const override { return 0; }
    QVariant data(const QModelIndex&, int) const override { return {}; }
    QHash<int, QByteArray> roleNames() const override { return {}; }
    void addClip(MediaClip) {}
    bool insertClip(int, MediaClip) { return false; }
    bool renameClip(const QString&, const QString&) { return false; }
    bool removeClip(const QString&) { return false; }
    bool moveClip(const QString&, int) { return false; }
};
}
