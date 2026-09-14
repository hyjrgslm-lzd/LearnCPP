#pragma once
#include <memory>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QWidget>

namespace c16_l08 {
inline std::unique_ptr<QWidget> createReviewPanel()
{
    auto panel = std::make_unique<QWidget>();
    auto* layout = new QHBoxLayout(panel.get());
    auto* play = new QPushButton("Play", panel.get());
    play->setObjectName("playButton");
    play->setAccessibleName("播放或暂停预览");
    play->setAccessibleDescription("切换 MediaWorkbench 本地预览的播放状态");
    play->setFocusPolicy(Qt::StrongFocus);
    auto* label = new QLabel("&Timeline", panel.get());
    label->setObjectName("scrubLabel");
    auto* slider = new QSlider(Qt::Horizontal, panel.get());
    slider->setObjectName("scrubSlider");
    slider->setRange(0, 100);
    slider->setValue(0);
    slider->setAccessibleName("媒体时间轴");
    slider->setAccessibleDescription("用左右方向键移动当前审阅位置");
    slider->setFocusPolicy(Qt::StrongFocus);
    label->setBuddy(slider);
    layout->addWidget(play);
    layout->addWidget(label);
    layout->addWidget(slider);
    return panel;
}
}
