#include <c16/check.hpp>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QMetaObject>
#include <QObject>
#include <QSemaphore>
#include <QString>
#include <QStringList>
#include <QThread>

#include <atomic>
#include <iostream>
#include <vector>

namespace {

bool wait_for(QSemaphore& semaphore, int timeout_ms = 2000)
{
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < timeout_ms) {
        if (semaphore.tryAcquire(1)) {
            return true;
        }
        QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
        QThread::yieldCurrentThread();
    }
    return semaphore.tryAcquire(1);
}

struct SyncInput {
    QString media_id;
    int frames = 0;
};

struct SyncResult {
    QString media_id;
    int frames = 0;
};

SyncResult analyze_now(const SyncInput& input, QStringList& trace)
{
    c16::require(!input.media_id.isEmpty(), "sync baseline requires a media id");
    c16::require(input.frames > 0, "sync baseline requires positive frame count");

    SyncResult result{input.media_id, 0};
    for (int frame = 0; frame < input.frames; ++frame) {
        trace << QString("sync frame %1").arg(frame);
        ++result.frames;
    }
    return result;
}

void print_trace(const QString& title, const QStringList& trace)
{
    std::cout << title.toStdString() << '\n';
    for (const QString& line : trace) {
        std::cout << "  " << line.toStdString() << '\n';
    }
}

class BaselineWorker final : public QObject {
public:
    QSemaphore entered;
    QSemaphore resume;
    QSemaphore finished;
    QSemaphore cancel_ran;
    QStringList trace;
    std::atomic_bool cancel_seen{false};

    void analyze()
    {
        trace << "analyze entered worker thread";
        entered.release();
        resume.acquire();
        trace << (cancel_seen.load(std::memory_order_acquire) ? "cancel was visible during work"
                                                              : "cancel was still queued during work");
        finished.release();
    }

    void cancel()
    {
        cancel_seen.store(true, std::memory_order_release);
        trace << "queued cancel slot ran";
        cancel_ran.release();
    }
};

class ThreadCleanup {
public:
    ThreadCleanup(QThread& thread, BaselineWorker& worker) : thread_(thread), worker_(worker) {}
    ~ThreadCleanup()
    {
        worker_.resume.release();
        thread_.quit();
        thread_.wait();
    }

private:
    QThread& thread_;
    BaselineWorker& worker_;
};

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    return c16::run([&] {
        QStringList sync_trace;
        bool queued_marker_ran = false;

        QMetaObject::invokeMethod(&app, [&] {
            queued_marker_ran = true;
            sync_trace << "main queued marker ran";
        }, Qt::QueuedConnection);

        const SyncResult sync = analyze_now({"clip-sync", 3}, sync_trace);
        c16::require(sync.media_id == "clip-sync" && sync.frames == 3,
            "sync baseline counts every input frame");
        c16::require(!queued_marker_ran, "main queued event waits until sync function returns");

        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        c16::require(queued_marker_ran, "main queued event runs after returning to event loop");
        print_trace("sync baseline trace", sync_trace);

        QThread thread;
        BaselineWorker worker;
        worker.moveToThread(&thread);
        thread.start();
        ThreadCleanup cleanup(thread, worker);

        QMetaObject::invokeMethod(&worker, [&] { worker.analyze(); }, Qt::QueuedConnection);
        c16::require(wait_for(worker.entered), "baseline worker reached blocking work");

        QMetaObject::invokeMethod(&worker, [&] { worker.cancel(); }, Qt::QueuedConnection);
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        c16::require(!worker.cancel_seen.load(std::memory_order_acquire),
            "queued cancel cannot run while the same worker is blocked");

        worker.resume.release();
        c16::require(wait_for(worker.finished), "baseline finishes after test releases the gate");
        c16::require(wait_for(worker.cancel_ran), "queued cancel eventually runs after worker returns");
        c16::require(worker.cancel_seen.load(std::memory_order_acquire),
            "queued cancel runs only after analyze returns to the worker event loop");
        print_trace("worker queued-cancel trace", worker.trace);
    });
}
