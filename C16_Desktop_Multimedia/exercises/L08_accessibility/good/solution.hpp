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
    auto root = std::make_unique<QWidget>();
    auto* row = new QHBoxLayout(root.get());
    auto* button = new QPushButton("Play", root.get());
    button->setObjectName("playButton");
    button->setAccessibleName("播放或暂停预览");
    button->setAccessibleDescription("开始或暂停本地媒体审阅");
    button->setFocusPolicy(Qt::StrongFocus);
    auto* text = new QLabel("&Timeline", root.get());
    text->setObjectName("scrubLabel");
    auto* slider = new QSlider(Qt::Horizontal, root.get());
    slider->setObjectName("scrubSlider");
    slider->setRange(0, 100);
    slider->setAccessibleName("媒体时间轴");
    slider->setAccessibleDescription("当前媒体位置，键盘方向键可调整");
    slider->setFocusPolicy(Qt::StrongFocus);
    text->setBuddy(slider);
    row->addWidget(button);
    row->addWidget(text);
    row->addWidget(slider);
    return root;
}
}
