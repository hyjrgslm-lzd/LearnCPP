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
    PreviewResult runPreview(QString, int, int) { return {}; }
};
}
