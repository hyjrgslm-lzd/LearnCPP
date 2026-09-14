#include <solution.hpp>
#include <c16/check.hpp>

#include <QAccessible>
#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <QSlider>

namespace {
void check_accessible_names_and_roles()
{
    auto panel = c16_l08::createReviewPanel();
    auto* play = panel->findChild<QPushButton*>("playButton");
    auto* scrub = panel->findChild<QSlider*>("scrubSlider");
    c16::require(play && scrub, "panel exposes expected controls");
    c16::require(play->accessibleName() == "播放或暂停预览", "button accessible name is meaningful");
    c16::require(scrub->accessibleName() == "媒体时间轴", "slider accessible name is meaningful");
    c16::require(!play->accessibleDescription().isEmpty(), "button accessible description explains action");
    c16::require(!scrub->accessibleDescription().isEmpty(), "slider accessible description explains keyboard use");
    auto* play_iface = QAccessible::queryAccessibleInterface(play);
    auto* slider_iface = QAccessible::queryAccessibleInterface(scrub);
    c16::require(play_iface && play_iface->role() == QAccessible::Button, "play control reports button role");
    c16::require(slider_iface && slider_iface->role() == QAccessible::Slider, "timeline reports slider role");
    c16::require(play_iface->text(QAccessible::Description).contains("预览")
            || play_iface->text(QAccessible::Description).contains("审阅"),
        "button description is exposed through accessible interface");
    c16::require(slider_iface->text(QAccessible::Description).contains("方向键")
            || slider_iface->text(QAccessible::Description).contains("键盘"),
        "slider keyboard description is exposed through accessible interface");
}

void check_keyboard_focus_and_label()
{
    auto panel = c16_l08::createReviewPanel();
    auto* play = panel->findChild<QPushButton*>("playButton");
    auto* scrub = panel->findChild<QSlider*>("scrubSlider");
    auto* label = panel->findChild<QLabel*>("scrubLabel");
    c16::require(play->focusPolicy() & Qt::TabFocus, "play button is keyboard reachable");
    c16::require(scrub->focusPolicy() & Qt::TabFocus, "timeline is keyboard reachable");
    c16::require(label && label->buddy() == scrub, "label buddy points to timeline");
    auto* play_iface = QAccessible::queryAccessibleInterface(play);
    auto* slider_iface = QAccessible::queryAccessibleInterface(scrub);
    c16::require(play_iface && play_iface->state().focusable, "button reports focusable accessible state");
    c16::require(slider_iface && slider_iface->state().focusable, "slider reports focusable accessible state");
}

void check_action_and_value_text()
{
    auto panel = c16_l08::createReviewPanel();
    auto* play = panel->findChild<QPushButton*>("playButton");
    auto* scrub = panel->findChild<QSlider*>("scrubSlider");
    auto* play_iface = QAccessible::queryAccessibleInterface(play);
    auto* slider_iface = QAccessible::queryAccessibleInterface(scrub);
    c16::require(play_iface->text(QAccessible::Name) == "播放或暂停预览", "accessible interface sees button name");
    c16::require(slider_iface->text(QAccessible::Name) == "媒体时间轴", "accessible interface sees slider name");
    scrub->setValue(42);
    c16::require(slider_iface->text(QAccessible::Value).contains("42"), "slider value is exposed");
}
}

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    return c16::run([] {
        check_accessible_names_and_roles();
        check_keyboard_focus_and_label();
        check_action_and_value_text();
    });
}
