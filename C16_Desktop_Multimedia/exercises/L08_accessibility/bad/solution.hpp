#pragma once
#include <memory>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSlider>
#include <QWidget>

namespace c16_l08 {
inline std::unique_ptr<QWidget> createReviewPanel()
{
    auto panel = std::make_unique<QWidget>();
    auto* layout = new QHBoxLayout(panel.get());
    auto* play = new QPushButton(">", panel.get());
    play->setObjectName("playButton");
    auto* slider = new QSlider(Qt::Horizontal, panel.get());
    slider->setObjectName("scrubSlider");
    slider->setFocusPolicy(Qt::NoFocus);
    layout->addWidget(play);
    layout->addWidget(slider);
    return panel;
}
}
