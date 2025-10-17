#include "ControlWidVideo.h"
#include "ui_ControlWidVideo.h"

ControlWidVideo::ControlWidVideo(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ControlWidVideo)
    , isSliding(false)
{
    ui->setupUi(this);
    
    // 设置滑块初始范围
    ui->horizontalSlider->setRange(0, 0);
    
    // 初始化为播放状态
    ui->pushButton->setText(tr("播放"));
}

ControlWidVideo::~ControlWidVideo()
{
    delete ui;
}

void ControlWidVideo::setSliderRange(int min, int max)
{
    ui->horizontalSlider->setRange(min, max);
}

void ControlWidVideo::setSliderPosition(int position)
{
    if (!isSliding) {
        ui->horizontalSlider->setValue(position);
    }
}

void ControlWidVideo::setPlayButtonText(const QString &text)
{
    ui->pushButton->setText(text);
}

void ControlWidVideo::on_pushButton_clicked()
{
    emit playPauseRequested();
}

void ControlWidVideo::on_horizontalSlider_sliderMoved(int position)
{
    emit sliderMoved(position);
}

void ControlWidVideo::on_horizontalSlider_sliderPressed()
{
    isSliding = true;
    emit sliderPressed();
}

void ControlWidVideo::on_horizontalSlider_sliderReleased()
{
    isSliding = false;
    emit sliderReleased();
    // 释放时也发送一次位置信号，确保视频播放位置更新
    emit sliderMoved(ui->horizontalSlider->value());
}
