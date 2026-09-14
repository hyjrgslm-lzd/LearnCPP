#pragma once
#include <QObject>
#include <QString>
#include <QThread>
#include <vector>

namespace c16_l03 {
struct FrameRecord { QString media_id; int frame = 0; Qt::HANDLE receiver_thread = nullptr; };
class MediaSource : public QObject {
    Q_OBJECT
public:
    void publish(QString id, int frame) { emit frameReady(id, frame); }
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
inline QMetaObject::Connection connectFrames(MediaSource* source, FrameSink* sink)
{
    return QObject::connect(source, &MediaSource::frameReady, sink, [sink](QString id, int frame) {
        sink->frames.push_back({id, frame, QThread::currentThreadId()});
        emit sink->received();
    }, Qt::DirectConnection);
}
}
