#pragma once
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
    PreviewResult runPreview(QString media_id, int frames, int)
    {
        PreviewResult result;
        result.events << "start:" + media_id;
        for (int i = 0; i < frames; ++i) result.events << QString("frame:%1").arg(i);
        result.events << "finished";
        result.exit_code = 0;
        return result;
    }
};
}
