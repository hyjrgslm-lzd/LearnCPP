#include <c16/check.hpp>

#include <QAudioDevice>
#include <QAudioFormat>
#include <QAudioSink>
#include <QByteArray>
#include <QCoreApplication>
#include <QIODevice>
#include <QMediaDevices>
#include <QTimer>

#include <algorithm>
#include <iostream>

namespace {

QByteArray silence_for(const QAudioFormat& format, qint64 duration_us)
{
    const auto bytes = format.bytesForDuration(duration_us);
    c16::require(bytes > 0, "preferred audio format cannot represent 100ms");
    QByteArray data(bytes, '\0');
    if (format.sampleFormat() == QAudioFormat::UInt8) {
        const unsigned char midpoint = 128;
        std::fill(data.begin(), data.end(), static_cast<char>(midpoint));
    }
    return data;
}

void check_default_output_device()
{
    const QAudioDevice device = QMediaDevices::defaultAudioOutput();
    if (device.isNull()) {
        std::cout << "SKIP: no default audio output device\n";
        std::exit(77);
    }

    const QAudioFormat format = device.preferredFormat();
    c16::require(format.isValid(), "default audio output has invalid preferred format");
    c16::require(device.isFormatSupported(format), "default audio output does not support its preferred format");

    QAudioSink sink(device, format);
    c16::require(!sink.isNull(), "QAudioSink construction failed");
    sink.setVolume(0.0);
    sink.setBufferSize(format.bytesForDuration(200000));

    QIODevice* output = sink.start();
    c16::require(output != nullptr, "QAudioSink did not return a writable device");
    c16::require(sink.state() != QAudio::StoppedState || sink.error() == QAudio::NoError,
        "QAudioSink stopped immediately after start");

    const QByteArray payload = silence_for(format, 100000);
    qsizetype written = 0;
    bool failed = false;
    bool timed_out = false;
    QEventLoop loop;
    QTimer poll;
    poll.setInterval(5);

    QObject::connect(&sink, &QAudioSink::stateChanged, &loop, [&] {
        if (sink.state() == QAudio::StoppedState && sink.error() != QAudio::NoError) {
            failed = true;
            loop.quit();
        }
    });
    QObject::connect(&poll, &QTimer::timeout, &loop, [&] {
        if (failed) {
            loop.quit();
            return;
        }
        while (written < payload.size() && sink.bytesFree() > 0) {
            const auto count = std::min<qsizetype>(sink.bytesFree(), payload.size() - written);
            const auto n = output->write(payload.constData() + written, count);
            if (n <= 0) {
                failed = true;
                loop.quit();
                return;
            }
            written += n;
        }
        if (written == payload.size() && sink.processedUSecs() > 0) {
            loop.quit();
        }
    });

    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(&timeout, &QTimer::timeout, &loop, [&] {
        timed_out = true;
        failed = true;
        loop.quit();
    });

    poll.start();
    timeout.start(1500);
    loop.exec();
    poll.stop();

    const auto processed = sink.processedUSecs();
    sink.stop();

    c16::require(!timed_out, "default audio output timed out before processing silence");
    c16::require(!failed, "default audio output did not accept and process silence before timeout");
    c16::require(written == payload.size(), "default audio output accepted only partial silence payload");
    c16::require(processed > 0, "default audio output did not advance processedUSecs");
    std::cout << "PASS device audio: rate=" << format.sampleRate()
              << " channels=" << format.channelCount()
              << " bytes=" << written
              << " processed_us=" << processed << '\n';
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    return c16::run(check_default_output_device);
}
