#include <solution.hpp>
#include <c16/check.hpp>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QStringList>

namespace {
void check_preview_finishes()
{
    c16_l01::PreviewController controller;
    auto result = controller.runPreview("intro.mov", 3, 1000);
    c16::require(result.exit_code == 0, "preview exits through its own event loop");
    c16::require(result.events == QStringList({"start:intro.mov", "frame:0", "frame:1", "frame:2", "finished"}),
        "preview preserves event order");
    c16::require(result.used_exec, "exercise must run a finite Qt event loop");
}

void check_rejects_empty_work()
{
    c16_l01::PreviewController controller;
    auto result = controller.runPreview("empty.mov", 0, 1000);
    c16::require(result.exit_code == 2, "empty previews are rejected before exec");
    c16::require(result.events == QStringList({"reject:empty.mov"}), "rejection is explicit");
}

void check_timeout_is_finite()
{
    c16_l01::PreviewController controller;
    QElapsedTimer timer;
    timer.start();
    auto result = controller.runPreview("too-long.mov", 100, 1);
    c16::require(timer.elapsed() < 500, "timeout keeps the local event loop finite");
    c16::require(result.exit_code == 1, "timeout uses a distinct exit code");
    c16::require(result.events.contains("timeout"), "timeout is recorded");
}
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    return c16::run([] {
        check_preview_finishes();
        check_rejects_empty_work();
        check_timeout_is_finite();
    });
}
