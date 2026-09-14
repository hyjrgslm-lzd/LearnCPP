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
#include <mutex>

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

namespace detail {

struct Job {
    int request_id = 0;
    QString media_id;
    int frames = 0;
    std::shared_ptr<FrameGate> gate;
};

class Worker final : public QObject {
public:
    AnalysisResult run(const Job& job)
    {
        AnalysisResult result;
        result.request_id = job.request_id;
        result.media_id = job.media_id;
        result.worker_thread = QThread::currentThreadId();

        for (int i = 0; i < job.frames; ++i) {
            job.gate->entered.release();
            job.gate->resume.acquire();
            if (job.gate->cancel.load(std::memory_order_acquire)
                || QThread::currentThread()->isInterruptionRequested()) {
                result.cancelled = true;
                break;
            }
            ++result.frames;
            job.gate->processed.fetch_add(1, std::memory_order_release);
        }
        return result;
    }
};

} // namespace detail

class MediaAnalysisSession final : public QObject {
public:
    MediaAnalysisSession()
    {
        worker_ = new detail::Worker;
        worker_->moveToThread(&thread_);
        QObject::connect(&thread_, &QThread::finished, worker_, &QObject::deleteLater);
        thread_.start();
    }

    ~MediaAnalysisSession() override { close(); }

    MediaAnalysisSession(const MediaAnalysisSession&) = delete;
    MediaAnalysisSession& operator=(const MediaAnalysisSession&) = delete;

    template<class Callback>
    int analyze(QString media_id, int frames, std::shared_ptr<FrameGate> gate, QObject* receiver,
        Callback&& callback)
    {
        if (!receiver || receiver->thread() != thread() || frames <= 0 || !gate
            || closed_.load(std::memory_order_acquire) || !worker_) {
            return -1;
        }

        cancel();
        const int request_id = ++generation_;
        accepting_.store(true, std::memory_order_release);
        current_gate_ = gate;

        auto callback_box =
            std::make_shared<std::function<void(AnalysisResult)>>(std::forward<Callback>(callback));
        detail::Job job{request_id, std::move(media_id), frames, std::move(gate)};
        QPointer<MediaAnalysisSession> self(this);
        QPointer<QObject> guarded_receiver(receiver);
        QPointer<detail::Worker> guarded_worker(worker_);

        QMetaObject::invokeMethod(worker_, [self, guarded_receiver, guarded_worker, callback_box, job]() {
            if (!guarded_worker) {
                return;
            }
            AnalysisResult result = guarded_worker->run(job);
            if (!self) {
                return;
            }
            QMetaObject::invokeMethod(self, [self, guarded_receiver, callback_box, result]() {
                if (!self || !guarded_receiver) {
                    return;
                }
                if (!self->accepting_.load(std::memory_order_acquire)
                    || result.request_id != self->generation_.load(std::memory_order_acquire)) {
                    return;
                }
                (*callback_box)(result);
            }, Qt::QueuedConnection);
        }, Qt::QueuedConnection);

        return request_id;
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
        accepting_.store(false, std::memory_order_release);
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
    detail::Worker* worker_ = nullptr;
    std::shared_ptr<FrameGate> current_gate_;
    std::atomic<int> generation_{0};
    std::atomic_bool accepting_{false};
    std::atomic_bool closed_{false};
};

} // namespace c16_l05
