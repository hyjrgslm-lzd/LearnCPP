#pragma once
#include <QEventLoop>
#include <QTimer>
#include <QString>
#include <QStringList>

namespace c16_l01 {
struct PreviewResult {
    QStringList events;
    int exit_code = -1;
    bool used_exec = false;
};

class PreviewController {
public:
    PreviewResult runPreview(QString media_id, int frames, int timeout_ms)
    {
        PreviewResult out;
        if (frames < 1) {
            out.events << "reject:" + media_id;
            out.exit_code = 2;
            return out;
        }
        QEventLoop loop;
        QTimer timer;
        int frame = 0;
        out.events << "start:" + media_id;
        QObject::connect(&timer, &QTimer::timeout, &loop, [&] {
            if (frame == frames) {
                out.events << "finished";
                out.exit_code = 0;
                loop.exit(0);
            } else {
                out.events << QString("frame:%1").arg(frame++);
            }
        });
        QTimer::singleShot(timeout_ms, &loop, [&] {
            if (out.exit_code < 0) {
                out.events << "timeout";
                out.exit_code = 1;
                loop.exit(1);
            }
        });
        timer.start(1);
        out.used_exec = true;
        loop.exec();
        return out;
    }
};
}
