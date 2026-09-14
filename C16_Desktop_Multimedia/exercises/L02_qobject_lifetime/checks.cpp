#include <solution.hpp>
#include <c16/check.hpp>

#include <QCoreApplication>
#include <QEvent>
#include <QEventLoop>
#include <QPointer>
#include <QTimer>

namespace {
void drain()
{
    for (int i = 0; i < 10; ++i) {
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    }
}

void check_parent_destroys_children()
{
    bool child_destroyed = false;
    auto parent = c16_l02::makeLibraryTree(&child_destroyed);
    c16::require(parent && parent->findChild<QObject*>("clip"), "heap child is parented and discoverable");
    parent.reset();
    c16::require(child_destroyed, "destroying parent destroys child exactly through QObject tree");
}

void check_stack_parent_is_rejected()
{
    QObject stack_clip;
    stack_clip.setObjectName("stack");
    auto parent = c16_l02::makeLibraryTree(nullptr);
    c16::require(!c16_l02::adoptExternalClip(parent.get(), &stack_clip), "stack/external object is not adopted into tree");
    c16::require(stack_clip.parent() == nullptr, "rejected external object keeps original owner");
}

void check_qpointer_and_deferred_delete()
{
    auto* object = new QObject;
    QPointer<QObject> watched(object);
    c16_l02::deleteWhenIdle(object);
    c16::require(watched, "deleteLater is deferred until events are drained");
    drain();
    c16::require(watched.isNull(), "QPointer clears after deferred QObject destruction");
}
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    return c16::run([] {
        check_parent_destroys_children();
        check_stack_parent_is_rejected();
        check_qpointer_and_deferred_delete();
    });
}
