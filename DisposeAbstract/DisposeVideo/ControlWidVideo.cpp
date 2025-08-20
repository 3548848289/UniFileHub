#include "ControlWidVideo.h"
#include "ui_ControlWidVideo.h"

ControlWidVideo::ControlWidVideo(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ControlWidVideo)
{
    ui->setupUi(this);
}

ControlWidVideo::~ControlWidVideo()
{
    delete ui;
}

// // 播放按钮
// QPushButton *playButton = new QPushButton(tr("播放"), this);

// // 播放进度条
// QSlider *slider = new QSlider(Qt::Horizontal, this);
// slider->setRange(0, 0); // 先设空范围，等时长获取后更新

// // 播放/暂停控制
// connect(playButton, &QPushButton::clicked, this, [=]() {
//     if (player->playbackState() == QMediaPlayer::PlayingState) {
//         player->pause();
//         playButton->setText(tr("播放"));
//     } else {
//         player->play();
//         playButton->setText(tr("暂停"));
//     }
// });

// // 视频时长变化时更新 slider 范围
// connect(player, &QMediaPlayer::durationChanged, this, [=](qint64 duration) {
//     slider->setRange(0, static_cast<int>(duration));
// });

// // 播放进度变化时更新 slider
// connect(player, &QMediaPlayer::positionChanged, this, [=](qint64 pos) {
//     if (!slider->isSliderDown()) {
//         slider->setValue(static_cast<int>(pos));
//     }
// });

// // 拖动 slider 改变播放位置
// connect(slider, &QSlider::sliderMoved, this, [=](int position) {
//     player->setPosition(position);
// });
