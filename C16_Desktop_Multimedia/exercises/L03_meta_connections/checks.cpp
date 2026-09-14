#include <solution.hpp>
#include <c16/check.hpp>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QSemaphore>
#include <QThread>

namespace {
void drain()
{
    for (int i = 0; i < 20; ++i) QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
}

bool wait_for(QSemaphore& semaphore, int timeout_ms = 2000)
{
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < timeout_ms) {
        if (semaphore.tryAcquire(1)) return true;
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        QThread::yieldCurrentThread();
    }
    return semaphore.tryAcquire(1);
}

void check_direct_delivery()
{
    c16_l03::MediaSource source;
    c16_l03::FrameSink sink;
    c16_l03::connectFrames(&source, &sink);
    source.publish("a", 7);
    c16::require(sink.frames.size() == 1, "direct same-thread signal is delivered");
    c16::require(sink.frames.front().media_id == "a" && sink.frames.front().frame == 7, "signal arguments survive");
}

void check_context_disconnect()
{
    c16_l03::MediaSource source;
    auto* sink = new c16_l03::FrameSink;
    int calls = 0;
    QObject guard;
    QObject::connect(sink, &c16_l03::FrameSink::received, &guard, [&] { ++calls; });
    c16_l03::connectFrames(&source, sink);
    source.publish("first", 0);
    c16::require(calls == 1, "live receiver sees first signal");
    delete sink;
    source.publish("late", 1);
    drain();
    c16::require(calls == 1, "destroyed receiver context prevents late callback");
}

void check_auto_connection_uses_emit_thread()
{
    c16_l03::MediaSource source;
    c16_l03::FrameSink sink;
    QSemaphore done;
    QObject::connect(&sink, &c16_l03::FrameSink::received, &sink, [&] { done.release(); });
    c16_l03::connectFrames(&source, &sink);
    QThread* worker = QThread::create([&] { source.publish("worker", 2); });
    worker->start();
    c16::require(wait_for(done), "queued AutoConnection reaches receiver event loop");
    worker->wait();
    delete worker;
    c16::require(sink.frames.size() == 1, "one worker frame delivered");
    c16::require(sink.frames.front().receiver_thread == QThread::currentThreadId(),
        "slot executes in receiver thread for cross-thread AutoConnection");
}
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    return c16::run([] {
        check_direct_delivery();
        check_context_disconnect();
        check_auto_connection_uses_emit_thread();
    });
}
