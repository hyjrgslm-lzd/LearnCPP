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
    std::atomic_bool cancel{false};

    void allow(int count = 1) { resume.release(count); }
};

struct AnalysisResult {
    int request_id = 0;
    QString media_id;
    int frames = 0;
    bool cancelled = false;
    Qt::HANDLE worker_thread = nullptr;
};

class Worker final : public QObject {
public:
    AnalysisResult run(int id, const QString& media_id, int frames, const std::shared_ptr<FrameGate>& gate)
    {
        AnalysisResult result{id, media_id, 0, false, QThread::currentThreadId()};
        for (int i = 0; i < frames; ++i) {
            gate->entered.release();
            gate->resume.acquire();
            if (gate->cancel.load(std::memory_order_acquire)
                || QThread::currentThread()->isInterruptionRequested()) {
                result.cancelled = true;
                break;
            }
            ++result.frames;
            gate->processed.fetch_add(1, std::memory_order_release);
        }
        return result;
    }
};

class MediaAnalysisSession final : public QObject {
public:
    MediaAnalysisSession()
    {
        worker_ = new Worker;
        worker_->moveToThread(&thread_);
        connect(&thread_, &QThread::finished, worker_, &QObject::deleteLater);
        thread_.start();
    }

    ~MediaAnalysisSession() override { close(); }

    template<class Callback>
    int analyze(QString media_id, int frames, std::shared_ptr<FrameGate> gate, QObject* receiver,
        Callback&& callback)
    {
        if (!receiver || receiver->thread() != thread() || frames <= 0 || !gate) {
            return -1;
        }
        cancel();
        current_gate_ = gate;
        const int id = ++generation_;
        auto callback_box =
            std::make_shared<std::function<void(AnalysisResult)>>(std::forward<Callback>(callback));
        QPointer<Worker> worker(worker_);
        QPointer<QObject> guarded_receiver(receiver);

        QMetaObject::invokeMethod(worker_, [worker, guarded_receiver, callback_box, media_id, frames, gate, id] {
            if (!worker) {
                return;
            }
            auto result = worker->run(id, media_id, frames, gate);
            QMetaObject::invokeMethod(guarded_receiver, [guarded_receiver, callback_box, result] {
                if (!guarded_receiver) {
                    return;
                }
                (*callback_box)(result);
            }, Qt::QueuedConnection);
        }, Qt::QueuedConnection);
        return id;
    }

    void cancel() noexcept
    {
        if (current_gate_) {
            current_gate_->cancel.store(true, std::memory_order_release);
            current_gate_->resume.release(1024);
        }
    }

    void close() noexcept
    {
        if (closed_.exchange(true, std::memory_order_acq_rel)) {
            return;
        }
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
    Worker* worker_ = nullptr;
    std::shared_ptr<FrameGate> current_gate_;
    std::atomic<int> generation_{0};
    std::atomic_bool closed_{false};
};

} // namespace c16_l05
