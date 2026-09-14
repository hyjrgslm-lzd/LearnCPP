#include "workbench.hpp"

#include <c16/check.hpp>

#include <QApplication>
#include <QDir>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTest>
#include <QTimer>

namespace {

const QUrl mainQmlUrl(QStringLiteral("qrc:/C16QuickBridge/Main.qml"));

void pump(int ms = 50)
{
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < ms) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        QTest::qWait(10);
    }
}

int runSelfCheck(const QString& fixtures)
{
    return c16::run([&] {
        workbench::SessionController controller;
        c16::require(controller.addMedia(QDir(fixtures).filePath("c16_pulse_mono_48k_s16.wav")), "quick imports wav");
        c16::require(controller.addMedia(QDir(fixtures).filePath("c16_rgb24_pcm_1s.avi")), "quick imports avi");

        QQmlApplicationEngine engine;
        engine.setInitialProperties({{"controller", QVariant::fromValue(&controller)},
            {"mediaModel", QVariant::fromValue(controller.model())}});
        engine.load(mainQmlUrl);
        c16::require(!engine.rootObjects().isEmpty(), "qml root loads");
        auto* root = qobject_cast<QQuickWindow*>(engine.rootObjects().front());
        c16::require(root != nullptr && root->isVisible(), "qml root is a visible window");
        auto* list = root->findChild<QQuickItem*>("mediaList");
        auto* play = root->findChild<QQuickItem*>("playButton");
        c16::require(list && play, "qml list and play button exist");

        c16::require(controller.model()->rowCount() == 2, "qml sees shared C++ model rows");
        controller.model()->setFilter("pulse");
        pump();
        c16::require(list->property("count").toInt() == 1, "ListView count follows C++ model filter");
        c16::require(controller.model()->rowCount() == 1, "qml filter uses same model object");
        controller.model()->setFilter({});
        c16::require(controller.selectRow(0), "controller selection works from quick path");
        pump();
        c16::require(root->property("selectedText").toString().contains("pulse"), "qml binding sees selectedName");
        QTest::mouseClick(root, Qt::LeftButton, {}, play->mapToScene(QPointF(play->width() / 2, play->height() / 2)).toPoint());
        pump();
        c16::require(controller.playing(), "Quick play button calls shared controller");
        QTest::mouseClick(root, Qt::LeftButton, {}, play->mapToScene(QPointF(play->width() / 2, play->height() / 2)).toPoint());
        controller.close();
    });
}

} // namespace

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    const QStringList args = app.arguments();
    if (args.contains("--self-check")) {
        const int fixtureIndex = args.indexOf("--fixtures");
        const QString fixtures = fixtureIndex >= 0 && fixtureIndex + 1 < args.size()
            ? args[fixtureIndex + 1]
            : QDir::currentPath();
        return runSelfCheck(fixtures);
    }
    workbench::SessionController controller;
    QQmlApplicationEngine engine;
    engine.setInitialProperties({{"controller", QVariant::fromValue(&controller)},
        {"mediaModel", QVariant::fromValue(controller.model())}});
    engine.load(mainQmlUrl);
    if (engine.rootObjects().isEmpty()) {
        return 1;
    }
    const int code = app.exec();
    controller.close();
    return code;
}
