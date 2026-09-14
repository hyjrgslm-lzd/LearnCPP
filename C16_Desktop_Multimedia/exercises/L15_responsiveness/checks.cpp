#include <solution.hpp>
#include <c16/check.hpp>
#include <QCoreApplication>
#include <QEvent>
#include <QThread>
#include <atomic>
#include <memory>
#include <thread>
#include <vector>

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    return c16::run([&] {
        std::vector<std::uint64_t> shown;
        bool on_main_thread = true;
        c16_l15::PreviewMailbox box([&](auto value) {
            on_main_thread &= QThread::currentThread() == app.thread();
            shown.push_back(value);
        });
        std::atomic<bool> accepted{true};
        std::jthread producer([&] {
            for (std::uint64_t i = 1; i <= 128; ++i)
                if (!box.submit(i)) accepted = false;
        });
        producer.join();
        c16::require(accepted, "student must accept previews before close");
        c16::require(shown.empty() && box.posted() == 1, "one queued notification while UI is held");
        QCoreApplication::sendPostedEvents(&box, QEvent::MetaCall);
        c16::require(shown == std::vector<std::uint64_t>{128}, "latest preview survives coalescing");
        c16::require(on_main_thread, "UI callback executes in receiver thread");
        box.submit(129);
        box.close();
        box.close();
        c16::require(!box.submit(130), "closed mailbox rejects further input");
        QCoreApplication::sendPostedEvents(&box, QEvent::MetaCall);
        c16::require(shown.size() == 1, "close suppresses pending delivery");

        int calls = 0;
        auto removed = std::make_unique<c16_l15::PreviewMailbox>([&](auto) { ++calls; });
        removed->submit(1);
        removed.reset();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
        c16::require(calls == 0, "destroyed QObject context cancels delivery");

        std::vector<std::uint64_t> reentered;
        c16_l15::PreviewMailbox* pointer = nullptr;
        c16_l15::PreviewMailbox reentrant([&](auto value) {
            reentered.push_back(value);
            if (value == 1) pointer->submit(2);
        });
        pointer = &reentrant;
        reentrant.submit(1);
        QCoreApplication::sendPostedEvents(&reentrant, QEvent::MetaCall);
        QCoreApplication::sendPostedEvents(&reentrant, QEvent::MetaCall);
        c16::require(reentered == std::vector<std::uint64_t>{1, 2}, "callback may submit without deadlock or lost notification");
    });
}
