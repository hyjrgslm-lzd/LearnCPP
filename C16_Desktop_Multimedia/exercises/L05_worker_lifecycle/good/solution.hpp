#pragma once

#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QSemaphore>
#include <QString>
#include <QThread>

#include <atomic>
#include <functional>
#include <memory>

namespace c16_l05 {

struct FrameGate {
    QSemaphore entered;
    QSemaphore resume;
    std::atomic<int> processed{0};
    std::atomic_bool stop{false};

    void allow(int count = 1) { resume.release(count); }
};

struct AnalysisResult {
    int request_id = 0;
    QString media_id;
    int frames = 0;
    bool cancelled = false;
    Qt::HANDLE worker_thread = nullptr;
};

class ScanWorker final : public QObject {
public:
    AnalysisResult scan(int request_id, const QString& media_id, int frames, const std::shared_ptr<FrameGate>& gate)
    {
        AnalysisResult out{request_id, media_id, 0, false, QThread::currentThreadId()};
        for (int n = 0; n < frames; ++n) {
            gate->entered.release();
            gate->resume.acquire();
            if (gate->stop.load(std::memory_order_acquire) || QThread::currentThread()->isInterruptionRequested()) {
                out.cancelled = true;
                return out;
            }
            ++out.frames;
            gate->processed.fetch_add(1, std::memory_order_release);
        }
        return out;
    }
};

class MediaAnalysisSession final : public QObject {
public:
    MediaAnalysisSession()
    {
        worker_ = new ScanWorker;
        worker_->moveToThread(&thread_);
        connect(&thread_, &QThread::finished, worker_, &QObject::deleteLater);
        thread_.start();
    }

    ~MediaAnalysisSession() override { close(); }

    template<class F>
    int analyze(QString media_id, int frames, std::shared_ptr<FrameGate> gate, QObject* receiver, F&& f)
    {
        if (!receiver || receiver->thread() != thread() || frames < 1 || !gate
            || closed_.load(std::memory_order_acquire)) {
            return -1;
        }

        cancel();
        active_gate_ = gate;
        alive_.store(true, std::memory_order_release);
        const int serial = serial_.fetch_add(1, std::memory_order_acq_rel) + 1;
        auto sink = std::make_shared<std::function<void(AnalysisResult)>>(std::forward<F>(f));
        QPointer<MediaAnalysisSession> owner(this);
        QPointer<QObject> destination(receiver);
        QPointer<ScanWorker> worker(worker_);

        QMetaObject::invokeMethod(worker_, [owner, destination, worker, sink, media_id, frames, gate, serial] {
            if (!worker) {
                return;
            }
            auto result = worker->scan(serial, media_id, frames, gate);
            if (!owner) {
                return;
            }
            QMetaObject::invokeMethod(owner, [owner, destination, sink, result] {
                if (!owner || !destination || !owner->alive_.load(std::memory_order_acquire)) {
                    return;
                }
                if (owner->serial_.load(std::memory_order_acquire) != result.request_id) {
                    return;
                }
                (*sink)(result);
            }, Qt::QueuedConnection);
        }, Qt::QueuedConnection);

        return serial;
    }

    void cancel() noexcept
    {
        if (active_gate_) {
            active_gate_->stop.store(true, std::memory_order_release);
            active_gate_->resume.release(1024);
        }
    }

    void close() noexcept
    {
        if (closed_.exchange(true, std::memory_order_acq_rel)) {
            return;
        }
        alive_.store(false, std::memory_order_release);
        cancel();
        if (thread_.isRunning()) {
            thread_.requestInterruption();
            thread_.quit();
            thread_.wait();
        }
        worker_ = nullptr;
    }

    bool isRunning() const noexcept { return thread_.isRunning(); }

private:
    QThread thread_;
    ScanWorker* worker_ = nullptr;
    std::shared_ptr<FrameGate> active_gate_;
    std::atomic<int> serial_{0};
    std::atomic_bool alive_{false};
    std::atomic_bool closed_{false};
};

} // namespace c16_l05
