#pragma once
#include <QAbstractListModel>
#include <QString>
#include <vector>

namespace c16_l06 {
struct MediaClip {
    QString id;
    QString title;
    int duration_seconds = 0;
};

class MediaListModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles { IdRole = Qt::UserRole + 1, TitleRole, DurationRole };
    int rowCount(const QModelIndex& parent = {}) const override
    {
        return parent.isValid() ? 0 : static_cast<int>(clips_.size());
    }
    QVariant data(const QModelIndex& index, int role) const override
    {
        if (!index.isValid() || index.row() < 0 || index.row() >= rowCount({})) return {};
        const auto& clip = clips_[static_cast<std::size_t>(index.row())];
        if (role == IdRole) return clip.id;
        if (role == TitleRole || role == Qt::DisplayRole) return clip.title;
        if (role == DurationRole) return clip.duration_seconds;
        return {};
    }
    QHash<int, QByteArray> roleNames() const override
    {
        return {{IdRole, "id"}, {TitleRole, "title"}, {DurationRole, "duration"}};
    }
    void addClip(MediaClip clip)
    {
        insertClip(rowCount({}), std::move(clip));
    }
    bool insertClip(int row, MediaClip clip)
    {
        if (row < 0 || row > rowCount({})) return false;
        beginInsertRows({}, row, row);
        clips_.insert(clips_.begin() + row, std::move(clip));
        endInsertRows();
        return true;
    }
    bool renameClip(const QString& id, const QString& title)
    {
        for (int row = 0; row < rowCount({}); ++row) {
            if (clips_[static_cast<std::size_t>(row)].id == id) {
                clips_[static_cast<std::size_t>(row)].title = title;
                const auto changed = index(row, 0);
                emit dataChanged(changed, changed, {TitleRole, Qt::DisplayRole});
                return true;
            }
        }
        return false;
    }
    bool removeClip(const QString& id)
    {
        const int row = findRow(id);
        if (row < 0) return false;
        beginRemoveRows({}, row, row);
        clips_.erase(clips_.begin() + row);
        endRemoveRows();
        return true;
    }
    bool moveClip(const QString& id, int destination_row)
    {
        const int source_row = findRow(id);
        if (source_row < 0 || destination_row < 0 || destination_row >= rowCount({}) || source_row == destination_row) {
            return false;
        }
        const int begin_destination = destination_row > source_row ? destination_row + 1 : destination_row;
        beginMoveRows({}, source_row, source_row, {}, begin_destination);
        auto clip = std::move(clips_[static_cast<std::size_t>(source_row)]);
        clips_.erase(clips_.begin() + source_row);
        clips_.insert(clips_.begin() + destination_row, std::move(clip));
        endMoveRows();
        return true;
    }
private:
    int findRow(const QString& id) const
    {
        for (int row = 0; row < rowCount({}); ++row) {
            if (clips_[static_cast<std::size_t>(row)].id == id) return row;
        }
        return -1;
    }
    std::vector<MediaClip> clips_;
};
}
