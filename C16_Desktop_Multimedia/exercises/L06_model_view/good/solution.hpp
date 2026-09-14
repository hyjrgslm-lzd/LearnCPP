#pragma once
#include <QAbstractListModel>
#include <QString>
#include <algorithm>
#include <vector>

namespace c16_l06 {
struct MediaClip { QString id; QString title; int duration_seconds = 0; };
class MediaListModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles { IdRole = Qt::UserRole + 1, TitleRole, DurationRole };
    int rowCount(const QModelIndex& parent = {}) const override { return parent.isValid() ? 0 : int(items_.size()); }
    QVariant data(const QModelIndex& idx, int role) const override
    {
        if (!idx.isValid() || idx.row() < 0 || idx.row() >= int(items_.size())) return {};
        const auto& item = items_[std::size_t(idx.row())];
        switch (role) {
        case IdRole: return item.id;
        case TitleRole:
        case Qt::DisplayRole: return item.title;
        case DurationRole: return item.duration_seconds;
        default: return {};
        }
    }
    QHash<int, QByteArray> roleNames() const override
    {
        auto roles = QAbstractListModel::roleNames();
        roles[IdRole] = "id";
        roles[TitleRole] = "title";
        roles[DurationRole] = "duration";
        return roles;
    }
    void addClip(MediaClip clip) { insertClip(int(items_.size()), std::move(clip)); }
    bool insertClip(int row, MediaClip clip)
    {
        if (row < 0 || row > int(items_.size())) return false;
        beginInsertRows({}, row, row);
        items_.push_back(std::move(clip));
        if (row != int(items_.size()) - 1) {
            std::rotate(items_.begin() + row, items_.end() - 1, items_.end());
        }
        endInsertRows();
        return true;
    }
    bool renameClip(const QString& id, const QString& title)
    {
        for (int row = 0; row < int(items_.size()); ++row) {
            if (items_[std::size_t(row)].id == id) {
                items_[std::size_t(row)].title = title;
                auto idx = index(row, 0);
                emit dataChanged(idx, idx, {TitleRole, Qt::DisplayRole});
                return true;
            }
        }
        return false;
    }
    bool removeClip(const QString& id)
    {
        int row = find(id);
        if (row < 0) return false;
        beginRemoveRows({}, row, row);
        items_.erase(items_.begin() + row);
        endRemoveRows();
        return true;
    }
    bool moveClip(const QString& id, int destination)
    {
        int source = find(id);
        if (source < 0 || destination < 0 || destination >= int(items_.size()) || source == destination) return false;
        beginMoveRows({}, source, source, {}, destination > source ? destination + 1 : destination);
        auto item = std::move(items_[std::size_t(source)]);
        items_.erase(items_.begin() + source);
        items_.insert(items_.begin() + destination, std::move(item));
        endMoveRows();
        return true;
    }
private:
    int find(const QString& id) const
    {
        for (int row = 0; row < int(items_.size()); ++row) if (items_[std::size_t(row)].id == id) return row;
        return -1;
    }
    std::vector<MediaClip> items_;
};
}
