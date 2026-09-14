#pragma once
#include <QAbstractListModel>
#include <QString>
#include <vector>

namespace c16_l06 {
struct MediaClip { QString id; QString title; int duration_seconds = 0; };
class MediaListModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles { IdRole = Qt::UserRole + 1, TitleRole, DurationRole };
    int rowCount(const QModelIndex& = {}) const override { return int(items_.size()); }
    QVariant data(const QModelIndex& index, int role) const override
    {
        if (!index.isValid() || index.row() < 0 || index.row() >= int(items_.size())) return {};
        const auto& item = items_[std::size_t(index.row())];
        if (role == TitleRole || role == Qt::DisplayRole) return item.title;
        if (role == DurationRole) return item.duration_seconds;
        if (role == IdRole) return QString::number(index.row());
        return {};
    }
    QHash<int, QByteArray> roleNames() const override { return {{TitleRole, "title"}}; }
    void addClip(MediaClip clip) { items_.push_back(std::move(clip)); }
    bool insertClip(int row, MediaClip clip)
    {
        if (row < 0 || row > int(items_.size())) return false;
        items_.insert(items_.begin() + row, std::move(clip));
        return true;
    }
    bool renameClip(const QString& id, const QString& title)
    {
        for (auto& item : items_) if (item.id == id) { item.title = title; return true; }
        return false;
    }
    bool removeClip(const QString& id)
    {
        for (auto it = items_.begin(); it != items_.end(); ++it) if (it->id == id) { items_.erase(it); return true; }
        return false;
    }
    bool moveClip(const QString& id, int destination)
    {
        for (auto it = items_.begin(); it != items_.end(); ++it) {
            if (it->id == id) {
                auto item = std::move(*it);
                items_.erase(it);
                items_.insert(items_.begin() + destination, std::move(item));
                return true;
            }
        }
        return false;
    }
private:
    std::vector<MediaClip> items_;
};
}
