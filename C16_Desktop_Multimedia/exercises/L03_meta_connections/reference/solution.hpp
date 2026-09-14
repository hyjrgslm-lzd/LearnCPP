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
    void publish(QString media_id, int frame) { emit frameReady(std::move(media_id), frame); }
signals:
    void frameReady(QString mediaId, int frame);
};

class FrameSink : public QObject {
    Q_OBJECT
public:
    std::vector<FrameRecord> frames;
    void accept(QString media_id, int frame)
    {
        frames.push_back({std::move(media_id), frame, QThread::currentThreadId()});
        emit received();
    }
signals:
    void received();
};

inline QMetaObject::Connection connectFrames(MediaSource* source, FrameSink* sink)
{
    return QObject::connect(source, &MediaSource::frameReady, sink,
        [sink](const QString& media_id, int frame) { sink->accept(media_id, frame); }, Qt::AutoConnection);
}
}
