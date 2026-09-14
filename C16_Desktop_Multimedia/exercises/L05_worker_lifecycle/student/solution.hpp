#pragma once

#include <QSemaphore>
#include <QString>
#include <QObject>
#include <QThread>

#include <atomic>
#include <memory>

namespace c16_l05 {

struct FrameGate {
    QSemaphore entered;
    QSemaphore resume;
    std::atomic<int> processed{0};

    void allow(int count = 1) { resume.release(count); }
};

struct AnalysisResult {
    int request_id = 0;
    QString media_id;
    int frames = 0;
    bool cancelled = false;
    Qt::HANDLE worker_thread = nullptr;
};

class MediaAnalysisSession {
public:
    int analyze(QString, int, std::shared_ptr<FrameGate>, QObject*, auto) { return -1; }
    void cancel() noexcept {}
    void close() noexcept {}
    bool isRunning() const noexcept { return false; }
};

} // namespace c16_l05
