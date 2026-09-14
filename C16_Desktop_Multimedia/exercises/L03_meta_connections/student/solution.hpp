#pragma once
#include <QObject>
#include <QString>
#include <QThread>
#include <vector>

namespace c16_l03 {
struct FrameRecord {
    QString media_id;
    int frame = 0;
    Qt::HANDLE receiver_thread = nullptr;
};

class MediaSource : public QObject {
    Q_OBJECT
public:
    void publish(QString, int) {}
signals:
    void frameReady(QString, int);
};

class FrameSink : public QObject {
    Q_OBJECT
public:
    std::vector<FrameRecord> frames;
signals:
    void received();
};
inline QMetaObject::Connection connectFrames(MediaSource*, FrameSink*) { return {}; }
}
