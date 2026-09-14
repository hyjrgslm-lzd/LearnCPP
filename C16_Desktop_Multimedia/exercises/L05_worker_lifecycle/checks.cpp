#include <solution.hpp>

#include <c16/check.hpp>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QSemaphore>
#include <QThread>

#include <atomic>
#include <memory>
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

void drain_events()
{
    for (int i = 0; i < 20; ++i) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
        QThread::yieldCurrentThread();
    }
}

void check_normal_completion()
{
    c16_l05::MediaAnalysisSession session;
    QObject receiver;
    auto gate = std::make_shared<c16_l05::FrameGate>();
    QSemaphore done;
    c16_l05::AnalysisResult result;

    const int id = session.analyze("clip-a", 2, gate, &receiver, [&](c16_l05::AnalysisResult value) {
        result = value;
        done.release();
    });

    c16::require(id > 0, "analyze accepts live receiver and positive work");
    c16::require(wait_for(gate->entered), "worker reaches first controlled frame");
    gate->allow();
    c16::require(wait_for(gate->entered), "worker reaches second controlled frame");
    gate->allow();
    c16::require(wait_for(done), "normal result is delivered through Qt event loop");
    c16::require(result.request_id == id, "result keeps request generation");
    c16::require(result.media_id == "clip-a", "result keeps media id");
    c16::require(result.frames == 2 && !result.cancelled, "normal request finishes all frames");
    c16::require(result.worker_thread != QThread::currentThreadId(), "work runs outside the controller thread");
}

void check_cancel()
{
    c16_l05::MediaAnalysisSession session;
    QObject receiver;
    auto gate = std::make_shared<c16_l05::FrameGate>();
    QSemaphore done;
    c16_l05::AnalysisResult result;

    const int id = session.analyze("cancel-me", 4, gate, &receiver, [&](c16_l05::AnalysisResult value) {
        result = value;
        done.release();
    });

    c16::require(id > 0, "cancel case starts");
    c16::require(wait_for(gate->entered), "cancel case reaches controlled work");
    session.cancel();
    c16::require(wait_for(done), "cancel result is delivered without queuing cancel onto blocked worker");
    c16::require(result.request_id == id && result.cancelled, "cancelled request reports its own generation");
    c16::require(result.frames == 0, "cancel before release does not commit a frame");
}

void check_old_result_is_rejected()
{
    c16_l05::MediaAnalysisSession session;
    QObject receiver;
    auto old_gate = std::make_shared<c16_l05::FrameGate>();
    auto new_gate = std::make_shared<c16_l05::FrameGate>();
    QSemaphore callbacks;
    std::vector<c16_l05::AnalysisResult> results;

    const int old_id = session.analyze("old", 1, old_gate, &receiver, [&](c16_l05::AnalysisResult value) {
        results.push_back(value);
        callbacks.release();
    });
    c16::require(wait_for(old_gate->entered), "old request starts before replacement");

    const int new_id = session.analyze("new", 1, new_gate, &receiver, [&](c16_l05::AnalysisResult value) {
        results.push_back(value);
        callbacks.release();
    });
    c16::require(new_id > old_id, "replacement advances generation");

    old_gate->allow();
    c16::require(wait_for(new_gate->entered), "new request runs after old blocked work unwinds");
    new_gate->allow();

    c16::require(wait_for(callbacks), "new request delivers exactly one accepted result");
    drain_events();
    c16::require(results.size() == 1, "old late result is rejected by generation");
    c16::require(results.front().request_id == new_id && results.front().media_id == "new",
        "accepted result belongs to newest request");
}

void check_destroyed_receiver()
{
    c16_l05::MediaAnalysisSession session;
    auto dead_gate = std::make_shared<c16_l05::FrameGate>();
    auto live_gate = std::make_shared<c16_l05::FrameGate>();
    auto* dead_receiver = new QObject;
    QObject live_receiver;
    std::atomic<int> dead_calls{0};
    QSemaphore live_done;
    c16_l05::AnalysisResult live_result;

    session.analyze("dead", 1, dead_gate, dead_receiver, [&](c16_l05::AnalysisResult) {
        dead_calls.fetch_add(1, std::memory_order_release);
    });
    c16::require(wait_for(dead_gate->entered), "dead receiver request reaches worker");
    delete dead_receiver;

    const int live_id = session.analyze("live", 1, live_gate, &live_receiver, [&](c16_l05::AnalysisResult value) {
        live_result = value;
        live_done.release();
    });

    dead_gate->allow();
    c16::require(wait_for(live_gate->entered), "worker continues after receiver destruction");
    live_gate->allow();
    c16::require(wait_for(live_done), "later live receiver still receives result");
    c16::require(live_result.request_id == live_id, "live request keeps generation after receiver loss");
    drain_events();
    c16::require(dead_calls.load(std::memory_order_acquire) == 0, "destroyed receiver is not called");
}

void check_close_is_finite_and_repeatable()
{
    c16_l05::MediaAnalysisSession session;
    QObject receiver;
    auto gate = std::make_shared<c16_l05::FrameGate>();
    std::atomic<int> calls{0};

    c16::require(session.analyze("closing", 3, gate, &receiver, [&](c16_l05::AnalysisResult) {
        calls.fetch_add(1, std::memory_order_release);
    }) > 0, "close case starts");
    c16::require(wait_for(gate->entered), "close case reaches controlled work");

    session.close();
    session.close();
    drain_events();
    c16::require(!session.isRunning(), "close stops worker thread");
    c16::require(calls.load(std::memory_order_acquire) == 0, "close rejects in-flight result");
    c16::require(session.analyze("after-close", 1, std::make_shared<c16_l05::FrameGate>(), &receiver,
                      [](c16_l05::AnalysisResult) {}) == -1,
        "closed session refuses new work");
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    return c16::run([] {
        check_normal_completion();
        check_cancel();
        check_old_result_is_rejected();
        check_destroyed_receiver();
        check_close_is_finite_and_repeatable();
    });
}
