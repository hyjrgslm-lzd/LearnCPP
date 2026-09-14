#include <c16/check.hpp>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEvent>
#include <QMetaObject>
#include <QObject>
#include <charconv>
#include <cstdint>
#include <iostream>
#include <string_view>
#include <thread>

static std::uint64_t preview(int i) { return std::uint64_t(i) * 2654435761ULL ^ 17ULL; }

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    try {
        int count = 30000;
        if (argc > 1) {
            const std::string_view text(argv[1]);
            const auto parsed = std::from_chars(text.data(), text.data() + text.size(), count);
            c16::require(parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size(), "integer workload");
        }
        c16::require(count > 0 && count <= 100000, "bounded workload 1..100000");
        QObject receiver;
        int applied = 0;
        std::uint64_t final_value = 0;
        QElapsedTimer timer;
        timer.start();
        std::jthread producer([&] {
            for (int i = 0; i < count; ++i) {
                const auto value = preview(i);
                QMetaObject::invokeMethod(&receiver, [&, value] {
                    final_value = value;
                    ++applied;
                }, Qt::QueuedConnection);
            }
        });
        producer.join();
        const auto post_ns = timer.nsecsElapsed();
        c16::require(applied == 0, "UI has not serviced the event queue during controlled production");
        timer.restart();
        QCoreApplication::sendPostedEvents(&receiver, QEvent::MetaCall);
        const auto drain_ns = timer.nsecsElapsed();
        c16::require(applied == count && final_value == preview(count - 1), "baseline delivers every update and correct final preview");
        std::cout << "{\"version\":\"per-event\",\"count\":" << count
                  << ",\"submitted\":" << count << ",\"callbacks\":" << applied
                  << ",\"peak_pending\":" << count << ",\"post_ns\":" << post_ns
                  << ",\"drain_ns\":" << drain_ns << ",\"final\":" << final_value << "}\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
