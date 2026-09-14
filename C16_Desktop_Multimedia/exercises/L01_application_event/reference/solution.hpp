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
        PreviewResult result;
        if (frames <= 0) {
            result.events << "reject:" + media_id;
            result.exit_code = 2;
            return result;
        }

        QEventLoop loop;
        int next = 0;
        result.events << "start:" + media_id;
        QTimer frame_timer;
        frame_timer.setInterval(1);
        QObject::connect(&frame_timer, &QTimer::timeout, &loop, [&] {
            if (next == frames) {
                result.events << "finished";
                result.exit_code = 0;
                loop.quit();
                return;
            }
            result.events << QString("frame:%1").arg(next++);
        });
        QTimer::singleShot(timeout_ms, &loop, [&] {
            if (result.exit_code == -1) {
                result.events << "timeout";
                result.exit_code = 1;
                loop.quit();
            }
        });
        frame_timer.start();
        result.used_exec = true;
        loop.exec();
        return result;
    }
};
}
